cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PROJECT_DIR OR PROJECT_DIR STREQUAL "")
    message(FATAL_ERROR "PROJECT_DIR must point to the application project")
endif()
if(NOT DEFINED ECOS_SDK_HOME OR ECOS_SDK_HOME STREQUAL "")
    set(ECOS_SDK_HOME "$ENV{ECOS_SDK_HOME}")
endif()
if(ECOS_SDK_HOME STREQUAL "")
    message(FATAL_ERROR "ECOS_SDK_HOME must point to the SDK root")
endif()

if(CMAKE_PROJECT_NAME)
    enable_language(ASM)
else()
    project(ecos_ysyx_2512 LANGUAGES C ASM)
endif()

set(FIRMWARE_NAME "retrosoc_fw" CACHE STRING "Firmware output name")
set(SOC_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")

set(FIRMWARE_SOURCES
    "${SOC_ROOT}/startup/start.S"
    "${SOC_ROOT}/startup/loader.c"
    "${PROJECT_DIR}/main.c"
    "${SOC_ROOT}/hal/sys_uart/sys_uart.c"
)

add_executable(firmware ${FIRMWARE_SOURCES})
set_target_properties(firmware PROPERTIES
    OUTPUT_NAME "${FIRMWARE_NAME}"
    SUFFIX ".elf"
)

target_include_directories(firmware PRIVATE
    "${PROJECT_DIR}"
    "${SOC_ROOT}/include"
    "${ECOS_SDK_HOME}/hal/sys_uart"
)

target_compile_options(firmware PRIVATE
    -Wall
    -std=gnu11
    -march=rv32e
    -mabi=ilp32e
    -nostdlib
    -fdata-sections
    -ffunction-sections
    -Os
    -g
)
target_compile_definitions(firmware PRIVATE CONFIG_LINK_TARGET_MEM)

# The FSBL must not call code that is still stored in Flash.
set_source_files_properties("${SOC_ROOT}/startup/loader.c" PROPERTIES
    COMPILE_OPTIONS "-O0"
)
# QSPI polling is timing-sensitive on the verified L4 implementation.
set_source_files_properties("${SOC_ROOT}/hal/qspi/qspi.c" PROPERTIES
    COMPILE_OPTIONS "-O0"
)

set(LINKER_SCRIPT "${SOC_ROOT}/linker/sections.lds")
set(MAP_FILE "${OUTPUT_DIR}/${FIRMWARE_NAME}.map")
target_link_options(firmware PRIVATE
    -nostdlib
    -march=rv32e
    -mabi=ilp32e
    "-Wl,-e,_start"
    "-Wl,-m,elf32lriscv"
    "-Wl,-z,noexecstack"
    "-Wl,-T,${LINKER_SCRIPT}"
    "-Wl,--gc-sections"
    "-Wl,-Map,${MAP_FILE}"
)
set_property(TARGET firmware APPEND PROPERTY LINK_DEPENDS
    "${LINKER_SCRIPT}"
    "${SOC_ROOT}/cmake/generate_outputs.cmake"
)

add_custom_command(TARGET firmware POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT_ELF=$<TARGET_FILE:firmware>"
        "-DOUTPUT_DIR=${OUTPUT_DIR}"
        "-DFIRMWARE_NAME=${FIRMWARE_NAME}"
        "-DOBJCOPY=${CMAKE_OBJCOPY}"
        "-DOBJDUMP=${CMAKE_OBJDUMP}"
        "-DSIZE_TOOL=${CMAKE_SIZE}"
        -P "${SOC_ROOT}/cmake/generate_outputs.cmake"
    BYPRODUCTS
        "${MAP_FILE}"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.bin"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.hex"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.txt"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.size"
    VERBATIM
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
