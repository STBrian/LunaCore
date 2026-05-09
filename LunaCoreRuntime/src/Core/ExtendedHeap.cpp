#include "ExtendedHeap.hpp"
#include <FsLib/fslib.hpp>
#include "Core/Filesystem.hpp"
#include "Core/Debug.hpp"

#include "csvc.h"
#include "tlsf.h"

#define PAGE_SIZE (0x1000) // 4kb

typedef struct {
    bool mapped;
    u32 idx;
    u32 srcAddr;
    u32 dstAddr;
} PageMapInfo;

typedef struct {
    fslib::File swapfile;
    PageMapInfo* mappedPages;
    u16* pageMissCount;
    u32 maxPages;
    u32 maxLoadedPages;
    u32 heapStart;
    u32 heapEnd;
    tlsf_t allocator;

} ExtHeapCtx;

#define START_EXTHEAP_ADDR 0x20000000
#define MAX_LOADED_PAGES 128

static ExtHeapCtx HeapCtx;

static u32 alignToPageStart(u32 addr) {
    return addr - (addr & (PAGE_SIZE - 1));
}

#define GET_PAGE_INDEX(pageaddr) ((u32)(((u32)pageaddr - START_EXTHEAP_ADDR) / PAGE_SIZE))

Result ExtendedHeapInit(size_t heapsize) {
    size_t heapsize_b = heapsize * PAGE_SIZE;

    if (!fslib::directory_exists(path_from_string("sdmc:/Minecraft 3DS/LunaCore"))) {
        if (!fslib::create_directory(path_from_string("sdmc:/Minecraft 3DS/LunaCore"))) return -1;
    }
    HeapCtx.swapfile.open(path_from_string("sdmc:/Minecraft 3DS/LunaCore/swapfile.bin"), FS_OPEN_CREATE|FS_OPEN_WRITE|FS_OPEN_READ, heapsize_b);
    if (!HeapCtx.swapfile.is_open()) return -1;
    size_t maxLoadedPages = heapsize < MAX_LOADED_PAGES ? heapsize : MAX_LOADED_PAGES;

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
    for (size_t i = 0; i < maxLoadedPages; i++) {
        Result res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, mappedPages[i].dstAddr, CUR_PROCESS_HANDLE, mappedPages[i].srcAddr, PAGE_SIZE, MAPEXFLAGS_SHARED);
        if (R_FAILED(res)) {
            LOGDEBUG("Failed to map to %08X", mappedPages[i].dstAddr);
            goto error;
        } else 
            mappedPages[i].mapped = true;
    }

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
    HeapCtx.swapfile.close();
    if (mappedPages) free(mappedPages);
    if (pageMissCount) free(pageMissCount);
    // Free blocks if any
    for (size_t i = 0; i < maxLoadedPages; i++) {
        if (HeapCtx.mappedPages[i].mapped) svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, HeapCtx.mappedPages[i].dstAddr, PAGE_SIZE);
        if (HeapCtx.mappedPages[i].srcAddr != 0) free((void*)HeapCtx.mappedPages[i].srcAddr);
    }
    return -1;
}

bool ExtendedHeapIsAddressInHeap(u32 addr) {
    return (addr >= HeapCtx.heapStart && addr < HeapCtx.heapEnd);
}

static void writePageContent(PageMapInfo* info) {
    size_t fileOffset = info->dstAddr - START_EXTHEAP_ADDR;
    HeapCtx.swapfile.seek(fileOffset, fslib::File::BEGINNING);
    ssize_t res = HeapCtx.swapfile.write((void*)info->srcAddr, PAGE_SIZE);
    if (res != PAGE_SIZE) Core::Abort("Failed to write page to swapfile");
}

static void loadPageContent(PageMapInfo* info) {
    size_t fileOffset = info->dstAddr - START_EXTHEAP_ADDR;
    HeapCtx.swapfile.seek(fileOffset, fslib::File::BEGINNING);
    ssize_t res = HeapCtx.swapfile.read((void*)info->srcAddr, PAGE_SIZE);
    if (res != PAGE_SIZE) Core::Abort("Failed to load page from swapfile");
}

static void swapfileFlush() {
    HeapCtx.swapfile.flush();
}

Result ExtendedHeapAttemptLoadPage(u32 addr) {
    MemInfo info;
    PageInfo out;
    Result status;
    const u32 pageStart = alignToPageStart(addr);
    const u32 pageIdx = GET_PAGE_INDEX(pageStart);

    HeapCtx.pageMissCount[pageIdx]++;
    if (pageIdx > 0) HeapCtx.pageMissCount[pageIdx-1]++;
    if (pageIdx < HeapCtx.maxPages) HeapCtx.pageMissCount[pageIdx+1]++;

    // Search page with the minimum number of hit misses
    u16 minMappedIdx = 0;
    u16 minHitMiss = HeapCtx.pageMissCount[HeapCtx.mappedPages[0].idx];
    u32 maxLoadedPages = HeapCtx.maxLoadedPages;
    for (size_t i = 1; i < maxLoadedPages; i++) {
        const u16 curIdx = HeapCtx.mappedPages[i].idx;
        if (HeapCtx.pageMissCount[curIdx] < minHitMiss) {
            minHitMiss = HeapCtx.pageMissCount[curIdx];
            minMappedIdx = i;
        }
    }
    // Save old and load new
    PageMapInfo& pageInfo = HeapCtx.mappedPages[minMappedIdx];
    //LOGDEBUG("Swaping memory page %08X", pageInfo.dstAddr);
    svcFlushEntireDataCache();
    writePageContent(&pageInfo);
    svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, pageInfo.dstAddr, PAGE_SIZE);
    status = svcQueryProcessMemory(&info, &out, CUR_PROCESS_HANDLE, pageInfo.dstAddr);
    if (R_FAILED(status)) return -3;
    if (info.state == MemState::MEMSTATE_FREE) {
        LOGDEBUG("Attempt to unmap memory at %08X but it isn't mapped");
        return -4;
    }
    u32 addr_out;
    // Breaks memory
    status = svcControlMemoryUnsafe(&addr_out, pageInfo.dstAddr, PAGE_SIZE, MemOp::MEMOP_FREE, (MemPerm)0);
    if (R_FAILED(status)) {
        LOGDEBUG("Failed to unmap memory page");
        LOGDEBUG("Unmap status code: %08X", status);
        return -1;
    }
    status = svcQueryProcessMemory(&info, &out, CUR_PROCESS_HANDLE, pageInfo.dstAddr);
    if (R_FAILED(status)) return -3;
    if (info.state != MemState::MEMSTATE_FREE) {
        LOGDEBUG("Failed to unmap memory at %08X. Expected to be free, instead: %d", pageInfo.dstAddr, info.state);
        LOGDEBUG("Info: %d, %08X, %d", info.size, info.base_addr, info.perm);
        LOGDEBUG("Info: %d", out.flags);
        return -4;
    }
    __dsb();
    __isb();
    __dmb();
    swapfileFlush();
    pageInfo.dstAddr = pageStart;
    pageInfo.idx = pageIdx;
    //LOGDEBUG("Loading memory page %08X", pageInfo.dstAddr);
    loadPageContent(&pageInfo);
    status = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, pageInfo.dstAddr, CUR_PROCESS_HANDLE, pageInfo.srcAddr, PAGE_SIZE, MAPEXFLAGS_PRIVATE);
    if (R_FAILED(status)) {
        LOGDEBUG("Failed to map memory page");
        return -2;
    }
    svcInvalidateEntireInstructionCache();
    svcFlushEntireDataCache();
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