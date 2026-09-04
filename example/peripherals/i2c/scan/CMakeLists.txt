cmake_minimum_required(VERSION 3.20)
project(ecos_i2c_scan LANGUAGES C)

if(NOT DEFINED ECOS_TARGET_DIR OR ECOS_TARGET_DIR STREQUAL "")
    message(FATAL_ERROR "ECOS_TARGET_DIR must point to the selected SDK Target")
endif()

# The SDK Target owns startup, linker and toolchain details.
include("${ECOS_TARGET_DIR}/CMakeLists.txt")
