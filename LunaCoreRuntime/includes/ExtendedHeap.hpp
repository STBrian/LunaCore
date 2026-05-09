#pragma once

#include "types.h"

Result ExtendedHeapInit(size_t heapsize);

bool ExtendedHeapIsAddressInHeap(u32 addr);

Result ExtendedHeapAttemptLoadPage(u32 addr);

void* ExtendedHeapMalloc(size_t size);

void* ExtendedHeapRealloc(void* ptr, size_t size);

void ExtendedHeapFree(void* p);