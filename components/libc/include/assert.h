#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <stdint.h>
#include "stdio.h"


#define USE_TIMMOLOG 1

#ifndef CONFIG_BUILD_DEBUG
    #define assert(expr, info) ((void)0)
#else
    #define assert(expr, info) \
        ({ \
            typeof(expr) result = (expr); \
            if (!(expr)) { \
#ifdef USE_TIMMOLOG
                printf("(%s:%d) %s", __FILE__, __LINE__,(info)); \
#else
                log_warn("%s",(info)); \
#endif
            } \
            result; \
        })
#endif

#endif
