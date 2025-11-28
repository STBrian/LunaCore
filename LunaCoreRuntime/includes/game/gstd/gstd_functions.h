#pragma once

#include "game/types.h"

#define GameMemalloc ((void*(*)(size_t))(0x11493c))
#define GameStrlen ((size_t(*)(const char*))(0x2fe990))

inline void *GameCalloc(size_t nitems, size_t size) {
    void *dstPtr = GameMemalloc(nitems * size);
    if (dstPtr == NULL)
        return NULL;
    for (int i = 0; i < nitems * size; i++)
        *reinterpret_cast<char*>((char*)dstPtr+i) = 0;
    return dstPtr;
}

#define GameSnprintf ((size_t(*)(char*,size_t,char*,...))(0x1000c0|1))