#!/usr/bin/env sh
set -eu

usage() {
    printf 'Usage: %s <build-dir> [report-dir]\n' "$0" >&2
}

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    usage
    exit 2
fi

BUILD_DIR=$1
REPORT_DIR=${2:-}
FAILURES=0

if [ "$REPORT_DIR" != "" ]; then
    mkdir -p "$REPORT_DIR"
    ARTIFACTS_FILE="$REPORT_DIR/artifacts.txt"
    RESULT_FILE="$REPORT_DIR/result.json"
else
    ARTIFACTS_FILE=""
    RESULT_FILE=""
fi

find_matches() {
    pattern=$1
    find "$BUILD_DIR" -maxdepth 1 -type f -name "$pattern" -print 2>/dev/null | sort
}

find_elf_files() {
    find "$BUILD_DIR" -maxdepth 1 -type f -print 2>/dev/null |
        while IFS= read -r candidate; do
            case "$candidate" in
                *.bin|*.hex|*.txt|*.map|*.lds|*.info)
                    continue
                    ;;
            esac
            if file "$candidate" 2>/dev/null | grep 'ELF' >/dev/null 2>&1; then
                printf '%s\n' "$candidate"
            fi
        done | sort
}

record_artifacts() {
    if [ "$ARTIFACTS_FILE" != "" ]; then
        find "$BUILD_DIR" -maxdepth 1 -type f -print 2>/dev/null | sort > "$ARTIFACTS_FILE"
    fi
}

json_result() {
    status=$1
    if [ "$RESULT_FILE" != "" ]; then
        {
            printf '{\n'
            printf '  "suite": "default-board-build",\n'
            printf '  "artifact_check": "%s",\n' "$status"
            printf '  "build_dir": "%s",\n' "$BUILD_DIR"
            printf '  "requires": {\n'
            printf '    "elf": true,\n'
            printf '    "firmware_bin_or_hex": true\n'
            printf '  }\n'
            printf '}\n'
        } > "$RESULT_FILE"
    fi
}

if [ ! -d "$BUILD_DIR" ]; then
    printf 'FAIL build_dir:%s\n' "$BUILD_DIR" >&2
    json_result fail
    exit 1
fi

ELF_MATCHES=$(find_matches '*.elf')
if [ "$ELF_MATCHES" = "" ]; then
    ELF_MATCHES=$(find_elf_files)
fi
BIN_MATCHES=$(find_matches '*.bin')
HEX_MATCHES=$(find_matches '*.hex')

record_artifacts

if [ "$ELF_MATCHES" = "" ]; then
    printf 'FAIL artifact:elf\n' >&2
    FAILURES=$((FAILURES + 1))
else
    printf 'PASS artifact:elf\n'
    printf '%s\n' "$ELF_MATCHES"
fi

if [ "$BIN_MATCHES" = "" ] && [ "$HEX_MATCHES" = "" ]; then
    printf 'FAIL artifact:*.bin-or-*.hex\n' >&2
    FAILURES=$((FAILURES + 1))
else
    printf 'PASS artifact:*.bin-or-*.hex\n'
    if [ "$BIN_MATCHES" != "" ]; then
        printf '%s\n' "$BIN_MATCHES"
    fi
    if [ "$HEX_MATCHES" != "" ]; then
        printf '%s\n' "$HEX_MATCHES"
    fi
fi

if [ "$FAILURES" -eq 0 ]; then
    json_result pass
    exit 0
fi

json_result fail
exit 1
