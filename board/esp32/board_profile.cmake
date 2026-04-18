# Board profile: ESP32

set(EMBEDDIP_BOARD_SOURCES
    ${BOARD_COMMON_SOURCES}
    board/esp32/board_esp32eye_memory.cpp
)

set(EMBEDDIP_DEVICE_SOURCES
    ${DEVICE_COMMON_SOURCES}
    device/camera/esp32_ov2640.cpp
    device/serial/esp32_uart.cpp
)

set(EMBEDDIP_BOARD_DEFINES
    EMBED_DIP_BOARD_ESP32=1
    ARDUINO_ARCH_ESP32
)

set(EMBEDDIP_BOARD_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/board/esp32
)
