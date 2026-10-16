#include "shell.h"

#include <assert.h>
#include <stdint.h>

/* Keep the host fixture linkable when section garbage collection is unavailable. */
const ShellCommand shellCommandList[1] = { { 0 } };
const unsigned short shellCommandCount = 0;

extern size_t shellGetVarValue(Shell *shell, ShellCommand *command);

static size_t agency_value;

static void record_agency_value(size_t value)
{
    agency_value = value;
}

SHELL_AGENCY_FUNC(record_agency_value, p1)

int main(void)
{
    char text[] = "hello";
    int number = -42;
    ShellCommand string_var = SHELL_VAR_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_VAR_STRING),
        text, text, text variable);
    ShellCommand pointer_var = SHELL_VAR_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_VAR_POINT),
        address, text, pointer variable);
    ShellCommand integer_var = SHELL_VAR_ITEM(
        SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_VAR_INT),
        number, &number, integer variable);

    assert(shellGetVarValue(NULL, &string_var) == (size_t)(uintptr_t)text);
    assert(shellGetVarValue(NULL, &pointer_var) == (size_t)(uintptr_t)text);
    assert((int)shellGetVarValue(NULL, &integer_var) == -42);
    agencyrecord_agency_value((size_t)(uintptr_t)text, 0u, 0u, 0u, 0u, 0u, 0u);
    assert(agency_value == (size_t)(uintptr_t)text);
    return 0;
}
