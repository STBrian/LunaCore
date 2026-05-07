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

    fslib::create_directory_recursively(path_from_string("sdmc:/Minecraft 3DS/LunaCore"));
    HeapCtx.swapfile.open(path_from_string("sdmc:/Minecraft 3DS/LunaCore/swapfile.bin"), FS_OPEN_CREATE|FS_OPEN_WRITE|FS_OPEN_READ, heapsize_b);
    if (!HeapCtx.swapfile.is_open()) return -1;
    size_t maxLoadedPages = heapsize < MAX_LOADED_PAGES ? heapsize : MAX_LOADED_PAGES;

    HeapCtx.heapStart = START_EXTHEAP_ADDR;
    HeapCtx.heapEnd = HeapCtx.heapStart + heapsize_b;
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
        HeapCtx.mappedPages[i].srcAddr = (u32)aligned_alloc(0x1000, PAGE_SIZE);
        if (HeapCtx.mappedPages[i].srcAddr == 0) goto error;
        HeapCtx.mappedPages[i].idx = i;
        HeapCtx.mappedPages[i].dstAddr = START_EXTHEAP_ADDR + PAGE_SIZE * i;
    }

    // Map pages
    for (size_t i = 0; i < maxLoadedPages; i++) {
        Result res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, HeapCtx.mappedPages[i].dstAddr, CUR_PROCESS_HANDLE, HeapCtx.mappedPages[i].srcAddr, PAGE_SIZE);
        if (R_FAILED(res)) {
            char buffer[0x100];
            snprintf(buffer, sizeof(buffer), "Failed to map to %08X", HeapCtx.mappedPages[i].dstAddr);
            Core::Abort(buffer);
        } else 
            HeapCtx.mappedPages[i].mapped = true;
    }

    HeapCtx.allocator = tlsf_create_with_pool((void*)START_EXTHEAP_ADDR, heapsize_b);
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

void ExtendedHeapAttemptLoadPage(u32 addr) {
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
    writePageContent(&pageInfo);
    svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, pageInfo.dstAddr, PAGE_SIZE);
    pageInfo.dstAddr = pageStart;
    pageInfo.idx = pageIdx;
    loadPageContent(&pageInfo);
    svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, pageInfo.dstAddr, CUR_PROCESS_HANDLE, pageInfo.srcAddr, PAGE_SIZE);
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