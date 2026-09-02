cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED ECOS_PROJECT_CONFIG OR ECOS_PROJECT_CONFIG STREQUAL "")
    message(FATAL_ERROR "ECOS_PROJECT_CONFIG must name the generated project CMake file")
endif()
if(NOT EXISTS "${ECOS_PROJECT_CONFIG}")
    message(FATAL_ERROR "Generated project CMake file does not exist: ${ECOS_PROJECT_CONFIG}")
endif()
include("${ECOS_PROJECT_CONFIG}")

if(CMAKE_PROJECT_NAME)
    enable_language(ASM)
else()
    project(ecos_ysyx_2512 LANGUAGES C ASM)
endif()

if(NOT DEFINED FIRMWARE_NAME OR FIRMWARE_NAME STREQUAL "")
    if(DEFINED CONFIG_FIRMWARE_NAME AND NOT CONFIG_FIRMWARE_NAME STREQUAL "")
        set(FIRMWARE_NAME "${CONFIG_FIRMWARE_NAME}")
    else()
        set(FIRMWARE_NAME "retrosoc_fw")
    endif()
endif()
set(FIRMWARE_NAME "${FIRMWARE_NAME}" CACHE STRING "Firmware output name")
set(SOC_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")

if(NOT ECOS_APP_SOURCES)
    message(FATAL_ERROR "The resolved project contains no application sources")
endif()
if(NOT ECOS_TARGET_SOURCES)
    message(FATAL_ERROR "The resolved project contains no Target sources")
endif()
if(NOT ECOS_LINKER_SCRIPT OR NOT EXISTS "${ECOS_LINKER_SCRIPT}")
    message(FATAL_ERROR "The resolved project linker script is invalid: ${ECOS_LINKER_SCRIPT}")
endif()

add_library(ecos_build_options INTERFACE)
target_include_directories(ecos_build_options INTERFACE
    ${ECOS_TARGET_INCLUDE_DIRS}
)
target_compile_features(ecos_build_options INTERFACE c_std_11)
target_compile_options(ecos_build_options INTERFACE
    -Wall
    -march=rv32e
    -mabi=ilp32e
    -nostdlib
    -fdata-sections
    -ffunction-sections
)
if(DEFINED CONFIG_BUILD_OPT_FLAGS AND NOT CONFIG_BUILD_OPT_FLAGS STREQUAL "")
    target_compile_options(ecos_build_options INTERFACE "${CONFIG_BUILD_OPT_FLAGS}")
else()
    target_compile_options(ecos_build_options INTERFACE -Os)
endif()
if(CONFIG_BUILD_DEBUG STREQUAL "y")
    target_compile_options(ecos_build_options INTERFACE -g)
endif()
target_compile_options(ecos_build_options INTERFACE
    "$<$<COMPILE_LANGUAGE:C>:-include>"
    "$<$<COMPILE_LANGUAGE:C>:${ECOS_CONFIG_HEADER}>"
)
target_compile_definitions(ecos_build_options INTERFACE CONFIG_LINK_TARGET_MEM)

add_executable(firmware
    ${ECOS_TARGET_SOURCES}
    ${ECOS_APP_SOURCES}
    ${ECOS_TARGET_CAPABILITY_SOURCES}
)
set_target_properties(firmware PROPERTIES
    OUTPUT_NAME "${FIRMWARE_NAME}"
    SUFFIX ".elf"
)
target_include_directories(firmware PRIVATE
    ${ECOS_APP_INCLUDE_DIRS}
)
target_compile_definitions(firmware PRIVATE ${ECOS_APP_DEFINES})
target_link_libraries(firmware PRIVATE ecos_build_options)

foreach(_ecos_component_key IN LISTS ECOS_COMPONENT_KEYS)
    set(_ecos_component_target "ecos_component_${_ecos_component_key}")
    if(ECOS_COMPONENT_${_ecos_component_key}_SOURCES)
        add_library(${_ecos_component_target} STATIC
            ${ECOS_COMPONENT_${_ecos_component_key}_SOURCES}
        )
        target_include_directories(${_ecos_component_target} PUBLIC
            ${ECOS_COMPONENT_${_ecos_component_key}_INCLUDE_DIRS}
        )
        target_compile_definitions(${_ecos_component_target} PUBLIC
            ${ECOS_COMPONENT_${_ecos_component_key}_DEFINES}
        )
        target_link_libraries(${_ecos_component_target} PUBLIC ecos_build_options)
    else()
        add_library(${_ecos_component_target} INTERFACE)
        target_include_directories(${_ecos_component_target} INTERFACE
            ${ECOS_COMPONENT_${_ecos_component_key}_INCLUDE_DIRS}
        )
        target_compile_definitions(${_ecos_component_target} INTERFACE
            ${ECOS_COMPONENT_${_ecos_component_key}_DEFINES}
        )
        target_link_libraries(${_ecos_component_target} INTERFACE ecos_build_options)
    endif()
endforeach()
foreach(_ecos_component_key IN LISTS ECOS_COMPONENT_KEYS)
    set(_ecos_component_target "ecos_component_${_ecos_component_key}")
    if(ECOS_COMPONENT_${_ecos_component_key}_SOURCES)
        set(_ecos_component_scope PUBLIC)
    else()
        set(_ecos_component_scope INTERFACE)
    endif()
    foreach(_ecos_dependency_key IN LISTS ECOS_COMPONENT_${_ecos_component_key}_DEPENDENCIES)
        target_link_libraries(${_ecos_component_target} ${_ecos_component_scope}
            "ecos_component_${_ecos_dependency_key}"
        )
    endforeach()
    target_link_libraries(firmware PRIVATE ${_ecos_component_target})
endforeach()

# The FSBL must not call code that is still stored in Flash.
set_source_files_properties("${SOC_ROOT}/startup/loader.c" PROPERTIES
    COMPILE_OPTIONS "-O0"
)
# QSPI polling is timing-sensitive on the verified L4 implementation.
set_source_files_properties("${SOC_ROOT}/hal/qspi/qspi.c" PROPERTIES
    COMPILE_OPTIONS "-O0"
)

set(MAP_FILE "${OUTPUT_DIR}/${FIRMWARE_NAME}.map")
target_link_options(firmware PRIVATE
    -nostdlib
    -march=rv32e
    -mabi=ilp32e
    "-Wl,-e,_start"
    "-Wl,-m,elf32lriscv"
    "-Wl,-z,noexecstack"
    "-Wl,-T,${ECOS_LINKER_SCRIPT}"
    "-Wl,--gc-sections"
    "-Wl,-Map,${MAP_FILE}"
)
set_property(TARGET firmware APPEND PROPERTY LINK_DEPENDS
    "${ECOS_LINKER_SCRIPT}"
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
