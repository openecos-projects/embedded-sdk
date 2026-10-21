#include "kernel.h"
#include "os_shell.h"

#include "ecos/bsp/console.h"
#include "ecos/log.h"

#define LOG_TAG "mini-os"

/* Demo worker: spawn more copies with the shell command `spawn`. */
void os_worker_entry(task_t *task)
{
    LOCAL(uint32_t step;);

    TASK_BEGIN(task);

    for (local.step = 0u; local.step < 5u; ++local.step) {
        (void)ECOS_LOGI(
            LOG_TAG, "[%s] step %lu", os_task_name(task),
            (unsigned long)local.step
        );
        OS_SLEEP(task, 200u);
    }
    (void)ECOS_LOGI(LOG_TAG, "[%s] finished", os_task_name(task));

    TASK_END(task);
}

/* Boot-time heartbeat: proves that sleeping and round-robin still work. */
void os_heartbeat_entry(task_t *task)
{
    LOCAL(uint32_t beat;);

    TASK_BEGIN(task);

    for (;;) {
        ++local.beat;
        (void)ECOS_LOGI(
            LOG_TAG, "[heartbeat] beat=%lu uptime=%lu ms",
            (unsigned long)local.beat, (unsigned long)os_ticks()
        );
        OS_SLEEP(task, 1000u);
    }

    TASK_END(task);
}

int main(void)
{
    int shell_id;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, os_kernel_init(), "initialize kernel");
    ECOS_PANIC_ON_ERROR(LOG_TAG, os_shell_init(), "initialize shell");

    shell_id = os_task_create("shell", os_shell_entry, NULL);
    if (shell_id != 0) {
        ECOS_PANIC(LOG_TAG, "shell task must own id 0 (got %d)", shell_id);
    }
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, os_task_create("heartbeat", os_heartbeat_entry, NULL),
        "create heartbeat task"
    );
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, os_task_create("worker", os_worker_entry, NULL),
        "create worker task"
    );

    (void)ECOS_LOGI(
        LOG_TAG,
        "mini-os booted: cooperative round-robin, %u ticks/s; type 'help'",
        1000000u / OS_TICK_PERIOD_US
    );
    os_run();
}
