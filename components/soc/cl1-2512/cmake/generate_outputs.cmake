if(NOT DEFINED INPUT_ELF OR NOT DEFINED OUTPUT_DIR OR NOT DEFINED FIRMWARE_NAME)
    message(FATAL_ERROR "INPUT_ELF, OUTPUT_DIR and FIRMWARE_NAME are required")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

execute_process(
    COMMAND "${OBJCOPY}" -S -O binary "${INPUT_ELF}" "${OUTPUT_DIR}/${FIRMWARE_NAME}.bin"
    RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "objcopy binary conversion failed: ${_result}")
endif()

execute_process(
    COMMAND "${OBJCOPY}" --change-addresses -0x20000000 -O verilog
            "${INPUT_ELF}" "${OUTPUT_DIR}/${FIRMWARE_NAME}.hex"
    RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "objcopy hex conversion failed: ${_result}")
endif()

execute_process(
    COMMAND "${OBJDUMP}" -d "${INPUT_ELF}"
    OUTPUT_FILE "${OUTPUT_DIR}/${FIRMWARE_NAME}.txt"
    RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "objdump disassembly failed: ${_result}")
endif()

execute_process(
    COMMAND "${SIZE_TOOL}" -A "${INPUT_ELF}"
    OUTPUT_FILE "${OUTPUT_DIR}/${FIRMWARE_NAME}.size"
    RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "size report failed: ${_result}")
endif()
