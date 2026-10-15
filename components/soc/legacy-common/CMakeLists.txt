cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED ECOS_PROJECT_CONFIG OR ECOS_PROJECT_CONFIG STREQUAL "")
    message(FATAL_ERROR "ECOS_PROJECT_CONFIG must name the generated project CMake file")
endif()
if(NOT EXISTS "${ECOS_PROJECT_CONFIG}")
    message(FATAL_ERROR "Generated project CMake file does not exist: ${ECOS_PROJECT_CONFIG}")
endif()
include("${ECOS_PROJECT_CONFIG}")

if(CONFIG_LINK_TARGET_MEM AND DEFINED ECOS_SOC_MEM_LINKER_SCRIPT)
    set(ECOS_LINKER_SCRIPT "${ECOS_SOC_MEM_LINKER_SCRIPT}")
endif()
if(CONFIG_LINK_RAM_REGION_PSRAM AND DEFINED ECOS_SOC_PSRAM_LINKER_SCRIPT)
    set(ECOS_LINKER_SCRIPT "${ECOS_SOC_PSRAM_LINKER_SCRIPT}")
endif()

if(CMAKE_PROJECT_NAME)
    enable_language(ASM)
else()
    project(ecos_legacy_target LANGUAGES C ASM)
endif()

if(NOT DEFINED ECOS_SOC_ROOT OR ECOS_SOC_ROOT STREQUAL "")
    message(FATAL_ERROR "ECOS_SOC_ROOT must name the selected Target directory")
endif()
if(NOT DEFINED ECOS_SOC_MARCH OR NOT DEFINED ECOS_SOC_ABI OR
   NOT DEFINED ECOS_SOC_LINK_MACHINE)
    message(FATAL_ERROR "Target ISA variables are required")
endif()

set(SOC_ROOT "${ECOS_SOC_ROOT}")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")
set(COMMON_ROOT "${CMAKE_CURRENT_LIST_DIR}")

if(NOT ECOS_APP_SOURCES)
    message(FATAL_ERROR "The resolved project contains no application sources")
endif()
if(NOT ECOS_TARGET_SOURCES)
    message(FATAL_ERROR "The resolved project contains no Target sources")
endif()
if(NOT ECOS_LINKER_SCRIPT OR NOT EXISTS "${ECOS_LINKER_SCRIPT}")
    message(FATAL_ERROR "The resolved project linker script is invalid: ${ECOS_LINKER_SCRIPT}")
endif()

if(NOT DEFINED FIRMWARE_NAME OR FIRMWARE_NAME STREQUAL "")
    if(DEFINED CONFIG_FIRMWARE_NAME AND NOT CONFIG_FIRMWARE_NAME STREQUAL "")
        set(FIRMWARE_NAME "${CONFIG_FIRMWARE_NAME}")
    else()
        set(FIRMWARE_NAME "retrosoc_fw")
    endif()
endif()
set(FIRMWARE_NAME "${FIRMWARE_NAME}" CACHE STRING "Firmware output name")

add_library(ecos_build_options INTERFACE)
target_compile_features(ecos_build_options INTERFACE c_std_11)
target_include_directories(ecos_build_options INTERFACE
    "${ECOS_GENERATED_INCLUDE_DIR}"
)
target_compile_options(ecos_build_options INTERFACE
    -Wall
    "-march=${ECOS_SOC_MARCH}"
    "-mabi=${ECOS_SOC_ABI}"
    -nostdlib
    -ffreestanding
    -fno-builtin
    -mcmodel=medany
    -msmall-data-limit=0
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

set(_ecos_component_include_dirs)
foreach(_ecos_component_key IN LISTS ECOS_COMPONENT_KEYS)
    list(APPEND _ecos_component_include_dirs
        ${ECOS_COMPONENT_${_ecos_component_key}_INCLUDE_DIRS}
    )
endforeach()
list(REMOVE_DUPLICATES _ecos_component_include_dirs)

add_library(ecos_target_objects OBJECT
    ${ECOS_TARGET_SOURCES}
    ${ECOS_TARGET_CAPABILITY_SOURCES}
)
target_include_directories(ecos_target_objects PRIVATE
    ${ECOS_TARGET_INCLUDE_DIRS}
    ${ECOS_TARGET_CAPABILITY_INCLUDE_DIRS}
    ${_ecos_component_include_dirs}
)
target_link_libraries(ecos_target_objects PRIVATE ecos_build_options)

if(CONFIG_LINK_TARGET_MEM AND ECOS_USE_MEM_LOADER)
    set(_ecos_application_target application)
    set(_ecos_application_output_name app)
else()
    set(_ecos_application_target firmware)
    set(_ecos_application_output_name "${FIRMWARE_NAME}")
endif()

add_executable(${_ecos_application_target}
    ${ECOS_APP_SOURCES}
    $<TARGET_OBJECTS:ecos_target_objects>
)
set_target_properties(${_ecos_application_target} PROPERTIES
    OUTPUT_NAME "${_ecos_application_output_name}"
    SUFFIX ".elf"
)
target_include_directories(${_ecos_application_target} PRIVATE ${ECOS_APP_INCLUDE_DIRS})
target_compile_definitions(${_ecos_application_target} PRIVATE ${ECOS_APP_DEFINES})
target_link_libraries(${_ecos_application_target} PRIVATE ecos_build_options)

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
        target_link_libraries(${_ecos_component_target} PRIVATE ecos_build_options)
    else()
        add_library(${_ecos_component_target} INTERFACE)
        target_include_directories(${_ecos_component_target} INTERFACE
            ${ECOS_COMPONENT_${_ecos_component_key}_INCLUDE_DIRS}
        )
        target_compile_definitions(${_ecos_component_target} INTERFACE
            ${ECOS_COMPONENT_${_ecos_component_key}_DEFINES}
        )
    endif()
    if(ECOS_COMPONENT_${_ecos_component_key}_CMAKE_TARGET)
        add_library(
            "${ECOS_COMPONENT_${_ecos_component_key}_CMAKE_TARGET}"
            ALIAS
            ${_ecos_component_target}
        )
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
    foreach(_ecos_dependency_key IN LISTS ECOS_COMPONENT_${_ecos_component_key}_PRIVATE_DEPENDENCIES)
        if(ECOS_COMPONENT_${_ecos_component_key}_SOURCES)
            set(_ecos_private_scope PRIVATE)
        else()
            set(_ecos_private_scope INTERFACE)
        endif()
        target_link_libraries(${_ecos_component_target} ${_ecos_private_scope}
            "ecos_component_${_ecos_dependency_key}"
        )
    endforeach()
endforeach()
foreach(_ecos_component_key IN LISTS ECOS_COMPONENT_ROOT_KEYS)
    target_link_libraries(${_ecos_application_target} PRIVATE
        "ecos_component_${_ecos_component_key}"
    )
endforeach()

if(_ecos_application_target STREQUAL "application")
    set(_ecos_application_map "${OUTPUT_DIR}/app.map")
else()
    set(_ecos_application_map "${OUTPUT_DIR}/${FIRMWARE_NAME}.map")
endif()
target_link_options(${_ecos_application_target} PRIVATE
    -nostdlib
    "-march=${ECOS_SOC_MARCH}"
    "-mabi=${ECOS_SOC_ABI}"
    -mcmodel=medany
    "-Wl,-e,_start"
    "-Wl,-m,${ECOS_SOC_LINK_MACHINE}"
    "-Wl,-z,noexecstack"
    "-Wl,-T,${ECOS_LINKER_SCRIPT}"
    "-Wl,--gc-sections"
    "-Wl,-Map,${_ecos_application_map}"
)
set_property(TARGET ${_ecos_application_target} APPEND PROPERTY LINK_DEPENDS
    "${ECOS_LINKER_SCRIPT}"
)

if(_ecos_application_target STREQUAL "application")
    set(ECOS_MEM_LOADER_APP_BINARY "${OUTPUT_DIR}/app.bin")
    configure_file(
        "${COMMON_ROOT}/legacy_mem_payload.S.in"
        "${CMAKE_BINARY_DIR}/legacy_mem_payload.S"
        @ONLY
    )
    set_source_files_properties(
        "${CMAKE_BINARY_DIR}/legacy_mem_payload.S"
        PROPERTIES OBJECT_DEPENDS "${ECOS_MEM_LOADER_APP_BINARY}"
    )
    add_custom_command(TARGET application POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" -S -O binary
            "$<TARGET_FILE:application>" "${ECOS_MEM_LOADER_APP_BINARY}"
        BYPRODUCTS "${ECOS_MEM_LOADER_APP_BINARY}"
        VERBATIM
    )

    add_executable(firmware
        "${COMMON_ROOT}/legacy_mem_loader.S"
        "${COMMON_ROOT}/legacy_mem_loader.c"
        "${CMAKE_BINARY_DIR}/legacy_mem_payload.S"
    )
    set_target_properties(firmware PROPERTIES
        OUTPUT_NAME "${FIRMWARE_NAME}"
        SUFFIX ".elf"
    )
    target_include_directories(firmware PRIVATE
        ${ECOS_TARGET_INCLUDE_DIRS}
    )
    target_link_libraries(firmware PRIVATE ecos_build_options)
    add_dependencies(firmware application)

    set(MAP_FILE "${OUTPUT_DIR}/${FIRMWARE_NAME}.map")
    set(ECOS_MEM_LOADER_LINKER_SCRIPT
        "${COMMON_ROOT}/legacy_mem_loader.lds"
    )
    target_link_options(firmware PRIVATE
        -nostdlib
        "-march=${ECOS_SOC_MARCH}"
        "-mabi=${ECOS_SOC_ABI}"
        -mcmodel=medany
        "-Wl,-e,_start"
        "-Wl,-m,${ECOS_SOC_LINK_MACHINE}"
        "-Wl,-z,noexecstack"
        "-Wl,-T,${ECOS_MEM_LOADER_LINKER_SCRIPT}"
        "-Wl,--gc-sections"
        "-Wl,-Map,${MAP_FILE}"
    )
    set_property(TARGET firmware APPEND PROPERTY LINK_DEPENDS
        "${ECOS_MEM_LOADER_LINKER_SCRIPT}"
        "${CMAKE_BINARY_DIR}/legacy_mem_payload.S"
        "${COMMON_ROOT}/generate_outputs.cmake"
    )
else()
    set(MAP_FILE "${OUTPUT_DIR}/${FIRMWARE_NAME}.map")
    set_property(TARGET firmware APPEND PROPERTY LINK_DEPENDS
        "${COMMON_ROOT}/generate_outputs.cmake"
    )
endif()

add_custom_command(TARGET firmware POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT_ELF=$<TARGET_FILE:firmware>"
        "-DOUTPUT_DIR=${OUTPUT_DIR}"
        "-DFIRMWARE_NAME=${FIRMWARE_NAME}"
        "-DOBJCOPY=${CMAKE_OBJCOPY}"
        "-DSIZE_TOOL=${CMAKE_SIZE}"
        "-DOBJDUMP=${CMAKE_OBJDUMP}"
        -P "${COMMON_ROOT}/generate_outputs.cmake"
    BYPRODUCTS
        "${MAP_FILE}"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.bin"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.hex"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.txt"
        "${OUTPUT_DIR}/${FIRMWARE_NAME}.size"
    VERBATIM
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
