#pragma once

#include <cstdint>

typedef uint32_t shash;

constexpr shash hash(const char *str)
{
    shash hash_ = 0;
    while (*str)
    {
        hash_ += *str;
        hash_ &= 0xFFFFFFFF;
        hash_ += (hash_ << 10);
        hash_ &= 0xFFFFFFFF;
        hash_ ^= (hash_ >> 6);
        str++;
    }
    hash_ += (hash_ << 3);
    hash_ &= 0xFFFFFFFF;
    hash_ ^= (hash_ >> 11);
    hash_ += (hash_ << 15);
    return hash_ & 0xFFFFFFFF;
}