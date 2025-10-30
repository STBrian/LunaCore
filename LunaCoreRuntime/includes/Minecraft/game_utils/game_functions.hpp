#pragma once

#include "types.h"

#define GameMemalloc ((void*(*)(size_t))(0x11493c))
#define GameStrlen ((size_t(*)(const char*))(0x2fe990))

inline void *GameCalloc(size_t size) {
    void *dstPtr = GameMemalloc(size);
    if (dstPtr == NULL)
        return NULL;
    for (int i = 0; i < size; i++)
        *reinterpret_cast<char*>((char*)dstPtr+i) = 0;
    return dstPtr;
}

#define GameSnprintf ((size_t(*)(char*,size_t,char*,...))(0x1000c0|1))