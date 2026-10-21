#include "kernel.h"

#include "ecos/driver/timer.h"
#include "ecos/log.h"

#include <string.h>

#define LOG_TAG "mini-os"

/* Catch up at most this many missed ticks per scheduler round. */
#define OS_TICK_CATCH_UP 4u

static os_task_t g_tasks[OS_MAX_TASKS];
static volatile uint32_t g_ticks;
static uint8_t g_hardware_tick;

static const ecos_timer_config_t g_timer_config = { OS_TICK_PERIOD_US };

static void timer_restart(void)
{
    (void)ecos_timer_init(ECOS_TIMER_DEFAULT, &g_timer_config);
    (void)ecos_timer_start(ECOS_TIMER_DEFAULT);
}

int os_kernel_init(void)
{
    int result;

    memset(g_tasks, 0, sizeof(g_tasks));
    g_ticks = 0u;

    result = ecos_timer_init(ECOS_TIMER_DEFAULT, &g_timer_config);
    if (result == ECOS_OK) {
        result = ecos_timer_start(ECOS_TIMER_DEFAULT);
    }

    g_hardware_tick = result == ECOS_OK ? 1u : 0u;
    if (g_hardware_tick) {
        (void)ECOS_LOGI(
            LOG_TAG, "tick source: timer0, %u us", OS_TICK_PERIOD_US
        );
    } else {
        /*
         * A board without a usable period timer still runs; one tick is then
         * accounted per scheduler round. The kernel API stays identical.
         */
        (void)ECOS_LOGW(
            LOG_TAG,
            "timer0 unavailable (%d); ticking once per scheduler round",
            result
        );
    }
    return ECOS_OK;
}

uint32_t os_ticks(void)
{
    return g_ticks;
}

void os_tick_update(void)
{
    uint32_t caught_up;

    if (!g_hardware_tick) {
        ++g_ticks;
        return;
    }

    for (caught_up = 0u; caught_up < OS_TICK_CATCH_UP; ++caught_up) {
        int expired = ecos_timer_is_expired(ECOS_TIMER_DEFAULT);

        if (expired < 0) {
            /* Fall back to scheduler-round ticks instead of spinning. */
            g_hardware_tick = 0u;
            ++g_ticks;
            (void)ECOS_LOGW(LOG_TAG, "tick timer failed (%d); using rounds", expired);
            return;
        }
        if (expired == 0) {
            return;
        }
        ++g_ticks;
        timer_restart();
    }
}

static os_task_t *task_from_coro(const task_t *coro)
{
    int id;

    if (coro == NULL) {
        return NULL;
    }
    id = coro->id;
    if (id < 0 || id >= OS_MAX_TASKS || !g_tasks[id].used) {
        return NULL;
    }
    return &g_tasks[id];
}

int os_task_create(const char *name, func_t entry, void *args)
{
    int id;

    if (entry == NULL) {
        return ECOS_ERR_INVALID_ARGUMENT;
    }
    for (id = 0; id < OS_MAX_TASKS; ++id) {
        if (!g_tasks[id].used) {
            break;
        }
    }
    if (id == OS_MAX_TASKS) {
        return ECOS_ERR_NO_MEMORY;
    }

    memset(&g_tasks[id], 0, sizeof(g_tasks[id]));
    g_tasks[id].name = name != NULL ? name : "task";
    g_tasks[id].entry = entry;
    g_tasks[id].state = OS_TASK_READY;
    g_tasks[id].used = 1u;
    g_tasks[id].coro = create_task(id, entry, args);
    return id;
}

int os_task_kill(int id)
{
    os_task_t *task;

    if (id < 0 || id >= OS_MAX_TASKS) {
        return ECOS_ERR_INVALID_ARGUMENT;
    }
    task = &g_tasks[id];
    if (!task->used || task->coro.func == NULL) {
        return ECOS_ERR_NOT_FOUND;
    }

    delete_task(&task->coro);
    task->state = OS_TASK_FREE;
    task->used = 0u;
    return ECOS_OK;
}

int os_task_suspend(int id)
{
    os_task_t *task;

    if (id < 0 || id >= OS_MAX_TASKS) {
        return ECOS_ERR_INVALID_ARGUMENT;
    }
    task = &g_tasks[id];
    if (!task->used || task->coro.func == NULL) {
        return ECOS_ERR_NOT_FOUND;
    }
    task->state = OS_TASK_SUSPENDED;
    return ECOS_OK;
}

int os_task_resume(int id)
{
    os_task_t *task;

    if (id < 0 || id >= OS_MAX_TASKS) {
        return ECOS_ERR_INVALID_ARGUMENT;
    }
    task = &g_tasks[id];
    if (!task->used || task->coro.func == NULL) {
        return ECOS_ERR_NOT_FOUND;
    }
    if (task->state == OS_TASK_SUSPENDED) {
        task->state = OS_TASK_READY;
    }
    return ECOS_OK;
}

const os_task_t *os_task_get(int id)
{
    if (id < 0 || id >= OS_MAX_TASKS || !g_tasks[id].used ||
        g_tasks[id].coro.func == NULL) {
        return NULL;
    }
    return &g_tasks[id];
}

const char *os_task_name(const task_t *coro)
{
    const os_task_t *task = task_from_coro(coro);

    return task != NULL ? task->name : "?";
}

void os_sleep_begin(task_t *coro, uint32_t ticks)
{
    os_task_t *task = task_from_coro(coro);

    if (task == NULL) {
        return;
    }
    /* A zero-length sleep still yields once. */
    task->wake_tick = os_ticks() + (ticks == 0u ? 1u : ticks);
    task->state = OS_TASK_BLOCKED;
}

int os_sleep_done(task_t *coro)
{
    os_task_t *task = task_from_coro(coro);

    if (task == NULL) {
        return 1;
    }
    if ((int32_t)(os_ticks() - task->wake_tick) < 0) {
        task->state = OS_TASK_BLOCKED;
        return 0;
    }
    task->state = OS_TASK_READY;
    return 1;
}

void os_run(void)
{
    for (;;) {
        int id;

        os_tick_update();

        for (id = 0; id < OS_MAX_TASKS; ++id) {
            os_task_t *task = &g_tasks[id];
            uint32_t started;

            if (!task->used) {
                continue;
            }
            if (task->coro.func == NULL) {
                task->state = OS_TASK_FREE;
                task->used = 0u;
                continue;
            }
            if (task->state == OS_TASK_SUSPENDED) {
                continue;
            }

            task->state = OS_TASK_RUNNING;
            started = os_ticks();
            task->coro.func(&task->coro);
            task->cpu_ticks += os_ticks() - started;
            ++task->switches;

            if (task->used && task->coro.func != NULL &&
                task->state == OS_TASK_RUNNING) {
                task->state = OS_TASK_READY;
            }

            /* A long-running task may have crossed a tick boundary. */
            os_tick_update();
        }
    }
}
