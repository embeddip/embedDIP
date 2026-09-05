execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${EMBEDDIP_SOURCE_DIR}" -B "${EMBEDDIP_BINARY_DIR}/bad-n6"
          -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7
  RESULT_VARIABLE bad_result OUTPUT_VARIABLE bad_out ERROR_VARIABLE bad_err)
if(bad_result EQUAL 0 OR NOT "${bad_out}${bad_err}" MATCHES "Invalid board/arch/cpu combination")
  message(FATAL_ERROR "STM32N6+CORTEX_M7 must be rejected")
endif()
