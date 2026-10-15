#ifndef _CONF_H_
#define _CONF_H_

/*
 * Legacy Makefile builds generated this header at include/generated/autoconf.h.
 * SDK 3.0 force-includes sdkconfig.h instead, so keep the old include optional.
 */
#if defined(__has_include)
#if __has_include("generated/autoconf.h")
#include "generated/autoconf.h"
#endif
#endif

#define EXEC_WAIT_POLICY (resume_cnt <= suspend_cnt)

#if !defined(CONFIG_COMPONENT_COROUTINE) || !CONFIG_COMPONENT_COROUTINE

#define USE_IDLE_TASK
#define MAX_TASK_NUM 10

#define USE_CTX
#define TASK_CTX_SIZE 64

#else

#if defined(CONFIG_COMPONENT_COROUTINE_USE_IDLE_TASK) && CONFIG_COMPONENT_COROUTINE_USE_IDLE_TASK
#define USE_IDLE_TASK
#endif

#ifdef CONFIG_COMPONENT_COROUTINE_MAX_TASK_NUM
#define MAX_TASK_NUM CONFIG_COMPONENT_COROUTINE_MAX_TASK_NUM
#endif

#if defined(CONFIG_COMPONENT_COROUTINE_USE_CTX) && CONFIG_COMPONENT_COROUTINE_USE_CTX
#define USE_CTX
#endif

#ifdef CONFIG_COMPONENT_COROUTINE_TASK_CTX_SIZE
#define TASK_CTX_SIZE CONFIG_COMPONENT_COROUTINE_TASK_CTX_SIZE
#endif

#endif

#endif
