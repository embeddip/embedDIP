if(NOT DEFINED EMBEDDIP_STM32CUBE_H7RS_ROOT)
  message(FATAL_ERROR "EMBEDDIP_STM32CUBE_H7RS_ROOT must be provided to this test")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${EMBEDDIP_SOURCE_DIR}" -B "${EMBEDDIP_BINARY_DIR}"
          -DEMBEDDIP_TARGET_BOARD=STM32H7S -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7
          "-DEMBEDDIP_STM32CUBE_H7RS_ROOT=${EMBEDDIP_STM32CUBE_H7RS_ROOT}"
  RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)

if(EMBEDDIP_EXPECT_MISSING_SDK)
  if(result EQUAL 0)
    message(FATAL_ERROR "H7S profile unexpectedly configured with a missing CubeH7RS SDK")
  endif()
  if(NOT "${out}${err}" MATCHES "EMBEDDIP_STM32CUBE_H7RS_ROOT")
    message(FATAL_ERROR "Missing CubeH7RS SDK diagnostic did not name EMBEDDIP_STM32CUBE_H7RS_ROOT: ${out}${err}")
  endif()
elseif(NOT result EQUAL 0)
  message(FATAL_ERROR "H7S profile did not configure: ${out}${err}")
endif()
