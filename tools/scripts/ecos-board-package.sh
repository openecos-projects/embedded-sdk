#!/usr/bin/env bash

# ECOS BSP 清单读取工具。这里只解析 ecos-board.yml 使用的简单格式，
# 避免要求用户额外安装 YAML 解析程序。

ecos_board_registry_dir() {
    if [[ -n "${ECOS_BOARD_REGISTRY:-}" ]]; then
        echo "$ECOS_BOARD_REGISTRY"
    else
        echo "$ECOS_SDK_HOME/board/UserBSP"
    fi
}

ecos_board_manifest_value() {
    local manifest="$1"
    local key="$2"
    awk -F: -v key="$key" '
        $1 == key {
            value = substr($0, index($0, ":") + 1)
            sub(/^[[:space:]]+/, "", value)
            sub(/[[:space:]]+$/, "", value)
            print value
            exit
        }
    ' "$manifest"
}

ecos_board_manifest_section_value() {
    local manifest="$1"
    local section="$2"
    local key="$3"
    awk -F: -v section="$section" -v key="$key" '
        $1 == section {
            in_section = 1
            next
        }
        in_section && $0 ~ /^[[:space:]]*$/ {
            next
        }
        in_section && $0 ~ /^  #[[:space:]]/ {
            next
        }
        in_section && $0 !~ /^  / {
            exit
        }
        in_section {
            line = $0
            sub(/^  /, "", line)
            split(line, parts, ":")
            if (parts[1] == key) {
                value = substr(line, index(line, ":") + 1)
                sub(/^[[:space:]]+/, "", value)
                sub(/[[:space:]]+$/, "", value)
                print value
                exit
            }
        }
    ' "$manifest"
}

ecos_board_manifest_aliases() {
    local manifest="$1"
    awk '
        $0 == "aliases:" {
            in_aliases = 1
            next
        }
        in_aliases && $0 !~ /^  - / {
            exit
        }
        in_aliases {
            alias = $0
            sub(/^  - /, "", alias)
            print alias
        }
    ' "$manifest"
}

ecos_board_manifest_profiles() {
    local manifest="$1"
    awk '
        $0 == "profiles:" {
            in_profiles = 1
            next
        }
        in_profiles && ($0 ~ /^[[:space:]]*$/ || $0 ~ /^  #/) {
            next
        }
        in_profiles && $0 !~ /^  / {
            exit
        }
        in_profiles && $0 ~ /^  [A-Za-z][A-Za-z0-9_-]*:[[:space:]]*$/ {
            name = $0
            sub(/^  /, "", name)
            sub(/:[[:space:]]*$/, "", name)
            print name
        }
    ' "$manifest"
}

ecos_board_manifest_profile_value() {
    local manifest="$1"
    local profile="$2"
    local key="$3"
    awk -v profile="$profile" -v key="$key" '
        $0 == "profiles:" {
            in_profiles = 1
            next
        }
        in_profiles && $0 !~ /^  / && $0 !~ /^[[:space:]]*$/ {
            exit
        }
        in_profiles && $0 ~ /^  [A-Za-z][A-Za-z0-9_-]*:[[:space:]]*$/ {
            name = $0
            sub(/^  /, "", name)
            sub(/:[[:space:]]*$/, "", name)
            in_profile = (name == profile)
            next
        }
        in_profile && $0 ~ /^    [A-Za-z][A-Za-z0-9_]*:/ {
            line = $0
            sub(/^    /, "", line)
            field = line
            sub(/:.*/, "", field)
            if (field == key) {
                value = substr(line, index(line, ":") + 1)
                sub(/^[[:space:]]+/, "", value)
                sub(/[[:space:]]+$/, "", value)
                print value
                exit
            }
        }
    ' "$manifest"
}

ecos_board_resolve_profile() {
    local manifest="$1"
    local requested="${2:-}"
    local profiles default_profile

    profiles="$(ecos_board_manifest_profiles "$manifest")"
    if [[ -z "$profiles" ]]; then
        if [[ -n "$requested" ]]; then
            echo "该 BSP 没有定义 profile，不能使用 --profile $requested" >&2
            return 1
        fi
        return 0
    fi

    if [[ -n "$requested" ]]; then
        if grep -Fxq "$requested" <<< "$profiles"; then
            echo "$requested"
            return 0
        fi
        echo "BSP 中不存在 profile: $requested" >&2
        echo "可用 profile: $(paste -sd, <<< "$profiles")" >&2
        return 1
    fi

    default_profile="$(ecos_board_manifest_value "$manifest" default_profile)"
    if [[ -n "$default_profile" ]]; then
        if ! grep -Fxq "$default_profile" <<< "$profiles"; then
            echo "default_profile 指向了不存在的 profile: $default_profile" >&2
            return 1
        fi
        echo "$default_profile"
    else
        head -n 1 <<< "$profiles"
    fi
}

ecos_board_profile_file() {
    local manifest="$1"
    local profile="$2"
    local key="$3"

    if [[ -n "$profile" ]]; then
        ecos_board_manifest_profile_value "$manifest" "$profile" "$key"
    else
        ecos_board_manifest_section_value "$manifest" files "$key"
    fi
}

ecos_board_manifest_dir() {
    dirname "$1"
}

ecos_board_manifest_path() {
    local manifest="$1"
    local section="$2"
    local key="$3"
    local default_value="${4:-}"
    local value

    value="$(ecos_board_manifest_section_value "$manifest" "$section" "$key")"
    [[ -n "$value" ]] || value="$default_value"
    [[ -n "$value" ]] || return 1
    echo "$(ecos_board_manifest_dir "$manifest")/$value"
}

ecos_board_iter_manifests() {
    local registry_dir
    registry_dir="$(ecos_board_registry_dir)"

    if [[ -d "$ECOS_SDK_HOME/board" ]]; then
        find "$ECOS_SDK_HOME/board" -mindepth 2 -maxdepth 2 \
            -name ecos-board.yml -type f ! -path '*/UserBSP/*' 2>/dev/null
    fi

    if [[ -n "${ECOS_BOARD_PATH:-}" ]]; then
        local entry
        IFS=: read -r -a entries <<< "$ECOS_BOARD_PATH"
        for entry in "${entries[@]}"; do
            if [[ -f "$entry" ]]; then
                echo "$entry"
            elif [[ -f "$entry/ecos-board.yml" ]]; then
                echo "$entry/ecos-board.yml"
            elif [[ -d "$entry" ]]; then
                find "$entry" -mindepth 1 -maxdepth 2 -name ecos-board.yml -type f 2>/dev/null
            fi
        done
    fi

    if [[ -d "$registry_dir" ]]; then
        find -L "$registry_dir" -mindepth 2 -maxdepth 2 -name ecos-board.yml -type f 2>/dev/null
    fi
}

ecos_board_matches() {
    local manifest="$1"
    local query="$2"
    local id alias

    id="$(ecos_board_manifest_value "$manifest" id)"
    [[ "$query" == "$id" ]] && return 0
    while IFS= read -r alias; do
        [[ "$query" == "$alias" ]] && return 0
    done < <(ecos_board_manifest_aliases "$manifest")
    return 1
}

ecos_board_find_manifest() {
    local query="$1"
    local manifest
    while IFS= read -r manifest; do
        if ecos_board_matches "$manifest" "$query"; then
            echo "$manifest"
            return 0
        fi
    done < <(ecos_board_iter_manifests | awk '!seen[$0]++')
    return 1
}

ecos_board_print_list() {
    local manifest id name aliases source
    printf "%-20s %-16s %-24s %s\n" "ID" "ALIASES" "NAME" "PATH"
    while IFS= read -r manifest; do
        id="$(ecos_board_manifest_value "$manifest" id)"
        name="$(ecos_board_manifest_value "$manifest" name)"
        aliases="$(ecos_board_manifest_aliases "$manifest" | paste -sd, -)"
        source="$(ecos_board_manifest_dir "$manifest")"
        printf "%-20s %-16s %-24s %s\n" "$id" "$aliases" "$name" "$source"
    done < <(ecos_board_iter_manifests | awk '!seen[$0]++')
}

ecos_board_validate_relative_path() {
    local board_dir="$1"
    local relative_path="$2"
    local label="$3"
    local expected_type="${4:-file}"
    local base resolved

    if [[ -z "$relative_path" ]]; then
        echo "缺少清单项: $label" >&2
        return 1
    fi
    if [[ "$relative_path" = /* || "$relative_path" == ".." || "$relative_path" == ../* || "$relative_path" == */../* ]]; then
        echo "清单路径必须位于 BSP 目录内: $label=$relative_path" >&2
        return 1
    fi

    base="$(realpath -m "$board_dir")"
    resolved="$(realpath -m "$board_dir/$relative_path")"
    if [[ "$resolved" != "$base"/* ]]; then
        echo "清单路径越过 BSP 目录: $label=$relative_path" >&2
        return 1
    fi

    if [[ "$expected_type" == "dir" ]]; then
        [[ -d "$resolved" ]] || {
            echo "清单声明的目录不存在: $label=$relative_path" >&2
            return 1
        }
    else
        [[ -f "$resolved" ]] || {
            echo "清单声明的文件不存在: $label=$relative_path" >&2
            return 1
        }
    fi
}

ecos_board_find_required_todos() {
    local board_dir="$1"
    grep -RInI \
        --exclude=README.md \
        --exclude-dir=.git \
        --exclude-dir=build \
        'TODO_BSP_REQUIRED' "$board_dir" 2>/dev/null || true
}

ecos_board_validate_manifest() {
    local manifest="$1"
    local board_dir schema id name category arch alias value failed=0
    local profiles profile default_profile

    if [[ ! -f "$manifest" ]]; then
        echo "找不到 BSP 清单: $manifest" >&2
        return 1
    fi
    board_dir="$(ecos_board_manifest_dir "$manifest")"
    schema="$(ecos_board_manifest_value "$manifest" schema)"
    id="$(ecos_board_manifest_value "$manifest" id)"
    name="$(ecos_board_manifest_value "$manifest" name)"
    category="$(ecos_board_manifest_value "$manifest" category)"
    arch="$(ecos_board_manifest_value "$manifest" arch)"

    [[ "$schema" == "1" ]] || { echo "schema 必须是 1" >&2; failed=1; }
    [[ "$id" =~ ^[a-z][a-z0-9]*(-[a-z0-9]+)*$ ]] || { echo "id 必须以小写字母开头，且只允许小写字母、数字和短横线: $id" >&2; failed=1; }
    [[ -n "$name" ]] || { echo "缺少清单项: name" >&2; failed=1; }
    [[ "$category" =~ ^[A-Za-z][A-Za-z0-9_]*$ ]] || { echo "category 格式不正确: $category" >&2; failed=1; }
    [[ -n "$arch" ]] || { echo "缺少清单项: arch" >&2; failed=1; }

    while IFS= read -r alias; do
        [[ "$alias" =~ ^[a-z0-9][a-z0-9_-]*$ ]] || {
            echo "别名格式不正确: $alias" >&2
            failed=1
        }
    done < <(ecos_board_manifest_aliases "$manifest")

    local required_files=(makefile board_header build_conf)
    local key
    for key in "${required_files[@]}"; do
        value="$(ecos_board_manifest_section_value "$manifest" files "$key")"
        ecos_board_validate_relative_path "$board_dir" "$value" "files.$key" file || failed=1
    done

    profiles="$(ecos_board_manifest_profiles "$manifest")"
    if [[ -n "$profiles" ]]; then
        default_profile="$(ecos_board_manifest_value "$manifest" default_profile)"
        if [[ -n "$default_profile" ]] && ! grep -Fxq "$default_profile" <<< "$profiles"; then
            echo "default_profile 指向了不存在的 profile: $default_profile" >&2
            failed=1
        fi
        while IFS= read -r profile; do
            [[ "$profile" =~ ^[a-z][a-z0-9_]*$ ]] || {
                echo "profile 名称格式不正确: $profile" >&2
                failed=1
            }
            for key in linker_script startup; do
                value="$(ecos_board_manifest_profile_value "$manifest" "$profile" "$key")"
                ecos_board_validate_relative_path "$board_dir" "$value" "profiles.$profile.$key" file || failed=1
            done
        done <<< "$profiles"
    else
        for key in linker_script startup; do
            value="$(ecos_board_manifest_section_value "$manifest" files "$key")"
            ecos_board_validate_relative_path "$board_dir" "$value" "files.$key" file || failed=1
        done
    fi
    for key in board drivers; do
        value="$(ecos_board_manifest_section_value "$manifest" kconfig "$key")"
        ecos_board_validate_relative_path "$board_dir" "$value" "kconfig.$key" file || failed=1
    done

    value="$(ecos_board_manifest_section_value "$manifest" files makefile_isolated)"
    if [[ -n "$value" ]]; then
        ecos_board_validate_relative_path "$board_dir" "$value" files.makefile_isolated file || failed=1
    fi

    value="$(ecos_board_manifest_section_value "$manifest" paths drivers)"
    [[ -n "$value" ]] || value=driver
    ecos_board_validate_relative_path "$board_dir" "$value" paths.drivers dir || failed=1
    for key in loader templates; do
        value="$(ecos_board_manifest_section_value "$manifest" paths "$key")"
        if [[ -n "$value" ]]; then
            ecos_board_validate_relative_path "$board_dir" "$value" "paths.$key" dir || failed=1
        fi
    done

    return "$failed"
}

ecos_board_resolve_template() {
    local manifest="$1"
    local template_name="$2"
    local query="$3"
    local candidate alias templates_path

    candidate="$ECOS_SDK_HOME/templates/$template_name/$query"
    [[ -d "$candidate" ]] && { echo "$candidate"; return 0; }
    while IFS= read -r alias; do
        candidate="$ECOS_SDK_HOME/templates/$template_name/$alias"
        [[ -d "$candidate" ]] && { echo "$candidate"; return 0; }
    done < <(ecos_board_manifest_aliases "$manifest")

    templates_path="$(ecos_board_manifest_section_value "$manifest" paths templates)"
    if [[ -n "$templates_path" ]]; then
        candidate="$(ecos_board_manifest_dir "$manifest")/$templates_path/$template_name"
        [[ -d "$candidate" ]] && { echo "$candidate"; return 0; }
    fi

    candidate="$ECOS_SDK_HOME/templates/$template_name/common"
    [[ -d "$candidate" ]] && { echo "$candidate"; return 0; }
    return 1
}

ecos_board_write_project_config() {
    local manifest="$1"
    local output="$2"
    local profile="${3:-}"
    local board_dir id category build_conf board_kconfig driver_kconfig drivers_path

    board_dir="$(realpath -m "$(ecos_board_manifest_dir "$manifest")")"
    id="$(ecos_board_manifest_value "$manifest" id)"
    category="$(ecos_board_manifest_value "$manifest" category)"
    build_conf="$(ecos_board_manifest_section_value "$manifest" files build_conf)"
    board_kconfig="$(ecos_board_manifest_section_value "$manifest" kconfig board)"
    driver_kconfig="$(ecos_board_manifest_section_value "$manifest" kconfig drivers)"
    drivers_path="$(ecos_board_manifest_section_value "$manifest" paths drivers)"
    [[ -n "$drivers_path" ]] || drivers_path=driver

    mkdir -p "$(dirname "$output")"
    {
        echo "# 由 ecos set_board 生成，请勿手工修改。"
        echo "BOARD_PACKAGE := $board_dir"
        echo "BOARD_ID := $id"
        echo "BOARD_CATEGORY := $category"
        [[ -z "$profile" ]] || echo "BOARD_PROFILE := $profile"
        echo "CATEGORY := $category"
        echo "BOARD_BUILD_CONF := $board_dir/$build_conf"
        echo "BOARD_KCONFIG := $board_dir/$board_kconfig"
        echo "DRIVER_KCONFIG := $board_dir/$driver_kconfig"
        echo "BOARD_DRIVER_DIR := $board_dir/$drivers_path"
    } > "$output"
}

ecos_board_write_sdk_path_config() {
    local output="$1"
    local sdk_root
    sdk_root="$(realpath -m "$ECOS_SDK_HOME")"

    mkdir -p "$(dirname "$output")"
    {
        echo "# 由 ecos init_project 生成。SDK 移动后可修改此文件，或执行 eval \"\$(ecos env)\"。"
        echo "ECOS_SDK_HOME ?= $sdk_root"
        echo "export ECOS_SDK_HOME"
    } > "$output"
}

ecos_board_ensure_sdk_include() {
    local makefile="$1"
    [[ -f "$makefile" ]] || return 0
    if ! grep -Fqx -- '-include configs/sdk-path.mk' "$makefile"; then
        sed -i '1i-include configs/sdk-path.mk' "$makefile"
    fi
}
