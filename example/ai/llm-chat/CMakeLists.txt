cmake_minimum_required(VERSION 3.20)
project(ecos_llm_chat LANGUAGES C)

if(NOT DEFINED ECOS_TARGET_DIR OR ECOS_TARGET_DIR STREQUAL "")
    message(FATAL_ERROR "ECOS_TARGET_DIR must point to the selected SDK Target")
endif()

# 模型与词表由用户自行准备（见 README.md），经 .incbin 链接进 Flash，
# XIP 直接读取，不占用 RAM。
set(LLM_MODEL_BIN "${CMAKE_CURRENT_SOURCE_DIR}/stories260k_q8.bin")
set(LLM_TOKENIZER_BIN "${CMAKE_CURRENT_SOURCE_DIR}/tok512.bin")
foreach(_llm_bin IN ITEMS "${LLM_MODEL_BIN}" "${LLM_TOKENIZER_BIN}")
    if(NOT EXISTS "${_llm_bin}")
        message(FATAL_ERROR
            "missing model file: ${_llm_bin}; see example/ai/llm-chat/README.md")
    endif()
endforeach()

set(LLM_DATA_S "${CMAKE_CURRENT_BINARY_DIR}/llm_data.S")
file(WRITE "${LLM_DATA_S}"
    "    .section .rodata\n"
    "    .balign 4\n"
    "    .global llm_model_start\n"
    "    .type llm_model_start, @object\n"
    "llm_model_start:\n"
    "    .incbin \"${LLM_MODEL_BIN}\"\n"
    "    .global llm_model_end\n"
    "llm_model_end:\n"
    "    .balign 4\n"
    "    .global llm_tokenizer_start\n"
    "    .type llm_tokenizer_start, @object\n"
    "llm_tokenizer_start:\n"
    "    .incbin \"${LLM_TOKENIZER_BIN}\"\n"
    "    .global llm_tokenizer_end\n"
    "llm_tokenizer_end:\n"
)

# The SDK Target owns startup, linker and toolchain details.
include("${ECOS_TARGET_DIR}/CMakeLists.txt")

target_sources(firmware PRIVATE "${LLM_DATA_S}")
# runq.c 的浮点运算与数学函数来自工具链运行时库（无 FPU 时为软浮点实现）。
target_link_libraries(firmware PRIVATE m gcc)
