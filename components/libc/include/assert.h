#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <stdint.h>
#include "stdio.h"
#include "generated/autoconf.h"

#ifndef CONFIG_BUILD_DEBUG

#define assert(expr, fmt, ...) ((void)0)

#else


#include "ecos/log.h"
#define assert(expr, fmt, ...) \
    ({ \
        typeof(expr) _assert_result = (expr); \
        if (!(_assert_result)) { \
            ECOS_PANIC("assert", fmt, ##__VA_ARGS__); \
        } \
        _assert_result; \
    })
#endif

#endif
