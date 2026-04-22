#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <stdint.h>
#include "stdio.h"

#ifndef CONFIG_BUILD_DEBUG
    #define assert(expr, info) ((void)0)
#else
    #define assert(expr, info) \
        ({ \
            typeof(expr) result = (expr); \
            if (!(expr)) { \
                printf("(%s:%d) %s", __FILE__, __LINE__,(info)); \
            } \
            result; \
        })
#endif

#endif
