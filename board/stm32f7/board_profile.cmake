# Board profile: STM32F7

set(EMBEDDIP_BOARD_SOURCES
    ${BOARD_COMMON_SOURCES}
    board/stm32f7/board_stm32f7_memory.c
    board/stm32f7/configs.h
)

set(EMBEDDIP_DEVICE_SOURCES
    ${DEVICE_COMMON_SOURCES}
    device/camera/stm32_ov5640.c
    device/display/stm32_rk043fn48h.c
    device/serial/stm32_uart.c
)

set(EMBEDDIP_BOARD_DEFINES
    EMBED_DIP_BOARD_STM32F7=1
    STM32F7xx
    STM32F746xx
)

set(EMBEDDIP_BOARD_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/board/stm32f7
)
