#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TEST_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
SDK_ROOT=$(CDPATH= cd -- "$TEST_ROOT/.." && pwd)

if [ "${ECOS_SDK_HOME:-}" = "" ]; then
    ECOS_SDK_HOME=$SDK_ROOT
    export ECOS_SDK_HOME
fi

FAILURES=0

pass() {
    printf 'PASS %s\n' "$1"
}

fail() {
    printf 'FAIL %s\n' "$1" >&2
    FAILURES=$((FAILURES + 1))
}

check_cmd() {
    if command -v "$1" >/dev/null 2>&1; then
        pass "command:$1"
    else
        fail "command:$1"
    fi
}

check_file() {
    if [ -e "$1" ]; then
        pass "path:$1"
    else
        fail "path:$1"
    fi
}

printf 'preflight.sdk_root=%s\n' "$SDK_ROOT"
printf 'preflight.test_root=%s\n' "$TEST_ROOT"
printf 'preflight.ecos_sdk_home=%s\n' "$ECOS_SDK_HOME"

if [ -d "$ECOS_SDK_HOME" ]; then
    pass "ECOS_SDK_HOME"
else
    fail "ECOS_SDK_HOME"
fi

check_cmd make
check_cmd python3
check_cmd riscv64-unknown-elf-gcc
check_cmd riscv64-unknown-elf-objcopy
check_cmd riscv64-unknown-elf-objdump

if command -v ecos >/dev/null 2>&1; then
    pass "command:ecos"
else
    fail "command:ecos"
fi

check_file "$TEST_ROOT/config/boards.yaml"
check_file "$ECOS_SDK_HOME/tools/kconfig/Kconfig"
check_file "$ECOS_SDK_HOME/board/StarrySkyC2/Makefile"
check_file "$ECOS_SDK_HOME/templates/hello/c2/main.c"

if [ "$FAILURES" -eq 0 ]; then
    printf 'preflight.status=pass\n'
    exit 0
fi

printf 'preflight.status=fail\n' >&2
printf 'preflight.failures=%s\n' "$FAILURES" >&2
exit 1
