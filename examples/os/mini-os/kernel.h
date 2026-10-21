#ifndef MINI_OS_KERNEL_H
#define MINI_OS_KERNEL_H

#include <stdint.h>

#include "coroutine.h"

/* Smallest useful kernel image: the task table is static so no heap is needed. */
#define OS_MAX_TASKS       8
#define OS_TICK_PERIOD_US  1000u

#if OS_TICK_PERIOD_US > 1000u || (1000u % OS_TICK_PERIOD_US) != 0u
#error "OS_TICK_PERIOD_US must divide 1000 us to keep 32-bit tick math"
#endif

/* 32-bit safe conversion, no 64-bit divide helper needed on rv32e. */
#define OS_TICKS_TO_MS(ticks) \
    ((uint32_t)(ticks) / (1000u / OS_TICK_PERIOD_US))

typedef enum {
    OS_TASK_FREE = 0,
    OS_TASK_READY,
    OS_TASK_RUNNING,
    OS_TASK_BLOCKED,
    OS_TASK_SUSPENDED
} os_task_state_t;

typedef struct {
    task_t coro;            /* LightCoroutine task context, owned by the kernel */
    const char *name;
    func_t entry;
    os_task_state_t state;
    uint32_t wake_tick;     /* tick at which a sleeping task becomes runnable */
    uint32_t cpu_ticks;     /* ticks spent inside the task body */
    uint32_t switches;
    uint8_t used;
} os_task_t;

/* Initialise the tick source and the task table. Returns an ECOS error code. */
int os_kernel_init(void);

/* Create a cooperative task. Returns the task id or a negative ECOS error. */
int os_task_create(const char *name, func_t entry, void *args);
int os_task_kill(int id);
int os_task_suspend(int id);
int os_task_resume(int id);

/* Read-only access used by kernel commands such as ps. */
const os_task_t *os_task_get(int id);
const char *os_task_name(const task_t *coro);

uint32_t os_ticks(void);

/* Poll the 1 ms hardware timer and advance the tick counter. */
void os_tick_update(void);

/* Sleep helpers used by OS_SLEEP. */
void os_sleep_begin(task_t *coro, uint32_t ticks);
int os_sleep_done(task_t *coro);

/* Round-robin scheduler; does not return. */
void os_run(void) __attribute__((noreturn));

/*
 * Block the calling task for at least `ticks` kernel ticks.
 *
 * The task body must declare its locals with LOCAL(...) from coroutine.h; the
 * sleep state itself lives in the kernel task table, not in the task stack.
 */
#define OS_SLEEP(task, ticks)               \
    do {                                    \
        os_sleep_begin((task), (ticks));    \
        TASK_WAIT((task), os_sleep_done((task))); \
    } while (0)

#endif
