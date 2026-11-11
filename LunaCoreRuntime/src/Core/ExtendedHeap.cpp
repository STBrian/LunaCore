#include "ExtendedHeap.hpp"
#include "Core/Filesystem.hpp"
#include <CTRPluginFramework.hpp>
#include "Core/Debug.hpp"
#include "Core/CrashHandler.hpp"

#include "csvc.h"
#include "tlsf.h"
#include "Helpers/Mutex.hpp"

#define PAGE_SIZE (0x1000) // 4KB (1 physical pages)
#define START_EXTHEAP_ADDR 0x02000000
#define MAX_LOADED_PAGES 96 // 96 * 4KB = 384KB (well within the 1024KB free CTRPF heap)

// #define BUILD_AZAHAR 1

namespace CTRPluginFramework::ProcessImpl {
    void    UpdateMemRegions(bool ignoreLock);
}

namespace CTRPluginFramework::Heap {
    u32 SpaceFree(void);
}

typedef struct {
    bool mapped;
    u32 idx;
    u32 srcAddr;
    u32 dstAddr;
    u32 lastAccess;
} PageMapInfo;

typedef struct {
    Core::File swapfile;
    PageMapInfo* mappedPages;
    u16* pageMissCount;
    u8* pageWrittenToSwap;
    u16 maxLoadedPages;
    u32 maxPages;
    u32 heapStart;
    u32 heapEnd;
    tlsf_t allocator;
} ExtHeapCtx;

static ExtHeapCtx HeapCtx;
#ifndef BUILD_AZAHAR
static Handle curHandle = 0;
#endif
Core::Mutex HeapCtx_mutex;
static u32 g_lruCounter = MAX_LOADED_PAGES;

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
        "mcr p15, 0, r0, c8, c7, 0\n"  // Invalidate unified TLB
        "mcr p15, 0, r0, c7, c5, 0\n"  // Invalidate instruction cache
        "mcr p15, 0, r0, c7, c10, 4\n" // Data Synchronization Barrier (DSB)
        "mcr p15, 0, r0, c7, c5, 4\n"  // PrefetchFlush / ISB
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
                LOGDEBUG("Resuming to PC: %08X", regs->pc);
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

    Core::Debug::LogInfof("Init extended heap. Available CTRPF heap: %u KB", CTRPluginFramework::Heap::SpaceFree() / 1024);
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
    u8* pageWrittenToSwap = (u8*)calloc(sizeof(u8), heapsize);

    if (mappedPages == nullptr) goto error;
    if (pageMissCount == nullptr) goto error;
    if (pageWrittenToSwap == nullptr) goto error;

    HeapCtx.mappedPages = mappedPages;
    HeapCtx.pageMissCount = pageMissCount;
    HeapCtx.pageWrittenToSwap = pageWrittenToSwap;
    
    // Initialize pages
    for (size_t i = 0; i < maxLoadedPages; i++) {
        mappedPages[i].srcAddr = (u32)aligned_alloc(0x1000, PAGE_SIZE);
        if (mappedPages[i].srcAddr == 0) goto error;
        mappedPages[i].idx = i;
        mappedPages[i].dstAddr = START_EXTHEAP_ADDR + PAGE_SIZE * i;
        mappedPages[i].lastAccess = (u32)i;
        mappedPages[i].mapped = false;
        memset((void*)mappedPages[i].srcAddr, 0, PAGE_SIZE);
    }

    // Map pages
    LOGDEBUG("Mapping initial pages");
    #ifndef BUILD_AZAHAR
    svcDuplicateHandle(&curHandle, CUR_PROCESS_HANDLE);
    #endif
    for (size_t i = 0; i < maxLoadedPages; i++) {
        LOGDEBUG("Mirroring %d", i);
        #ifndef BUILD_AZAHAR
        Result res = svcControlProcessMemory(curHandle, mappedPages[i].dstAddr, mappedPages[i].srcAddr, PAGE_SIZE, MEMOP_MAP, MEMPERM_READWRITE);
        #else
        Result res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, mappedPages[i].dstAddr, CUR_PROCESS_HANDLE, mappedPages[i].srcAddr, PAGE_SIZE);
        svcCustomBackdoor((void*)invalidate_mmu_cache);
        #endif
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
    if (pageWrittenToSwap) free(pageWrittenToSwap);
    if (mappedPages) {
        // Free blocks if any
        for (size_t i = 0; i < maxLoadedPages; i++) {
            #ifndef BUILD_AZAHAR
            if (mappedPages[i].mapped) svcControlProcessMemory(curHandle, mappedPages[i].dstAddr, mappedPages[i].srcAddr, PAGE_SIZE, MEMOP_UNMAP, MEMPERM_READWRITE);
            #else 
            if (mappedPages[i].mapped) svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, mappedPages[i].dstAddr, PAGE_SIZE);
            #endif
            if (mappedPages[i].srcAddr != 0) free((void*)mappedPages[i].srcAddr);
        }
        free(mappedPages);
    }
    #ifndef BUILD_AZAHAR
    if (curHandle != 0) svcCloseHandle(curHandle);
    #endif
    svcCustomBackdoor((void*)invalidate_mmu_cache);
    return -1;
}

bool ExtendedHeapIsAddressInHeap(u32 addr) {
    return (addr >= HeapCtx.heapStart && addr < HeapCtx.heapEnd);
}

static void writePageContent(u32 addr) {
    const size_t fileOffset = addr - START_EXTHEAP_ADDR;
    HeapCtx.swapfile.seek(fileOffset, SEEK_SET);
    int status = HeapCtx.swapfile.write((void*)addr, PAGE_SIZE);
    if (status != PAGE_SIZE) Core::Abort("Failed to write page to swapfile");
}

static void loadPageContent(u32 addr) {
    const size_t fileOffset = addr - START_EXTHEAP_ADDR;
    HeapCtx.swapfile.seek(fileOffset, SEEK_SET);
    int status = HeapCtx.swapfile.read((void*)addr, PAGE_SIZE);
    if (status != PAGE_SIZE) Core::Abort(CTRPF::Utils::Format("Failed to read swap page! %08X", status).c_str());
}

static u16 getVictimPageSlot(u32 incomingPageIdx) {
    if (HeapCtx.maxLoadedPages <= 1) {
        return 0;
    }

    // 1. If any slot is currently unmapped, use it immediately
    for (u16 i = 1; i < HeapCtx.maxLoadedPages; i++) {
        if (!HeapCtx.mappedPages[i].mapped) {
            return i;
        }
    }

    // 2. Select victim using frequency + foundation protection + recency
    u64 bestScore = 0xFFFFFFFFFFFFFFFFULL;
    u16 victimSlot = 1;

    for (u16 i = 1; i < HeapCtx.maxLoadedPages; i++) {
        if (HeapCtx.mappedPages[i].idx == incomingPageIdx) {
            continue;
        }

        const u32 pIdx = HeapCtx.mappedPages[i].idx;
        // Protect early foundation pages (0-3 contain TLSF control headers and initial Lua state)
        const u64 foundationPenalty = (pIdx < 4) ? 1000000000ULL : 0ULL;
        // High miss count indicates a hot working-set page that must be kept in memory
        const u64 missPenalty = (u64)HeapCtx.pageMissCount[pIdx] * 1000000ULL;
        // Recency tie-breaker
        const u64 score = foundationPenalty + missPenalty + HeapCtx.mappedPages[i].lastAccess;

        if (score < bestScore) {
            bestScore = score;
            victimSlot = i;
        }
    }

    return victimSlot;
}

Result ExtendedHeapAttemptLoadPage(u32 addr) {
    if (!ExtendedHeapIsAddressInHeap(addr)) {
        return -1;
    }

    const u32 pageStart = alignToPageStart(addr);
    const u32 pageIdx = GET_PAGE_INDEX(pageStart);

    if (pageIdx >= HeapCtx.maxPages) {
        return -1;
    }

    HeapCtx_mutex.lock();

    // 1. Check if the page is already mapped in any slot
    for (u16 i = 0; i < HeapCtx.maxLoadedPages; i++) {
        if (HeapCtx.mappedPages[i].mapped && HeapCtx.mappedPages[i].idx == pageIdx) {
            HeapCtx.mappedPages[i].lastAccess = ++g_lruCounter;
            HeapCtx_mutex.unlock();
            return 0;
        }
    }

    // 2. Increase miss counters safely
    HeapCtx.pageMissCount[pageIdx]++;
    if (pageIdx > 0) HeapCtx.pageMissCount[pageIdx - 1]++;
    if (pageIdx + 1 < HeapCtx.maxPages) HeapCtx.pageMissCount[pageIdx + 1]++;

    // 3. Select a victim slot using LRU (slot 0 is permanently pinned for TLSF control)
    const u16 minMappedIdx = getVictimPageSlot(pageIdx);
    PageMapInfo& pageInfo = HeapCtx.mappedPages[minMappedIdx];

    // 4. Save victim page to swapfile if it was mapped
    if (pageInfo.mapped) {
        svcFlushEntireDataCache();
        writePageContent(pageInfo.dstAddr);
        HeapCtx.pageWrittenToSwap[pageInfo.idx] = 1;

        #ifndef BUILD_AZAHAR
        Result status = svcControlProcessMemory(curHandle, pageInfo.dstAddr, pageInfo.srcAddr, PAGE_SIZE, MEMOP_UNMAP, MEMPERM_READWRITE);
        #else
        Result status = svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, pageInfo.dstAddr, PAGE_SIZE);
        svcCustomBackdoor((void*)invalidate_mmu_cache);
        #endif
        if (R_FAILED(status)) {
            LOGDEBUG("Failed to unmap memory page %08X, code: %08X", pageInfo.dstAddr, status);
            HeapCtx_mutex.unlock();
            return -1;
        }
        pageInfo.mapped = false;
    }

    svcCustomBackdoor((void*)invalidate_mmu_cache);

    // 5. Verify the destination virtual address is free
    MemInfo info;
    PageInfo out;
    Result status = svcQueryProcessMemory(&info, &out, CUR_PROCESS_HANDLE, pageStart);
    if (info.state != MemState::MEMSTATE_FREE) {
        LOGDEBUG("Failed to map memory at %08X. Expected to be free, instead state: %d", pageStart, info.state);
        HeapCtx_mutex.unlock();
        return -4;
    }

    // 6. Map the physical buffer to the new page address
    #ifndef BUILD_AZAHAR
    status = svcControlProcessMemory(curHandle, pageStart, pageInfo.srcAddr, PAGE_SIZE, MEMOP_MAP, MEMPERM_READWRITE);
    #else
    status = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, pageStart, CUR_PROCESS_HANDLE, pageInfo.srcAddr, PAGE_SIZE);
    svcCustomBackdoor((void*)invalidate_mmu_cache);
    #endif
    if (R_FAILED(status)) {
        LOGDEBUG("Failed to map memory page %08X, code: %08X", pageStart, status);
        HeapCtx_mutex.unlock();
        return -2;
    }

    pageInfo.mapped = true;
    pageInfo.dstAddr = pageStart;
    pageInfo.idx = pageIdx;
    pageInfo.lastAccess = ++g_lruCounter;

    svcCustomBackdoor((void*)invalidate_mmu_cache);
    CTRPluginFramework::ProcessImpl::UpdateMemRegions(true);

    // 7. Load page content if it was previously written, or zero-initialize if new
    if (HeapCtx.pageWrittenToSwap[pageIdx]) {
        loadPageContent(pageInfo.dstAddr);
    } else {
        memset((void*)pageInfo.dstAddr, 0, PAGE_SIZE);
    }

    svcFlushEntireDataCache();
    svcInvalidateEntireInstructionCache();

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

void ExtendedHeapExit(void) {
    HeapCtx_mutex.lock();
    if (HeapCtx.swapfile.isOpen()) {
        HeapCtx.swapfile.flush();
        HeapCtx.swapfile.close();
    }
    HeapCtx_mutex.unlock();
}
