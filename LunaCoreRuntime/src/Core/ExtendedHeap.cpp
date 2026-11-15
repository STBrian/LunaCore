#include "ExtendedHeap.hpp"
#include "CoreConstants.hpp"
#include "Core/Filesystem.hpp"
#include <CTRPluginFramework.hpp>
#include "Core/Debug.hpp"
#include "Core/CrashHandler.hpp"

#include "csvc.h"
#include "tlsf.h"
#include "Helpers/Mutex.hpp"

#define PAGE_SIZE (0x1000) // 4KB (1 physical pages)
#define START_EXTHEAP_ADDR 0x02000000
#define MAX_LOADED_PAGES 50 // 96 * 4KB = 384KB (well within the 1024KB free CTRPF heap)

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
    u32* pageMissCount;
    u8* pageWrittenToSwap;
    u16 maxLoadedPages;
    u32 maxPages;
    u32 heapStart;
    u32 heapEnd;
    tlsf_t allocator;
} ExtHeapCtx;

static ExtHeapCtx HeapCtx;
static bool isEmulator = false;
static Handle curHandle = 0;
Core::Mutex HeapCtx_mutex;
static u32 g_lruCounter = 1000;

static u32 alignToPageStart(u32 addr) {
    return addr - (addr & (PAGE_SIZE - 1));
}

#define GET_PAGE_INDEX(pageaddr) ((u32)(((u32)pageaddr - START_EXTHEAP_ADDR) / PAGE_SIZE))

static inline void ctr_dsb(void) {
    asm volatile("mcr p15, 0, %0, c7, c10, 4" :: "r" (0) : "memory");
}

static inline void ctr_isb(void) {
    asm volatile("mcr p15, 0, %0, c7, c5, 4" :: "r" (0) : "memory");
}

static Result mapPageMemory(u32 dst, u32 src) {
    if (isEmulator) {
        return svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, dst, CUR_PROCESS_HANDLE, src, PAGE_SIZE);
    } else {
        return svcControlProcessMemory(curHandle, dst, src, PAGE_SIZE, MEMOP_MAP, MEMPERM_READWRITE);
    }
}

static Result unmapPageMemory(u32 dst, u32 src) {
    if (isEmulator) {
        return svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, dst, PAGE_SIZE);
    } else {
        return svcControlProcessMemory(curHandle, dst, src, PAGE_SIZE, MEMOP_UNMAP, MEMPERM_READWRITE);
    }
}

namespace CTRPluginFramework::ProcessImpl {
    void ReturnFromException(CpuRegisters* regs);
}

typedef void (*CTRPF_ExceptionHandler)(ERRF_ExceptionInfo* excep, CpuRegisters* regs);
static CTRPF_ExceptionHandler g_originalExceptionHandler = nullptr;

static void ExtendedHeapFastExceptionHandler(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
    if (excep != nullptr && regs != nullptr && excep->type == ERRF_EXCEPTION_DATA_ABORT) {
        u32 status = (excep->fsr & 0xF) | ((excep->fsr >> 6) & 0x10);
        if ((status == 0x05 || status == 0x07) && ExtendedHeapIsAddressInHeap(excep->far)) {
            #ifdef DEBUG
            Core::Debug::LogInfof("Page fault reached at: %08X", excep->far);
            #endif
            Result res = ExtendedHeapAttemptLoadPage(excep->far);
            if (R_SUCCEEDED(res)) {
                #ifdef DEBUG
                Core::Debug::LogInfof("[DEBUG] Resuming to PC: %08X", regs->pc);
                #endif
                CTRPluginFramework::ProcessImpl::ReturnFromException(regs);
            }
            Core::Debug::LogErrorf("ExtendedHeap: Page swap failed for address %08X, code: %08X", excep->far, res);
        }
    }

    if (g_originalExceptionHandler != nullptr) {
        g_originalExceptionHandler(excep, regs);
    }
}

static const char* CrashHandlerExtendedHeapCallback(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
    if (excep != nullptr && regs != nullptr && excep->type == ERRF_EXCEPTION_DATA_ABORT) {
        if (ExtendedHeapIsAddressInHeap(excep->far)) {
            return "ExtendedHeap: Page swap failure or out of memory";
        }
    }
    return nullptr;
}

Result ExtendedHeapInit(size_t heapsize) {
    const u32 heapsize_b = heapsize * PAGE_SIZE;
    u16 maxLoadedPages = heapsize < MAX_LOADED_PAGES ? (u16)heapsize : (u16)MAX_LOADED_PAGES;
    maxLoadedPages &= ~1; // Ensure an even number of slots for balanced page coloring

    isEmulator = CTRPluginFramework::System::IsCitra();
    if (!isEmulator) {
        Result res = svcDuplicateHandle(&curHandle, CUR_PROCESS_HANDLE);
        if (R_FAILED(res)) {
            Core::Debug::LogErrorf("ExtendedHeap: Failed to duplicate process handle: %08X", res);
            return -1;
        }
    }

    Core::Debug::LogInfof("ExtendedHeap: Initializing (target: %u KB, max loaded: %u KB, isEmulator: %d). Available CTRPF heap: %u KB",
        (u32)(heapsize_b / 1024), (u32)(maxLoadedPages * 4), (int)isEmulator, CTRPluginFramework::Heap::SpaceFree() / 1024);

    if (!Core::Filesystem::DirectoryExists("lcfs:/LunaCore")) {
        Core::Filesystem::CreateDirectory("lcfs:/LunaCore");
    }
    CTRPF::Directory::Create(PLUGIN_FOLDER "/LunaCore");

    // Open swapfile: try in LunaCore subdirectory, fall back to root of lcfs if needed
    HeapCtx.swapfile.open("lcfs:/LunaCore/swapfile.bin", FS_OPEN_CREATE|FS_OPEN_READ|FS_OPEN_WRITE, heapsize_b);
    if (!HeapCtx.swapfile.isOpen()) {
        Core::Debug::LogWarnf("ExtendedHeap: Failed to open swapfile at 'lcfs:/LunaCore/swapfile.bin' (status: %d). Falling back to 'lcfs:/swapfile.bin'", (int)HeapCtx.swapfile.getStatus());
        HeapCtx.swapfile.open("lcfs:/swapfile.bin", FS_OPEN_CREATE|FS_OPEN_READ|FS_OPEN_WRITE, heapsize_b);
    }
    if (!HeapCtx.swapfile.isOpen()) {
        Core::Debug::LogErrorf("ExtendedHeap: Failed to open swapfile (status: %d)", (int)HeapCtx.swapfile.getStatus());
        return -1;
    }

    HeapCtx.heapStart = START_EXTHEAP_ADDR;
    HeapCtx.heapEnd = START_EXTHEAP_ADDR + heapsize_b;
    HeapCtx.maxPages = heapsize;
    HeapCtx.maxLoadedPages = maxLoadedPages;
    
    PageMapInfo* mappedPages = (PageMapInfo*)calloc(sizeof(PageMapInfo), maxLoadedPages);
    u32* pageMissCount = (u32*)calloc(sizeof(u32), heapsize);
    u8* pageWrittenToSwap = (u8*)calloc(sizeof(u8), heapsize);

    if (mappedPages == nullptr || pageMissCount == nullptr || pageWrittenToSwap == nullptr) {
        Core::Debug::LogError("ExtendedHeap: Failed to allocate management buffers");
        goto error;
    }

    HeapCtx.mappedPages = mappedPages;
    HeapCtx.pageMissCount = pageMissCount;
    HeapCtx.pageWrittenToSwap = pageWrittenToSwap;
    
    // Allocate physical page buffers in 8KB-aligned pairs so even slots have bit 12 = 0
    // and odd slots have bit 12 = 1, exactly matching their virtual address page colors.
    for (size_t i = 0; i < maxLoadedPages; i += 2) {
        void* pairBuffer = aligned_alloc(0x2000, PAGE_SIZE * 2);
        if (pairBuffer == nullptr) {
            Core::Debug::LogErrorf("ExtendedHeap: Failed to allocate physical page pair %u", (u32)i);
            goto error;
        }
        mappedPages[i].srcAddr = (u32)pairBuffer;
        mappedPages[i + 1].srcAddr = (u32)pairBuffer + PAGE_SIZE;
    }

    // Initialize pages and assign slots respecting page coloring (slot % 2 == pageIdx % 2)
    for (size_t i = 0; i < maxLoadedPages; i++) {
        // Map sentinel tail page to a matching-color slot near the end
        u32 initialPageIdx = (u32)i;
        if (heapsize > maxLoadedPages) {
            u32 tailPage = (u32)(heapsize - 1);
            if ((tailPage & 1) == 1 && i == (size_t)(maxLoadedPages - 1)) {
                initialPageIdx = tailPage;
            } else if ((tailPage & 1) == 0 && i == (size_t)(maxLoadedPages - 2)) {
                initialPageIdx = tailPage;
            }
        }

        mappedPages[i].idx = initialPageIdx;
        mappedPages[i].dstAddr = START_EXTHEAP_ADDR + PAGE_SIZE * initialPageIdx;
        // Tail page gets lastAccess = 1 so it is chosen first when swaps begin
        mappedPages[i].lastAccess = (initialPageIdx >= maxLoadedPages) ? 1 : (u32)(100 + i);
        mappedPages[i].mapped = false;
    }
    g_lruCounter = maxLoadedPages + 100;

    // Map initial pages
    LOGDEBUG("Mapping initial pages");
    for (size_t i = 0; i < maxLoadedPages; i++) {
        LOGDEBUG("Mirroring %d (page %u)", (int)i, mappedPages[i].idx);
        Result res = mapPageMemory(mappedPages[i].dstAddr, mappedPages[i].srcAddr);
        if (R_FAILED(res)) {
            Core::Debug::LogErrorf("ExtendedHeap: Failed to mirror page %u (%08X -> %08X), res: %08X", (u32)i, mappedPages[i].srcAddr, mappedPages[i].dstAddr, res);
            goto error;
        } else {
            mappedPages[i].mapped = true;
            memset((void*)mappedPages[i].dstAddr, 0, PAGE_SIZE);
        }
    }

    LOGDEBUG("Creating allocator");
    Core::CrashHandler::SetCallback(CrashHandlerExtendedHeapCallback);

    // Install fast page fault exception handler at TLS level to avoid CTRPF GSP/threading side effects
    {
        u32 mainTls = CTRPluginFramework::Process::GetMainThreadTLS();
        if (mainTls != 0) {
            u32* handlerSlot = (u32*)(mainTls + 0x40);
            if (*handlerSlot != (u32)ExtendedHeapFastExceptionHandler) {
                g_originalExceptionHandler = (CTRPF_ExceptionHandler)(*handlerSlot);
                *handlerSlot = (u32)ExtendedHeapFastExceptionHandler;
                *(u32*)(mainTls + 0x44) = 1;
                *(u32*)(mainTls + 0x48) = 1;
            }
        }
        u32 curTls = (u32)getThreadLocalStorage();
        if (curTls != 0) {
            u32* curHandlerSlot = (u32*)(curTls + 0x40);
            if (*curHandlerSlot != (u32)ExtendedHeapFastExceptionHandler) {
                if (g_originalExceptionHandler == nullptr && *curHandlerSlot != 0) {
                    g_originalExceptionHandler = (CTRPF_ExceptionHandler)(*curHandlerSlot);
                }
                *curHandlerSlot = (u32)ExtendedHeapFastExceptionHandler;
                *(u32*)(curTls + 0x44) = 1;
                *(u32*)(curTls + 0x48) = 1;
            }
        }
        ctr_dsb();
        ctr_isb();
    }

    HeapCtx.allocator = tlsf_create_with_pool((void*)START_EXTHEAP_ADDR, heapsize_b);
    if (!HeapCtx.allocator) {
        Core::Debug::LogError("ExtendedHeap: Failed to create tlsf allocator");
        goto error;
    }

    {
        LOGDEBUG("Test malloc");
        void* test = ExtendedHeapMalloc(100);
        if (test == NULL) {
            Core::Debug::LogError("ExtendedHeap: Self-test malloc failed");
            goto error;
        } else {
            LOGDEBUG("Test free");
            ExtendedHeapFree(test);
        }
    }
    Core::Debug::LogInfo("ExtendedHeap: Initialized successfully");
    return 0;

    error:
    Core::Debug::LogError("ExtendedHeap: Failed to init ExtendedHeap. Cleaning up...");
    ctr_dsb();
    ctr_isb();
    HeapCtx.swapfile.close();
    if (pageMissCount) free(pageMissCount);
    if (pageWrittenToSwap) free(pageWrittenToSwap);
    if (mappedPages) {
        // Free blocks if any
        for (size_t i = 0; i < maxLoadedPages; i++) {
            if (mappedPages[i].mapped) unmapPageMemory(mappedPages[i].dstAddr, mappedPages[i].srcAddr);
        }
        for (size_t i = 0; i < maxLoadedPages; i += 2) {
            if (mappedPages[i].srcAddr != 0) free((void*)mappedPages[i].srcAddr);
        }
        free(mappedPages);
    }
    if (!isEmulator && curHandle != 0) {
        svcCloseHandle(curHandle);
        curHandle = 0;
    }
    ctr_dsb();
    ctr_isb();
    return -1;
}

bool ExtendedHeapIsAddressInHeap(u32 addr) {
    return (addr >= HeapCtx.heapStart && addr < HeapCtx.heapEnd);
}

static void writePageContent(u32 addr) {
    const size_t fileOffset = addr - START_EXTHEAP_ADDR;
    HeapCtx.swapfile.seek(fileOffset, SEEK_SET);
    int status = HeapCtx.swapfile.write((void*)addr, PAGE_SIZE);
    if (status != PAGE_SIZE) Core::Abort(CTRPF::Utils::Format("Failed to write page to swapfile from %08X! %08X", addr, status).c_str());
    HeapCtx.swapfile.flush();
}

static void loadPageContent(u32 addr) {
    const size_t fileOffset = addr - START_EXTHEAP_ADDR;
    HeapCtx.swapfile.seek(fileOffset, SEEK_SET);
    int status = HeapCtx.swapfile.read((void*)addr, PAGE_SIZE);
    if (status != PAGE_SIZE) Core::Abort(CTRPF::Utils::Format("Failed to read swap page @ %08X! %08X", addr, status).c_str());
}

static u16 getVictimPageSlot(u32 incomingPageIdx) {
    if (HeapCtx.maxLoadedPages <= 1) {
        return 0;
    }

    const u16 color = (u16)(incomingPageIdx & 1);
    const u16 startSlot = (color == 0 ? 2 : 1); // Slot 0 is permanently pinned for TLSF control

    // 1. If any slot matching this color is currently unmapped, use it immediately
    for (u16 i = startSlot; i < HeapCtx.maxLoadedPages; i += 2) {
        if (!HeapCtx.mappedPages[i].mapped) {
            return i;
        }
    }

    // 2. Select victim using pageMissCount and LRU among slots with matching color
    u64 bestScore = 0xFFFFFFFFFFFFFFFFULL;
    u16 victimSlot = startSlot;

    for (u16 i = startSlot; i < HeapCtx.maxLoadedPages; i += 2) {
        if (HeapCtx.mappedPages[i].idx == incomingPageIdx) {
            continue;
        }

        const u32 pIdx = HeapCtx.mappedPages[i].idx;
        if (pIdx == 0) {
            continue; // Slot 0 is pinned
        }

        // Protect foundation pages (0-3: TLSF control metadata and initial root Lua state)
        const u64 foundationPenalty = (pIdx < 4) ? 0x100000000000ULL : 0ULL;

        // Anti-thrashing recency window: protect pages loaded very recently
        const u32 age = (u32)(g_lruCounter - HeapCtx.mappedPages[i].lastAccess);
        const u64 recencyPenalty = (age < 4) ? 0x010000000000ULL : 0ULL;

        // Scoring: Lower score = candidate for eviction.
        // High pageMissCount (hot pages) -> high score -> retained in memory!
        // Low pageMissCount (cold pages) -> low score -> chosen for eviction first!
        // LRU lastAccess acts as fine-grained tie-breaker for pages with identical miss frequency.
        const u64 missWeight = 100000ULL;
        const u64 score = foundationPenalty + recencyPenalty + 
                          ((u64)HeapCtx.pageMissCount[pIdx] * missWeight) + 
                          (u64)HeapCtx.mappedPages[i].lastAccess;

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

    // Periodic aging / decay of miss counts to keep them relevant to the current phase
    if ((g_lruCounter & 0x1FF) == 0) {
        for (u32 p = 0; p < HeapCtx.maxPages; p++) {
            HeapCtx.pageMissCount[p] >>= 1;
        }
    }

    // 3. Select a victim slot matching the incoming page's color
    const u16 minMappedIdx = getVictimPageSlot(pageIdx);
    PageMapInfo& pageInfo = HeapCtx.mappedPages[minMappedIdx];

    // 4. Save victim page to swapfile if it was mapped
    if (pageInfo.mapped) {
        // Clean dirty cache lines to physical RAM and invalidate them before SDMMC DMA write
        svcFlushProcessDataCache(CUR_PROCESS_HANDLE, pageInfo.dstAddr, PAGE_SIZE);
        ctr_dsb();

        writePageContent(pageInfo.dstAddr);
        HeapCtx.pageWrittenToSwap[pageInfo.idx] = 1;

        Result status = unmapPageMemory(pageInfo.dstAddr, pageInfo.srcAddr);
        if (R_FAILED(status)) {
            Core::Debug::LogErrorf("Failed to unmap memory page %08X, code: %08X", pageInfo.dstAddr, status);
            HeapCtx_mutex.unlock();
            return -1;
        }
        pageInfo.mapped = false;
    }

    ctr_dsb();
    ctr_isb();

    // 5. Verify the destination virtual address is free
    MemInfo info;
    PageInfo out;
    Result status = svcQueryProcessMemory(&info, &out, CUR_PROCESS_HANDLE, pageStart);
    if (info.state != MemState::MEMSTATE_FREE) {
        Core::Debug::LogErrorf("Failed to map memory at %08X. Expected to be free, instead state: %d", pageStart, info.state);
        HeapCtx_mutex.unlock();
        return -4;
    }

    // 6. Map the physical buffer to the new page address FIRST
    status = mapPageMemory(pageStart, pageInfo.srcAddr);
    if (R_FAILED(status)) {
        Core::Debug::LogErrorf("Failed to map memory page %08X, code: %08X", pageStart, status);
        HeapCtx_mutex.unlock();
        return -2;
    }

    pageInfo.mapped = true;
    pageInfo.dstAddr = pageStart;
    pageInfo.idx = pageIdx;
    pageInfo.lastAccess = ++g_lruCounter;

    ctr_dsb();
    ctr_isb();

    // 7. Now that pageStart is mapped in application memory, load content into it or zero-initialize if new
    if (HeapCtx.pageWrittenToSwap[pageIdx]) {
        // Invalidate D-cache before SDMMC DMA read so no stale/dirty lines exist
        svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, pageStart, PAGE_SIZE);
        ctr_dsb();

        loadPageContent(pageStart);

        // Invalidate D-cache and I-cache after SDMMC DMA read so CPU fetches freshly read data from RAM.
        // DO NOT FLUSH/CLEAN! A clean would overwrite RAM with stale cache data!
        svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, pageStart, PAGE_SIZE);
        svcInvalidateEntireInstructionCache();
        ctr_dsb();
        ctr_isb();
    } else {
        memset((void*)pageStart, 0, PAGE_SIZE);
        ctr_dsb();
    }

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
    if (g_originalExceptionHandler != nullptr) {
        u32 mainTls = CTRPluginFramework::Process::GetMainThreadTLS();
        if (mainTls != 0) {
            *(u32*)(mainTls + 0x40) = (u32)g_originalExceptionHandler;
        }
        u32 curTls = (u32)getThreadLocalStorage();
        if (curTls != 0) {
            *(u32*)(curTls + 0x40) = (u32)g_originalExceptionHandler;
        }
        ctr_dsb();
        ctr_isb();
        g_originalExceptionHandler = nullptr;
    }
    if (HeapCtx.swapfile.isOpen()) {
        HeapCtx.swapfile.flush();
        HeapCtx.swapfile.close();
    }
    if (!isEmulator && curHandle != 0) {
        svcCloseHandle(curHandle);
        curHandle = 0;
    }
    HeapCtx_mutex.unlock();
}
