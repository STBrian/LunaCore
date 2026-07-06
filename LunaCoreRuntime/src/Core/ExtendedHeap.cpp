#include "ExtendedHeap.hpp"
#include <FsLib/fslib.hpp>
#include "Core/Filesystem.hpp"
#include <CTRPluginFramework.hpp>
#include "Core/Debug.hpp"
#include "Core/CrashHandler.hpp"

#include "csvc.h"
#include "tlsf.h"
#include "Helpers/Mutex.hpp"

#define PAGE_SIZE (0x1000*2) // 4kb
#define START_EXTHEAP_ADDR 0x02000000
#define MAX_LOADED_PAGES (128/4)

namespace CTRPluginFramework::ProcessImpl {
    void    UpdateMemRegions(bool ignoreLock);
}

typedef struct {
    bool mapped;
    u32 idx;
    u32 srcAddr;
    u32 dstAddr;
} PageMapInfo;

typedef struct {
    Core::File swapfile;
    PageMapInfo* mappedPages;
    u16* pageMissCount;
    u16 maxLoadedPages;
    u32 maxPages;
    u32 heapStart;
    u32 heapEnd;
    tlsf_t allocator;
} ExtHeapCtx;

static ExtHeapCtx HeapCtx;
static Handle curHandle = 0;
Core::Mutex HeapCtx_mutex;

static u32 alignToPageStart(u32 addr) {
    return addr - (addr & (PAGE_SIZE - 1));
}

#define GET_PAGE_INDEX(pageaddr) ((u32)(((u32)pageaddr - START_EXTHEAP_ADDR) / PAGE_SIZE))

static s32 ctr_enable_all_svc_kernel(void)
{
   __asm__ volatile("cpsid aif");

   unsigned int*  svc_access_control = *(*(unsigned int***)0xFFFF9000 + 0x22) - 0x6;

   svc_access_control[0]=0xFFFFFFFE;
   svc_access_control[1]=0xFFFFFFFF;
   svc_access_control[2]=0xFFFFFFFF;
   svc_access_control[3]=0x3FFFFFFF;
   return 0;
}

static void invalidate_mmu_cache() {
    asm volatile (
        "mov r0, #0\n"
        "mcr p15, 0, r0, c8, c7, 0\n"
        "mcr p15, 0, r0, c7, c5, 0\n"
    );
}

namespace CTRPluginFramework::ProcessImpl {
    extern u32 exceptionCount;

    void ReturnFromException(CpuRegisters* regs);

     void    UnlockGameThreads(void);
}

namespace CTRPluginFrameworkImpl::Services::GSP {
    void    PauseInterruptReceiver(void);
}

namespace CTRPluginFramework::ScreenImpl {
    void    SwitchFrameBuffers(bool game);
}

static const char* CrashHandlerExtendedHeapCallback(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
    const bool manuallyCalled = excep == nullptr || regs == nullptr;
    if (!manuallyCalled && excep->type == ERRF_ExceptionType::ERRF_EXCEPTION_DATA_ABORT) {
        u32 status = (excep->fsr & 0xF) | ((excep->fsr >> 6) & 0x10);
        if ((status == 0x05 || status == 0x07) && ExtendedHeapIsAddressInHeap(excep->far)) { // Section | Page translation fault
            LOGDEBUG("Page fault reached at: %08X", excep->far);
            //CTRPF::ScreenImpl::SwitchFrameBuffers(true);
            Result status = ExtendedHeapAttemptLoadPage(excep->far);
            if (R_FAILED(status)) {
                return "Page swap failed";
            } else {
                AtomicPostDecrement(&CTRPF::ProcessImpl::exceptionCount);
                CTRPluginFrameworkImpl::Services::GSP::PauseInterruptReceiver(); 
                CTRPF::ProcessImpl::UnlockGameThreads();
                // LOGDEBUG("Resuming to PC: %08X", regs->pc);
                //svcSleepThread(1000000LL);
                CTRPF::ProcessImpl::ReturnFromException(regs);
            }
        }
    }
    return nullptr;
}

Result ExtendedHeapInit(size_t heapsize) {
    const u32 heapsize_b = heapsize * PAGE_SIZE;
    const u16 maxLoadedPages = heapsize < MAX_LOADED_PAGES ? heapsize : MAX_LOADED_PAGES;

    LOGDEBUG("Init extended heap");
    if (!Core::Filesystem::DirectoryExists("lcfs:/LunaCore")) {
        if (!Core::Filesystem::CreateDirectory("lcfs:/LunaCore")) 
            return -1;
    }
    HeapCtx.swapfile.open("lcfs:/LunaCore/swapfile.bin", FS_OPEN_CREATE|FS_OPEN_READ|FS_OPEN_WRITE, heapsize_b);
    if (!HeapCtx.swapfile.isOpen()) return -1;

    HeapCtx.heapStart = START_EXTHEAP_ADDR;
    HeapCtx.heapEnd = START_EXTHEAP_ADDR + heapsize_b;
    HeapCtx.maxPages = heapsize;
    HeapCtx.maxLoadedPages = maxLoadedPages;
    
    PageMapInfo* mappedPages = (PageMapInfo*)calloc(sizeof(PageMapInfo), maxLoadedPages);
    u16* pageMissCount = (u16*)calloc(sizeof(u16), heapsize);

    if (mappedPages == nullptr) goto error;
    if (pageMissCount == nullptr) goto error;

    HeapCtx.mappedPages = mappedPages;
    HeapCtx.pageMissCount = pageMissCount;
    
    // Initialize pages
    for (size_t i = 0; i < maxLoadedPages; i++) {
        mappedPages[i].srcAddr = (u32)aligned_alloc(0x1000, PAGE_SIZE);
        if (mappedPages[i].srcAddr == 0) goto error;
        mappedPages[i].idx = i;
        mappedPages[i].dstAddr = START_EXTHEAP_ADDR + PAGE_SIZE * i;
    }

    // Map pages
    LOGDEBUG("Mapping initial pages");
    svcDuplicateHandle(&curHandle, CUR_PROCESS_HANDLE);
    for (size_t i = 0; i < maxLoadedPages; i++) {
        LOGDEBUG("Mirroring %d", i);
        Result res = svcControlProcessMemory(curHandle, mappedPages[i].dstAddr, mappedPages[i].srcAddr, PAGE_SIZE, MEMOP_MAP, MEMPERM_READWRITE);
        //Result res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, mappedPages[i].dstAddr, CUR_PROCESS_HANDLE, mappedPages[i].srcAddr, PAGE_SIZE, MAPEXFLAGS_PRIVATE);
        if (R_FAILED(res)) {
            LOGDEBUG("Failed to mirror %08X to %08X", mappedPages[i].srcAddr, mappedPages[i].dstAddr);
            goto error;
        } else 
            mappedPages[i].mapped = true;
    }

    LOGDEBUG("Creating allocator");
    Core::CrashHandler::SetCallback(CrashHandlerExtendedHeapCallback);
    HeapCtx.allocator = tlsf_create_with_pool((void*)START_EXTHEAP_ADDR, heapsize_b);
    {
        LOGDEBUG("Test malloc");
        void* test = ExtendedHeapMalloc(100);
        if (test == NULL) {
            LOGDEBUG("Test malloc failed");
            goto error;
        } else {
            LOGDEBUG("Test free");
            ExtendedHeapFree(test);
        }
    }
    return 0;

    error:
    svcCustomBackdoor((void*)invalidate_mmu_cache);
    HeapCtx.swapfile.close();
    if (pageMissCount) free(pageMissCount);
    if (mappedPages) {
        // Free blocks if any
        for (size_t i = 0; i < maxLoadedPages; i++) {
            if (mappedPages[i].mapped) svcControlProcessMemory(curHandle, mappedPages[i].dstAddr, mappedPages[i].srcAddr, PAGE_SIZE, MEMOP_UNMAP, MEMPERM_READWRITE);
            if (mappedPages[i].srcAddr != 0) free((void*)mappedPages[i].srcAddr);
        }
        free(mappedPages);
    }
    if (curHandle != 0) svcCloseHandle(curHandle);
    svcCustomBackdoor((void*)invalidate_mmu_cache);
    return -1;
}

bool ExtendedHeapIsAddressInHeap(u32 addr) {
    return (addr >= HeapCtx.heapStart && addr < HeapCtx.heapEnd);
}

static void swapfileFlush() {
    HeapCtx.swapfile.flush();
}

static void writePageContent(u32 addr) {
    const size_t fileOffset = addr - START_EXTHEAP_ADDR;
    HeapCtx.swapfile.seek(fileOffset, SEEK_SET);
    u32 bytesWritten = 0;
    while (bytesWritten < PAGE_SIZE) {
        int status = HeapCtx.swapfile.write((void*)(addr + bytesWritten), 0x800);
        if (status != 0x800) Core::Abort("Failed to write page to swapfile");
        bytesWritten += 0x800;
    }
    swapfileFlush();
}

static void loadPageContent(u32 addr) {
    const size_t fileOffset = addr - START_EXTHEAP_ADDR;
    int noff = HeapCtx.swapfile.seek(fileOffset, SEEK_SET);
    u32 bytesRead = 0;
    while (bytesRead < PAGE_SIZE) {
        int status = HeapCtx.swapfile.read((void*)(addr + bytesRead), 0x800);
        if (status != 0x800) Core::Abort(CTRPF::Utils::Format("Failed to read swap page! %08X", status).c_str());
        bytesRead += 0x800;
    }
}

static u16 getMinHitMissLoadedPage() {
    const u32 maxLoadedPages = HeapCtx.maxLoadedPages;
    const u16* pageMissCount = HeapCtx.pageMissCount;
    const PageMapInfo* mappedPages = HeapCtx.mappedPages;

    u8 minMappedIdx = 0;
    u16 minHitMiss = pageMissCount[mappedPages[0].idx];
    for (u16 i = 1; i < maxLoadedPages; i++) {
        const u32 curIdx = mappedPages[i].idx;
        if (pageMissCount[curIdx] < minHitMiss) {
            minHitMiss = pageMissCount[curIdx];
            minMappedIdx = i;
        }
    }
    return minMappedIdx;
}

Result ExtendedHeapAttemptLoadPage(u32 addr) {
    static u32 counter = 0;
    MemInfo info;
    PageInfo out;
    Result status;
    const u32 pageStart = alignToPageStart(addr);
    const u32 pageIdx = GET_PAGE_INDEX(pageStart);
    HeapCtx_mutex.lock();
    const u16 minMappedIdx = getMinHitMissLoadedPage();

    // LOGDEBUG("Swap call #%d", counter++);
    // Increase miss counters
    HeapCtx.pageMissCount[pageIdx]++;
    if (pageIdx > 0) HeapCtx.pageMissCount[pageIdx-1]++;
    if (pageIdx < HeapCtx.maxPages) HeapCtx.pageMissCount[pageIdx+1]++;

    // Save old and load new
    PageMapInfo& pageInfo = HeapCtx.mappedPages[minMappedIdx];
    //LOGDEBUG("Swaping memory page %08X", pageInfo.dstAddr);
    svcInvalidateEntireInstructionCache();
    svcFlushEntireDataCache();
    writePageContent(pageInfo.dstAddr);
    status = svcControlProcessMemory(curHandle, pageInfo.dstAddr, pageInfo.srcAddr, PAGE_SIZE, MEMOP_UNMAP, MEMPERM_READWRITE);
    if (status != 0) {
        LOGDEBUG("Failed to unmap memory page");
        LOGDEBUG("Unmap status code: %08X", status);
        HeapCtx_mutex.unlock();
        return -1;
    }
    // svcCustomBackdoor((void*)invalidate_mmu_cache);
    pageInfo.dstAddr = pageStart;
    pageInfo.idx = pageIdx;
    status = svcQueryProcessMemory(&info, &out, CUR_PROCESS_HANDLE, pageInfo.dstAddr);
    if (info.state != MemState::MEMSTATE_FREE) {
        LOGDEBUG("Failed to map memory at %08X. Expected to be free, instead: %d", pageInfo.dstAddr, info.state);
        HeapCtx_mutex.unlock();
        return -4;
    }
    status = svcControlProcessMemory(curHandle, pageInfo.dstAddr, pageInfo.srcAddr, PAGE_SIZE, MEMOP_MAP, MEMPERM_READWRITE);
    if (R_FAILED(status)) {
        LOGDEBUG("Failed to map memory page. Error code: %08X", status);
        HeapCtx_mutex.unlock();
        return -2;
    }
    svcCustomBackdoor((void*)invalidate_mmu_cache);
    LOGDEBUG("Loading memory page %08X", pageInfo.dstAddr);
    CTRPluginFramework::ProcessImpl::UpdateMemRegions(true);
    loadPageContent(pageInfo.dstAddr);
    svcInvalidateEntireInstructionCache();
    svcFlushEntireDataCache();
    HeapCtx_mutex.unlock();
    return 0;
}

void* ExtendedHeapMalloc(size_t size) {
    return tlsf_malloc(HeapCtx.allocator, size);
}

void* ExtendedHeapRealloc(void* ptr, size_t size) {
    return tlsf_realloc(HeapCtx.allocator, ptr, size);
}

void ExtendedHeapFree(void* p) {
    return tlsf_free(HeapCtx.allocator, p);
}