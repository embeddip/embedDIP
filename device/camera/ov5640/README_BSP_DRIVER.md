# OV5640 Camera Driver - STM BSP Integration

## Overview

This directory now contains the official **STMicroelectronics BSP (Board Support Package) driver** for the OV5640 camera sensor, integrated with the embedDIP camera interface.

## Architecture

### Files Structure

```
ov5640/
├── stm32_ov5640.c      # embedDIP wrapper (adapter layer)
├── ov5640.c            # STM BSP driver (official implementation)
├── ov5640.h            # STM BSP driver API
├── ov5640_reg.c        # Register access functions
├── ov5640_reg.h        # Register definitions
└── README_BSP_DRIVER.md # This file
```

### Component Roles

#### 1. **STM BSP Driver** (`ov5640.c`, `ov5640.h`)
- Official driver from STMicroelectronics
- Provides high-level camera control functions:
  - `OV5640_Init()` - Initialize camera with resolution and pixel format
  - `OV5640_SetResolution()` - Change resolution dynamically
  - `OV5640_SetPixelFormat()` - Change pixel format (RGB565, YUV422, etc.)
  - `OV5640_EnableDVPMode()` - Enable parallel DVP interface
  - `OV5640_Start()` / `OV5640_Stop()` - Control camera streaming
- Uses object-oriented design with `OV5640_Object_t`

#### 2. **Register Layer** (`ov5640_reg.c`, `ov5640_reg.h`)
- Low-level register read/write functions
- Complete register map definitions
- Hardware abstraction for I2C communication

#### 3. **embedDIP Wrapper** (`stm32_ov5640.c`)
- Adapts BSP driver to embedDIP `camera_t` interface
- Implements I2C communication using STM32 HAL
- Maps between embedDIP enums and BSP enums:
  - `IMAGE_RES_WQVGA` ↔ `OV5640_R480x272`
  - `IMAGE_FORMAT_RGB565` ↔ `OV5640_RGB565`
- Manages camera power control

## Key Improvements

### Proper Separation of Concerns
- **Resolution** and **pixel format** are now handled separately (as they should be)
- Resolution: Controls image dimensions (480x272, 640x480, etc.)
- Format: Controls color encoding (RGB565, YUV422, Grayscale)

### Official STM BSP Driver
- Professionally maintained code from STMicroelectronics
- Comprehensive register configurations
- Proper error handling and status codes
- Support for all OV5640 features:
  - Multiple resolutions (160x120 to 800x480)
  - Multiple pixel formats (RGB565, RGB888, YUV422, Y8, JPEG)
  - Camera effects (brightness, contrast, color effects)
  - Mirror/flip configurations
  - Zoom control
  - Night mode

### Clean API
The embedDIP interface remains unchanged:
```c
camera->init(IMAGE_RES_WQVGA, IMAGE_FORMAT_RGB565);
camera->capture(SINGLE, inImg);
camera->stop();
camera->setRes(IMAGE_RES_VGA);
```

## How It Works

### Initialization Flow

1. **embedDIP API Call**
   ```c
   camera->init(IMAGE_RES_WQVGA, IMAGE_FORMAT_RGB565);
   ```

2. **Wrapper Layer** (`stm32_ov5640.c`)
   - Powers up the camera
   - Registers I2C I/O functions with BSP driver
   - Maps embedDIP enums to BSP enums
   - Calls BSP driver initialization

3. **BSP Driver** (`ov5640.c`)
   - Writes common initialization sequence (PLL, timing, ISP settings)
   - Writes resolution-specific registers
   - Writes pixel format registers (FORMAT_CTRL00, FORMAT_MUX_CTRL)
   - Enables DVP mode for parallel interface

4. **Hardware**
   - Camera sensor configured via I2C
   - Outputs video data via DCMI interface

### Register Configuration

The BSP driver uses separate arrays for each concern:

**Pixel Format** (in `OV5640_SetPixelFormat`):
- RGB565: `{0x4300, 0x6F}, {0x501F, 0x01}`
- YUV422: `{0x4300, 0x30}, {0x501F, 0x00}`
- RGB888: `{0x4300, 0x23}, {0x501F, 0x01}`
- Y8 (Grayscale): `{0x4300, 0x10}, {0x501F, 0x00}`

**Resolution** (in `OV5640_Init`):
- Each resolution has its own register array
- Configures output size, timing, windowing, etc.

## Usage Example

```c
#include "device/camera/camera.h"
#include "core/image.h"

// Initialize camera subsystem
camera_t *camera = &stm32_ov5640;
Image *img = NULL;

// Create image buffer
createImage(IMAGE_RES_WQVGA, IMAGE_FORMAT_RGB565, &img);

// Initialize camera
if (camera->init(IMAGE_RES_WQVGA, IMAGE_FORMAT_RGB565) == 0) {
    // Capture single frame
    camera->capture(SINGLE, img);

    // Wait for capture complete (via DCMI interrupt)
    HAL_Delay(100);

    // Process or transmit image...
    serial->send(img);

    // Stop camera
    camera->stop();
}

// Change resolution dynamically
camera->setRes(IMAGE_RES_VGA);
```

## Supported Configurations

### Resolutions
- `IMAGE_RES_QQVGA` (160x120) → `OV5640_R160x120`
- `IMAGE_RES_QVGA` (320x240) → `OV5640_R320x240`
- `IMAGE_RES_WQVGA` (480x272) → `OV5640_R480x272`
- `IMAGE_RES_VGA` (640x480) → `OV5640_R640x480`

### Pixel Formats
- `IMAGE_FORMAT_GRAYSCALE` → `OV5640_Y8`
- `IMAGE_FORMAT_RGB565` → `OV5640_RGB565`
- `IMAGE_FORMAT_RGB888` → `OV5640_RGB888`

## Hardware Connections

- **I2C3** - Camera control (address: 0x78)
- **DCMI** - Parallel video data interface
- **GPIO PJ14** - Camera power control
- **DMA** - High-speed image data transfer

## Build System

The BSP driver files are automatically included in the CMake build:

**embedDIP/CMakeLists.txt**:
```cmake
set(DEVICE_SOURCES
    ${DEVICE_COMMON_SOURCES}
    device/camera/ov5640/stm32_ov5640.c
    device/camera/ov5640/ov5640.c        # BSP driver
    device/camera/ov5640/ov5640_reg.c    # Register layer
    device/display/rk043fn48h/stm32_rk043fn48h.c
    device/serial/stm32_uart/stm32_uart.c
)
```

## Troubleshooting

### Camera Not Detected
- Check I2C connections and pull-up resistors
- Verify camera power (GPIO PJ14)
- Check camera I2C address (should be 0x78)
- Verify `OV5640_ReadID()` returns `0x5640`

### Image Issues
- Ensure DCMI clock and DMA are properly configured
- Check image buffer size matches resolution
- Verify pixel format matches DCMI configuration
- Wait sufficient time after `camera->capture()` for DMA completion

### Build Issues
- Ensure all 5 files are included in your build system
- Add `device/camera/ov5640` to include paths
- Verify CMSIS headers are accessible

## References

- **OV5640 Datasheet**: OmniVision OV5640 CMOS Image Sensor
- **STM32F746 Reference Manual**: RM0385 (DCMI chapter)
- **STM32 BSP Drivers**: Official STMicroelectronics BSP package
- **AN5020**: Digital camera interface (DCMI) on STM32 MCUs

## Migration Notes

If you were using the old driver that embedded format registers in resolution arrays:

**Old approach** (format in resolution arrays):
- Format was hardcoded in each resolution array
- Couldn't change format without changing resolution
- Register 0x501F was sometimes 0x03 (incorrect for RGB)

**New approach** (proper separation):
- Format and resolution are independent
- Can change either one dynamically
- Uses correct register values from official BSP
- More maintainable and extensible

---

**Author**: embedDIP Integration Layer
**Date**: 2024
**License**: Compatible with STM32 BSP License (see individual file headers)
