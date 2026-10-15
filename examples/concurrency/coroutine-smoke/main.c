#include "coroutine.h"
#include "ecos/bsp/console.h"
#include "ecos/log.h"

#define LOG_TAG "coroutine-smoke"

static void worker_task(task_t *task)
{
    LOCAL(uint32_t count;);
    TASK_BEGIN(task);

    for (;;) {
        local.count++;
        (void)ECOS_LOGI(LOG_TAG, "worker round %lu", (unsigned long)local.count);
        if (local.count >= 3u) {
            TASK_END(task);
            return;
        }
        TASK_YIELD(task);
    }

    TASK_END(task);
}

int main(void)
{
    ECOS_PANIC_ON_ERROR(
        LOG_TAG, bsp_console_init(), "initialize console"
    );

    (void)create_task(0, worker_task, NULL);
    task_scheduler(6);
    (void)ECOS_LOGI(LOG_TAG, "scheduler completed");
    return 0;
}
