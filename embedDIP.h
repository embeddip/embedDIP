#ifndef EMBED_DIP_H
#define EMBED_DIP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "embedDIP_configs.h"

// =============================
// Project Version
// =============================
#define EMBED_DIP_VERSION_MAJOR 0
#define EMBED_DIP_VERSION_MINOR 0
#define EMBED_DIP_VERSION_PATCH 0

// =============================
// Feature Flags
// =============================
#define ENABLE_UART_LOGGING 1
#define ENABLE_IMAGE_PROCESSING 1
#define ENABLE_CAMERA_INPUT 1
#define ENABLE_DISPLAY_OUTPUT 1

#define STM32F7xx 1

// Core APIs
#include "core/image.h"
#include "core/filter.h"
#include "core/histogram.h"
#include "core/memory_manager.h"
#include "imgproc/color.h"
#include "imgproc/fft.h"
#include "board/common.h"

// C++ Wrappers
#include "wrapper/ImageWrapper.hpp"
#include "wrapper/CameraWrapper.hpp"
#include "wrapper/DisplayWrapper.hpp"

// Device drivers
#include "device/camera/ov5640.h"
#include "device/display/display.h"

// =============================
// Board-specific C code
// =============================
#ifdef ARDUINO_ARCH_ESP32
#include "board/esp32/esp32_common.c"
#endif

#ifdef STM32F7xx
#include "board/stm32f7/stm32_common.c"
#endif


#ifdef __cplusplus
}
#endif

#endif // EMBED_DIP_H
