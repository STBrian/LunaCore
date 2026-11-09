#pragma once

#include <stdint.h>

uint32_t StringUtils_hash(const char* str) {
    return ((uint32_t(*)(const char*))(0x0012a814))(str);
}