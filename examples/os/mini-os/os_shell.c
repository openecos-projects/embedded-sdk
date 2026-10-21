#include "os_shell.h"

#include "ecos/bsp/console.h"
#include "ecos/log.h"
#include "shell.h"
#include "shell_port.h"

#include <string.h>

#define LOG_TAG "mini-os"

#define OS_SHELL_BUFFER_SIZE 1024u
#define OS_SHELL_COMMAND_CAPACITY 48u

/* Implemented by main.c; used by the `spawn` command. */
extern void os_worker_entry(task_t *task);

static Shell g_shell;
static char g_shell_buffer[OS_SHELL_BUFFER_SIZE];
static ShellCommand g_command_table[OS_SHELL_COMMAND_CAPACITY];

static const char *state_name(os_task_state_t state)
{
    switch (state) {
    case OS_TASK_READY:
        return "ready";
    case OS_TASK_RUNNING:
        return "run";
    case OS_TASK_BLOCKED:
        return "block";
    case OS_TASK_SUSPENDED:
        return "susp";
    case OS_TASK_FREE:
    default:
        return "free";
    }
}

static int parse_task_id(const char *text)
{
    unsigned long value = 0u;

    if (text == NULL || *text == '\0') {
        return -1;
    }
    for (; *text != '\0'; ++text) {
        if (*text < '0' || *text > '9') {
            return -1;
        }
        value = value * 10u + (unsigned long)(*text - '0');
        if (value >= OS_MAX_TASKS) {
            return -1;
        }
    }
    return (int)value;
}

static int cmd_ps(int argc, char **argv)
{
    Shell *shell = shellGetCurrent();
    int id;

    (void)argc;
    (void)argv;

    shellPrint(shell, "id name       state  ticks switches wake\n");
    for (id = 0; id < OS_MAX_TASKS; ++id) {
        const os_task_t *task = os_task_get(id);

        if (task == NULL) {
            continue;
        }
        shellPrint(
            shell, "%d %s %s %lu %lu %lu\n", id, task->name,
            state_name(task->state), (unsigned long)task->cpu_ticks,
            (unsigned long)task->switches, (unsigned long)task->wake_tick
        );
    }
    return 0;
}

static int cmd_uptime(int argc, char **argv)
{
    Shell *shell = shellGetCurrent();
    uint32_t ticks = os_ticks();

    (void)argc;
    (void)argv;

    shellPrint(
        shell, "uptime: %lu ticks (~%lu ms)\n", (unsigned long)ticks,
        (unsigned long)OS_TICKS_TO_MS(ticks)
    );
    return 0;
}

static int cmd_tick(int argc, char **argv)
{
    Shell *shell = shellGetCurrent();

    (void)argc;
    (void)argv;

    shellPrint(shell, "tick: %lu\n", (unsigned long)os_ticks());
    return 0;
}

static int cmd_spawn(int argc, char **argv)
{
    Shell *shell = shellGetCurrent();
    int id;

    (void)argc;
    (void)argv;

    id = os_task_create("worker", os_worker_entry, NULL);
    if (id < 0) {
        shellPrint(shell, "spawn failed: %d\n", id);
        return -1;
    }
    shellPrint(shell, "task %d (worker) created\n", id);
    return 0;
}

static int cmd_kill(int argc, char **argv)
{
    Shell *shell = shellGetCurrent();
    int id;
    int result;

    if (argc < 2) {
        shellPrint(shell, "usage: kill <id>\n");
        return -1;
    }
    id = parse_task_id(argv[1]);
    if (id < 0) {
        shellPrint(shell, "invalid task id\n");
        return -1;
    }
    if (id == 0) {
        shellPrint(shell, "refusing to kill the shell task\n");
        return -1;
    }
    result = os_task_kill(id);
    if (result != ECOS_OK) {
        shellPrint(shell, "kill failed: %d\n", result);
        return -1;
    }
    shellPrint(shell, "task %d killed\n", id);
    return 0;
}

static int cmd_suspend(int argc, char **argv)
{
    Shell *shell = shellGetCurrent();
    int id;
    int result;

    if (argc < 2) {
        shellPrint(shell, "usage: suspend <id>\n");
        return -1;
    }
    id = parse_task_id(argv[1]);
    if (id < 0) {
        shellPrint(shell, "invalid task id\n");
        return -1;
    }
    if (id == 0) {
        shellPrint(shell, "refusing to suspend the shell task\n");
        return -1;
    }
    result = os_task_suspend(id);
    if (result != ECOS_OK) {
        shellPrint(shell, "suspend failed: %d\n", result);
        return -1;
    }
    shellPrint(shell, "task %d suspended\n", id);
    return 0;
}

static int cmd_resume(int argc, char **argv)
{
    Shell *shell = shellGetCurrent();
    int id;
    int result;

    if (argc < 2) {
        shellPrint(shell, "usage: resume <id>\n");
        return -1;
    }
    id = parse_task_id(argv[1]);
    if (id < 0) {
        shellPrint(shell, "invalid task id\n");
        return -1;
    }
    result = os_task_resume(id);
    if (result != ECOS_OK) {
        shellPrint(shell, "resume failed: %d\n", result);
        return -1;
    }
    shellPrint(shell, "task %d resumed\n", id);
    return 0;
}

static const ShellCommand os_commands[] = {
    SHELL_CMD_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
            SHELL_CMD_DISABLE_RETURN,
        ps, cmd_ps, list kernel tasks
    ),
    SHELL_CMD_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
            SHELL_CMD_DISABLE_RETURN,
        uptime, cmd_uptime, show kernel uptime
    ),
    SHELL_CMD_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
            SHELL_CMD_DISABLE_RETURN,
        tick, cmd_tick, show tick counter
    ),
    SHELL_CMD_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
            SHELL_CMD_DISABLE_RETURN,
        spawn, cmd_spawn, create a demo worker task
    ),
    SHELL_CMD_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
            SHELL_CMD_DISABLE_RETURN,
        kill, cmd_kill, kill a task
    ),
    SHELL_CMD_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
            SHELL_CMD_DISABLE_RETURN,
        suspend, cmd_suspend, suspend a task
    ),
    SHELL_CMD_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
            SHELL_CMD_DISABLE_RETURN,
        resume, cmd_resume, resume a task
    ),
};

int os_shell_init(void)
{
    const unsigned short extra =
        (unsigned short)(sizeof(os_commands) / sizeof(os_commands[0]));
    unsigned short count;

    memset(&g_shell, 0, sizeof(g_shell));
    g_shell.read = shellRead;
    g_shell.write = shellWrite;
    shellInit(&g_shell, g_shell_buffer, sizeof(g_shell_buffer));

    count = g_shell.commandList.count;
    if ((size_t)count + extra >
        sizeof(g_command_table) / sizeof(g_command_table[0])) {
        shellRemove(&g_shell);
        return ECOS_ERR_NO_MEMORY;
    }

    /* Letter-Shell uses a static table by default; extend it per project. */
    memcpy(
        g_command_table, g_shell.commandList.base,
        (size_t)count * sizeof(ShellCommand)
    );
    memcpy(
        &g_command_table[count], os_commands,
        (size_t)extra * sizeof(ShellCommand)
    );
    g_shell.commandList.base = g_command_table;
    g_shell.commandList.count = (unsigned short)(count + extra);
    return ECOS_OK;
}

void os_shell_entry(task_t *task)
{
    LOCAL(uint8_t byte;);

    TASK_BEGIN(task);

    for (;;) {
        while (bsp_console_try_read(&local.byte) == 1) {
            shellHandler(&g_shell, (char)local.byte);
        }
        TASK_YIELD(task);
    }

    TASK_END(task);
}
