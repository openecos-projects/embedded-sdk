# CMake toolchain for the ysyx-2512 RV32E target.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ECOS_TOOLCHAIN_ROOT)

if(NOT DEFINED ECOS_TOOLCHAIN_ROOT OR ECOS_TOOLCHAIN_ROOT STREQUAL "")
    message(FATAL_ERROR "ECOS_TOOLCHAIN_ROOT must point to the active SDK toolchain")
endif()
set(ECOS_TOOLCHAIN_ROOT "${ECOS_TOOLCHAIN_ROOT}" CACHE PATH "Active ECOS SDK toolchain")

set(_ecos_toolchain_bin "${ECOS_TOOLCHAIN_ROOT}/bin")
set(_ecos_tool_suffix "")
if(CMAKE_HOST_WIN32)
    set(_ecos_tool_suffix ".exe")
endif()

set(CMAKE_C_COMPILER
    "${_ecos_toolchain_bin}/riscv-none-elf-gcc${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V C compiler"
)
set(CMAKE_ASM_COMPILER
    "${_ecos_toolchain_bin}/riscv-none-elf-gcc${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V assembler driver"
)
set(CMAKE_OBJCOPY
    "${_ecos_toolchain_bin}/riscv-none-elf-objcopy${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V objcopy"
)
set(CMAKE_OBJDUMP
    "${_ecos_toolchain_bin}/riscv-none-elf-objdump${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V objdump"
)
set(CMAKE_SIZE
    "${_ecos_toolchain_bin}/riscv-none-elf-size${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V size"
)
set(CMAKE_AR
    "${_ecos_toolchain_bin}/riscv-none-elf-ar${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V archiver"
)
set(CMAKE_RANLIB
    "${_ecos_toolchain_bin}/riscv-none-elf-ranlib${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V ranlib"
)
set(CMAKE_STRIP
    "${_ecos_toolchain_bin}/riscv-none-elf-strip${_ecos_tool_suffix}"
    CACHE FILEPATH "ECOS RISC-V strip"
)

set(_ecos_tools
    CMAKE_C_COMPILER
    CMAKE_ASM_COMPILER
    CMAKE_OBJCOPY
    CMAKE_OBJDUMP
    CMAKE_SIZE
    CMAKE_AR
    CMAKE_RANLIB
    CMAKE_STRIP
)
foreach(_ecos_tool IN LISTS _ecos_tools)
    if(NOT EXISTS "${${_ecos_tool}}")
        message(FATAL_ERROR "ECOS tool does not exist: ${${_ecos_tool}}")
    endif()
endforeach()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
