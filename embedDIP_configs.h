#ifndef EMBED_DIP_CONFIGS_H
#define EMBED_DIP_CONFIGS_H

// === Select target board by uncommenting ===
// Only one of these should be defined at a time

// #define TARGET_BOARD_STM32F7
#define TARGET_BOARD_STM32F7
// #define TARGET_BOARD_OTHER

// === Board-specific defines ===
#if defined(TARGET_BOARD_STM32F7)
#define STM32F7xx 1
#define ENABLE_UART_LOGGING 1
#define ENABLE_IMAGE_PROCESSING 1
#define ENABLE_CAMERA_INPUT 1
#define ENABLE_DISPLAY_OUTPUT 1
#define DEVICE_OV5640 1
#define DEVICE_RK043FN48H 1
#define DEVICE_STM32_UART 1

#elif defined(TARGET_BOARD_ESP32)
#define ARDUINO_ARCH_ESP32 1
#define ENABLE_UART_LOGGING 1
#define ENABLE_IMAGE_PROCESSING 1
#define ENABLE_CAMERA_INPUT 1
#define DEVICE_OV2640 1
#define DEVICE_ESP32_UART 1
#else
#error "Please define a target board in embedDIP_configs.h"
#endif

#endif // EMBED_DIP_CONFIGS_H
