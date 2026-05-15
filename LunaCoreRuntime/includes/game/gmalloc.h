#pragma once

#include "types.h"

#define _GameMemallocPTR ((void*(*)(size_t))(0x11493C))
#define _GameMemallocExPTR ((void *(*)(size_t, u32, u32, u32))0x11494C)
#define _GameFreePTR ((void(*)(void*))(0x1007D0|1))

#define GAME_HEAPTYPE_APP       0
#define GAME_HEAPTYPE_LINEAR    1

#define GameMemallocEx(size, type, alignment) _GameMemallocExPTR(size, type, alignment, 0)
#define GameMemsize(p) *(size_t*)((u32*)p - 3)
#define GameFree(p) _GameFreePTR(p)

#define gstd_malloc(size) GameMemallocEx(size, GAME_HEAPTYPE_APP, 8)
#define gstd_aligned_alloc(alignment, size) GameMemallocEx(size, GAME_HEAPTYPE_APP, alignment)
#define gstd_memalign(alignment, size) gstd_aligned_alloc(size, alignment)
#define gstd_free(p) GameFree(p)

#define glinearAlloc(size) GameMemallocEx(size, GAME_HEAPTYPE_LINEAR, 0x80)
#define glinearMemAlign(size, alignment) GameMemallocEx(size, GAME_HEAPTYPE_LINEAR, alignment)
#define glinearGetSize(p) GameMemsize(p)
#define glinearFree(p) GameFree(p)

#ifdef __cplusplus
extern "C" {
#endif

extern void* memset(void*, int, size_t);
extern void* memcpy(void*, const void*, size_t);

inline void* custom_gstd_calloc(size_t nitems, size_t size) {
    if (nitems > 0 && size > 0) {
        void* dstPtr = gstd_malloc(nitems * size);
        if (dstPtr) {
            memset(dstPtr, 0, nitems * size);
            return dstPtr;
        }
    }
    return NULL;
}

inline void* custom_gstd_realloc(void* p, size_t newsize) {
    if (p) {
        size_t oldsize = GameMemsize(p);

        if (oldsize == newsize)
            return p;

        void* newp = gstd_malloc(newsize);
        if (newp) {
            memcpy(newp, p, newsize < oldsize ? newsize : oldsize);
            gstd_free(p);
            return newp;
        }
    }
    return NULL;
}

inline void* custom_glinearRealloc(void* p, size_t newsize) {
    if (p) {
        size_t oldsize = glinearGetSize(p);

        if (oldsize == newsize)
            return p;

        void* newp = glinearAlloc(newsize);
        if (newp) {
            memcpy(newp, p, newsize < oldsize ? newsize : oldsize);
            glinearFree(p);
            return newp;
        }
    }
    return NULL;
}

#ifdef NOSTDLIB_BUILD
    void* malloc(size_t size);
    void* aligned_alloc(size_t alignment, size_t size);
    void* memalign(size_t alignment, size_t size);
    void* calloc(size_t nitems, size_t size);
    void* realloc(void* p, size_t newsize);
    void free(void* p);

    #ifdef GSTDLIB_MAIN
    void* malloc(size_t size) {
        return gstd_malloc(size);
    }

    void* aligned_alloc(size_t alignment, size_t size) {
        return gstd_aligned_alloc(alignment, size);
    }

    void* memalign(size_t alignment, size_t size) {
        return gstd_memalign(alignment, size);
    }

    void* calloc(size_t nitems, size_t size) {
        return custom_gstd_calloc(nitems, size);
    }

    void* realloc(void* p, size_t newsize) {
        return custom_gstd_realloc(p, newsize);
    }

    void free(void* p) {
        gstd_free(p);
    }
    #endif
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace gstd {
    inline void* malloc(size_t n) {
        return gstd_malloc(n);
    }

    inline void* aligned_alloc(size_t alignment, size_t size) {
        return gstd_aligned_alloc(size, alignment);
    }

    inline void* memalign(size_t alignment, size_t size) {
        return gstd_memalign(size, alignment);
    }

    inline void* calloc(size_t nitems, size_t size) {
        return custom_gstd_calloc(nitems, size);
    }

    inline void* realloc(void* p, size_t newsize) {
        return custom_gstd_realloc(p, newsize);
    }

    inline void free(void* p) {
        gstd_free(p);
    }
}
#endif