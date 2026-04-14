#pragma once

#if defined(__cplusplus)
#   define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#   define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

#define STATIC_ASSERT_SIZE(type, expected) \
    STATIC_ASSERT(sizeof(type) == (expected), #type " size mismatch")
