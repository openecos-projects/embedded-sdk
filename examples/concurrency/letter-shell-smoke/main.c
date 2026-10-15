#include "ecos/bsp/console.h"
#include "ecos/log.h"
#include "shell.h"
#include "shell_port.h"

int main(void)
{
    char buffer[1024];
    Shell shell = {0};

    ECOS_PANIC_ON_ERROR("letter-shell-smoke", bsp_console_init(), "initialize console");
    shell.read = shellRead;
    shell.write = shellWrite;
    shellInit(&shell, buffer, sizeof(buffer));
    (void)shellRun(&shell, "help");
    (void)ECOS_LOGI("letter-shell-smoke", "command parser completed");
    shellRemove(&shell);
    return 0;
}
