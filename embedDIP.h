#ifndef EMBED_DIP_H
#define EMBED_DIP_H

#ifdef __cplusplus
extern "C"
{
#endif

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

// =============================
// Core Modules
// =============================
#include "dip.h"
#include "common.h"
#include "image.h"
#include "pixel.h"
#include "color.h"
// =============================
// Input Modules
// =============================
#include "serial.h"
#include "camera.h"
#include "ov5640.h"
#include "fonts.h"

// =============================
// Output Modules
// =============================
#include "display.h"
#include "rk043fn48h.h"

// Add output includes if necessary

#ifdef __cplusplus
}
#endif

#endif // EMBED_DIP_H
