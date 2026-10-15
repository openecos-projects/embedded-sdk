#include "shell_ext.h"

#include <assert.h>

ShellCommand *shellSeekCommand(Shell *shell, const char *cmd,
                              ShellCommand *base, unsigned short compare_length)
{
    (void)shell;
    (void)cmd;
    (void)base;
    (void)compare_length;
    return NULL;
}

size_t shellGetVarValue(Shell *shell, ShellCommand *command)
{
    (void)shell;
    (void)command;
    return 0;
}

static void accepts(char *input, size_t expected)
{
    size_t result = 17u;
    assert(shellExtParsePara(NULL, input, NULL, &result) == 0);
    assert(result == expected);
}

static void rejects(char *input)
{
    size_t result = 17u;
    assert(shellExtParsePara(NULL, input, NULL, &result) != 0);
    assert(result == 17u);
}

int main(void)
{
    accepts("123", 123u);
    accepts("-42", (size_t)-42);
    accepts("0x2a", 42u);
    accepts("0b101", 5u);
    accepts("077", 63u);

    rejects("1.5");
    rejects("1.");
    rejects("0xzz");
    rejects("08");
    rejects("-");
    rejects("99999999999999999999999999999999999999");
    return 0;
}
