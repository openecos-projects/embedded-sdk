#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TEST_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
SDK_ROOT=$(CDPATH= cd -- "$TEST_ROOT/.." && pwd)

BOARD_ID=starrysky-c2
TARGET=c2
RUN_ID=${RUN_ID:-$(date +%Y%m%d-%H%M%S)}
SUITE=default-board-build
TEMPLATES=${TEMPLATES:-"asm_hello coroutine_test filesystem_test gpio hello i2c_scan minesweeper shell_test spi_flash_test spi_st7735 spi_st7735_donut st7789"}
ASM_TEMPLATES=${ASM_TEMPLATES:-"asm_hello"}

if [ "${ECOS_SDK_HOME:-}" = "" ]; then
    ECOS_SDK_HOME=$SDK_ROOT
    export ECOS_SDK_HOME
fi

WORK_RUN_DIR="$TEST_ROOT/work/$SUITE/$RUN_ID"
REPORT_DIR="$TEST_ROOT/reports/$SUITE/$RUN_ID/$BOARD_ID"
SUMMARY_FILE="$REPORT_DIR/summary.txt"
RESULT_FILE="$REPORT_DIR/result.json"

mkdir -p "$WORK_RUN_DIR" "$REPORT_DIR"

printf '== %s ==\n' "$SUITE"
printf 'board:    %s\n' "$BOARD_ID"
printf 'target:   %s\n' "$TARGET"
printf 'run_id:   %s\n' "$RUN_ID"
printf 'work:     %s\n' "$WORK_RUN_DIR"
printf 'reports:  %s\n' "$REPORT_DIR"
printf '\n'

"$SCRIPT_DIR/preflight.sh" > "$REPORT_DIR/preflight.log" 2>&1
printf 'preflight: pass\n'
printf '\n'
printf '%-22s %-10s %s\n' "template" "status" "stage"
printf '%-22s %-10s %s\n' "--------" "------" "-----"

TOTAL=0
PASSED=0
FAILED=0
: > "$SUMMARY_FILE"

for TEMPLATE in $TEMPLATES; do
    TOTAL=$((TOTAL + 1))
    PROJECT_NAME="default_build_${TARGET}_${TEMPLATE}"
    PROJECT_DIR="$WORK_RUN_DIR/$PROJECT_NAME"
    TEMPLATE_REPORT_DIR="$REPORT_DIR/$TEMPLATE"
    mkdir -p "$TEMPLATE_REPORT_DIR"

    if [ -e "$PROJECT_DIR" ]; then
        printf '%-22s %-10s %s\n' "$TEMPLATE" "FAIL" "project-exists"
        printf 'FAIL project already exists: %s\n' "$PROJECT_DIR" > "$TEMPLATE_REPORT_DIR/result.txt"
        printf '%s fail project-exists\n' "$TEMPLATE" >> "$SUMMARY_FILE"
        FAILED=$((FAILED + 1))
        continue
    fi

    if ! (
        cd "$WORK_RUN_DIR"
        ecos init_project "$TEMPLATE" -name "$PROJECT_NAME" -target "$TARGET"
    ) > "$TEMPLATE_REPORT_DIR/init_project.log" 2>&1; then
        printf '%-22s %-10s %s\n' "$TEMPLATE" "FAIL" "init_project"
        printf 'FAIL template=%s stage=init_project\n' "$TEMPLATE" > "$TEMPLATE_REPORT_DIR/result.txt"
        printf '%s fail init_project\n' "$TEMPLATE" >> "$SUMMARY_FILE"
        FAILED=$((FAILED + 1))
        continue
    fi

    if [ ! -d "$PROJECT_DIR" ]; then
        printf '%-22s %-10s %s\n' "$TEMPLATE" "FAIL" "project_missing"
        printf 'FAIL template=%s stage=project_missing\n' "$TEMPLATE" > "$TEMPLATE_REPORT_DIR/result.txt"
        printf '%s fail project-missing\n' "$TEMPLATE" >> "$SUMMARY_FILE"
        FAILED=$((FAILED + 1))
        continue
    fi

    TEMPLATE_TYPE=c
    for ASM_TEMPLATE in $ASM_TEMPLATES; do
        if [ "$TEMPLATE" = "$ASM_TEMPLATE" ]; then
            TEMPLATE_TYPE=asm
            break
        fi
    done

    if [ "$TEMPLATE_TYPE" = "asm" ]; then
        printf 'SKIP_CONFIG template=%s reason=asm-template\n' "$TEMPLATE" > "$TEMPLATE_REPORT_DIR/configure.log"
    else
        if [ -s "$PROJECT_DIR/configs/defconfig" ]; then
            CONFIG_TARGET=defconfig
        else
            CONFIG_TARGET=alldefconfig
        fi

        if ! (
            cd "$PROJECT_DIR"
            make "$CONFIG_TARGET"
        ) > "$TEMPLATE_REPORT_DIR/configure.log" 2>&1; then
            printf '%-22s %-10s %s\n' "$TEMPLATE" "FAIL" "configure"
            printf 'FAIL template=%s stage=configure\n' "$TEMPLATE" > "$TEMPLATE_REPORT_DIR/result.txt"
            printf '%s fail configure\n' "$TEMPLATE" >> "$SUMMARY_FILE"
            FAILED=$((FAILED + 1))
            continue
        fi
    fi

    if ! (
        cd "$PROJECT_DIR"
        make -j"$(nproc)"
    ) > "$TEMPLATE_REPORT_DIR/build.log" 2>&1; then
        printf '%-22s %-10s %s\n' "$TEMPLATE" "FAIL" "build"
        printf 'FAIL template=%s stage=build\n' "$TEMPLATE" > "$TEMPLATE_REPORT_DIR/result.txt"
        printf '%s fail build\n' "$TEMPLATE" >> "$SUMMARY_FILE"
        FAILED=$((FAILED + 1))
        continue
    fi

    if ! "$SCRIPT_DIR/check-artifacts.sh" "$PROJECT_DIR/build" "$TEMPLATE_REPORT_DIR" > "$TEMPLATE_REPORT_DIR/check-artifacts.log" 2>&1; then
        printf '%-22s %-10s %s\n' "$TEMPLATE" "FAIL" "artifacts"
        printf 'FAIL template=%s stage=artifacts\n' "$TEMPLATE" > "$TEMPLATE_REPORT_DIR/result.txt"
        printf '%s fail artifacts\n' "$TEMPLATE" >> "$SUMMARY_FILE"
        FAILED=$((FAILED + 1))
        continue
    fi

    printf '%-22s %-10s %s\n' "$TEMPLATE" "PASS" "-"
    printf 'PASS template=%s\n' "$TEMPLATE" > "$TEMPLATE_REPORT_DIR/result.txt"
    printf '%s pass\n' "$TEMPLATE" >> "$SUMMARY_FILE"
    PASSED=$((PASSED + 1))
done

{
    printf '{\n'
    printf '  "suite": "%s",\n' "$SUITE"
    printf '  "run_id": "%s",\n' "$RUN_ID"
    printf '  "board_id": "%s",\n' "$BOARD_ID"
    printf '  "target": "%s",\n' "$TARGET"
    printf '  "total": %s,\n' "$TOTAL"
    printf '  "passed": %s,\n' "$PASSED"
    printf '  "failed": %s,\n' "$FAILED"
    if [ "$FAILED" -eq 0 ]; then
        printf '  "status": "pass",\n'
    else
        printf '  "status": "fail",\n'
    fi
    printf '  "summary_file": "%s",\n' "$SUMMARY_FILE"
    printf '  "report_dir": "%s"\n' "$REPORT_DIR"
    printf '}\n'
} > "$RESULT_FILE"

if [ "$FAILED" -eq 0 ]; then
    printf '\nPASS %s %s templates=%s\n' "$SUITE" "$BOARD_ID" "$PASSED"
    exit 0
fi

printf '\nFAIL %s %s passed=%s failed=%s\n' "$SUITE" "$BOARD_ID" "$PASSED" "$FAILED" >&2
printf 'summary: %s\n' "$SUMMARY_FILE" >&2
printf 'reports: %s\n' "$REPORT_DIR" >&2
exit 1
