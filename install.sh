#!/usr/bin/env bash
set -euo pipefail

# ECOS Embedded SDK 一键安装脚本
# 
# 用法：
#   ./install.sh                    # 安装到 ~/.local/ecos-sdk/
#
# 安装后用户可以：
# 1. 使用 ecos-create-project 创建新项目
# 2. 将example目录拷贝到任意位置进行开发

# 如果有参数且是--help，显示简单帮助
if [[ $# -gt 0 && "$1" == "--help" ]]; then
    echo "ECOS Embedded SDK 一键安装脚本"
    echo ""
    echo "用法: ./install.sh"
    echo ""
    echo "将自动安装到: ~/.local/ecos-sdk/"
    echo "包含预编译RISC-V工具链和完整开发环境"
    echo ""
    echo "安装后可以："
    echo "• 使用 ecos-create-project 创建新项目"
    echo "• 拷贝示例工程到任意位置进行开发"
    exit 0
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="$HOME/.local/ecos-sdk"
TOOLS_DIR="$PREFIX/toolchain"
RISCV_DIR="$TOOLS_DIR/riscv"
RISCV_BIN="$RISCV_DIR/bin"
TOOLCHAIN_SOURCE="zip"

# 颜色输出
log() { echo -e "\033[1;32m[ECOS]\033[0m $*"; }
warn() { echo -e "\033[1;33m[WARN]\033[0m $*"; }
err() { echo -e "\033[1;31m[ERROR]\033[0m $*"; }

log "ECOS Embedded SDK 一键安装程序"
log "安装路径: $PREFIX"
log "使用预编译工具链"

# 检查依赖
check_dependencies() {
    local deps=(gcc make wget unzip git)
    local missing=()
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" >/dev/null 2>&1; then
            missing+=("$dep")
        fi
    done
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        err "缺少依赖: ${missing[*]}"
        log "请安装缺少的依赖，例如："
        log "  Ubuntu/Debian: sudo apt-get install ${missing[*]}"
        log "  CentOS/RHEL: sudo yum install ${missing[*]}"
        exit 1
    fi
}

# 安装SDK核心文件
install_sdk_core() {
    log "安装SDK核心文件..."
    
    # 拷贝组件库
    ensure_dir "$PREFIX/components/"
    cp -r "$SCRIPT_DIR/components/." "$PREFIX/components/"
    
    # 拷贝模板工程
    cp -r "$SCRIPT_DIR/templates" "$PREFIX/"

    # 拷贝板卡配置
    cp -r "$SCRIPT_DIR/board" "$PREFIX/"

    # 拷贝工具脚本
    cp -r "$SCRIPT_DIR/bin" "$PREFIX/"
    
    # 拷贝工具
    ensure_dir "$PREFIX/tools/"
    cp -r "$SCRIPT_DIR/tools/fixdep" "$PREFIX/tools/"
    cp -r "$SCRIPT_DIR/tools/kconfig" "$PREFIX/tools/"
    cp -r "$SCRIPT_DIR/tools/scripts" "$PREFIX/tools/"

    # 拷贝示例工程
    cp -r "$SCRIPT_DIR/example" "$PREFIX/"

    #拷贝文档
    cp -r "$SCRIPT_DIR/docs" "$PREFIX/"

    # 拷贝设备驱动
    cp -r "$SCRIPT_DIR/devices" "$PREFIX/"

}

# 检查系统是否已有RISC-V工具链
check_system_toolchain() {
  if command -v riscv64-unknown-elf-gcc >/dev/null 2>&1; then
    local gcc_path=$(which riscv64-unknown-elf-gcc)
    local gcc_version=$(riscv64-unknown-elf-gcc --version 2>/dev/null | head -n1 || echo "未知版本")
    log "检测到系统已安装RISC-V工具链："
    log "  路径: $gcc_path"
    log "  版本: $gcc_version"
    
    # 创建符号链接到SDK工具链目录
    ensure_dir "$RISCV_DIR/bin"
    ln -sf "$(dirname "$gcc_path")"/* "$RISCV_DIR/bin/" 2>/dev/null || true
    log "已创建符号链接到SDK工具链目录"
    return 0  # 系统已有工具链
  fi
  return 1  # 系统没有工具链
}

check_zip_exists() {
  local zip_path="$TOOLS_DIR/riscv.tar.gz"
  local extracted_dir="$RISCV_DIR"
  
  # 检查是否已有解压的工具链目录且包含必要文件
  if [[ -d "$extracted_dir/bin" ]] && [[ -x "$extracted_dir/bin/riscv64-unknown-elf-gcc" ]]; then
    log "检测到已安装的RISC-V工具链：$extracted_dir"
    return 0  # 已存在，无需重新安装
  fi
  
  # 检查是否有本地ZIP文件
  if [[ -f "$zip_path" ]]; then
    log "检测到本地ZIP包：$zip_path"
    local file_size=$(stat -c%s "$zip_path" 2>/dev/null || echo "0")
    if [[ $file_size -gt 1048576 ]]; then  # 大于1MB认为是有效文件
      log "使用本地ZIP包进行安装"
      return 1  # 有本地文件，需要解压
    else
      warn "本地ZIP文件过小，可能损坏，将重新下载"
      rm -f "$zip_path"
    fi
  fi
  
  return 2  # 需要下载
}

ensure_dir() { mkdir -p "$1"; }

# 安装工具链
install_toolchain() {
    log "安装RISC-V预编译工具链..."
    
    # 1. 首先检查系统是否已有工具链
    if check_system_toolchain; then
        log "使用系统已安装的RISC-V工具链"
        return 0
    fi
    
    # 2. 检查本地ZIP包或需要下载
    local url="https://github.com/ecoslab/ecos-embed-sdk/releases/download/v1.0.0/riscv.tar.gz"
    local zip_path="$TOOLS_DIR/riscv.tar.gz"
    ensure_dir "$TOOLS_DIR"    
    local zip_status
    set +e  # 临时禁用 set -e
    check_zip_exists
    zip_status=$?
    set -e  # 重新启用 set -e
    log "zip_status $zip_status" 
    case $zip_status in
        0)
        log "RISC-V工具链已安装，跳过下载"
        return 0
        ;;
        1)
        log "使用本地ZIP包解压到：$RISCV_DIR"
        ;;
        2)
        log "下载 RISC-V 工具链 (ZIP)：$url"
        if ! wget -O "$zip_path" "$url"; then
            err "下载失败，请检查网络连接或手动下载到：$zip_path"
            exit 1
        fi
        log "下载完成，解压到：$RISCV_DIR"
        ;;
    esac
    
    # 解压ZIP包
    if [[ $zip_status -ne 0 ]]; then
        rm -rf "$RISCV_DIR"
        if ! tar -zxvf "$zip_path" -C "$TOOLS_DIR"; then
        err "解压失败，ZIP文件可能损坏，请重新下载"
        rm -f "$zip_path"
        exit 1
        fi
        chmod -R +x "$RISCV_DIR" || true
        log "RISC-V工具链安装完成"
    fi
}

add_env() {
  # 检查配置文件中是否包含相同设置
  if grep -q "export PATH=$PREFIX/bin:\$PATH" ~/.bashrc; then
    log "ecos 环境变量已存在于 ~/.bashrc，跳过添加"
  else
    log "添加 ecos 环境变量到 ~/.bashrc..."
    echo "export PATH=$PREFIX/bin:\$PATH" >> ~/.bashrc
  fi

  if grep -q "export PATH=$PREFIX/toolchain/riscv_unknown/bin:\$PATH" ~/.bashrc; then
    log "工具链环境变量已存在于 ~/.bashrc，跳过添加"
  else
    log "添加工具链环境变量到 ~/.bashrc..."
    echo "export PATH=$PREFIX/toolchain/riscv_unknown/bin:\$PATH" >> ~/.bashrc
  fi

  if grep -q "export ECOS_SDK_HOME=$PREFIX" ~/.bashrc; then
    log "ecos 环境变量已存在于 ~/.bashrc，跳过添加"
  else
    log "添加 ecos 环境变量到 ~/.bashrc..."
    echo "export ECOS_SDK_HOME=$PREFIX" >> ~/.bashrc
  fi

    log "环境变量已添加到 ~/.bashrc"
}

# 主安装流程
main() {
    log "开始一键安装 ECOS Embedded SDK..."
    echo ""
    
    check_dependencies
    install_sdk_core
    install_toolchain
    add_env

    echo ""
    log "安装完成！"
    echo "检查toolchain版本 'riscv64-unknown-elf-gcc --version'"
    echo ""
    log "后续步骤："
    echo "1. 重新加载环境: source ~/.bashrc"
    echo "2. 检测SDK是否安装成功: ecos help"
    echo "3. 检测工具链是否安装成功: riscv64-unknown-elf-gcc --version"
    echo "4. 创建示例工程（在当前目录生成）: ecos init_project hello"
    echo "5. 配置示例工程: cd hello && make menuconfig"
    echo "6. 编译示例工程: make"
}

main "$@"