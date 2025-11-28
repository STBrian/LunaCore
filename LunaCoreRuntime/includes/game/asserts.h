#pragma once

#ifndef __cplusplus
#define ASSERT_SIZE(type, size) \
    _Static_assert(sizeof(type) == size, "type '" #type "' must have size " #size)

#else
#define ASSERT_SIZE(type, size) \
    static_assert(sizeof(type) == size, "type '" #type "' must have size " #size)

#endif