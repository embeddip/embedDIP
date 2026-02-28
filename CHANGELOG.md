# Changelog

All notable changes to embedDIP will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Integration tests for hardware platforms
- Code coverage reporting (target > 70%)
- Performance benchmarking suite
- Doxygen API documentation generation

---

## [0.1.0] - 2026-02-21

### 🎉 First Official Release

This is the first production-ready release of embedDIP, a portable embedded digital image processing library for microcontrollers (STM32F7, ESP32) and PC platforms.

### Added

#### Core Features
- **Unified error handling system** with `embeddip_status_t` enum and descriptive error codes
- **Multi-platform CMake build system** with support for STM32F7, ESP32, and HOST (x86) targets
- **C++ RAII wrappers** over C API for exception-safe resource management
- **Unit test suite** with 44 tests using Unity framework
- **CI/CD pipeline** with GitHub Actions for automated testing and validation

#### Image Processing
- Color space conversions (RGB888, RGB565, YUV422, HSV, Grayscale)
- Spatial filtering with custom kernels
- Histogram computation and equalization
- FFT-based frequency domain processing
- Template matching and segmentation
- Gamma correction and thresholding

#### Hardware Support
- **OV5640 camera driver** (STM32F7) with complete initialization and format configuration
- **OV2640 camera driver** (ESP32)
- **RK043FN48H display driver** (STM32F7) with LTDC/DMA2D acceleration
- **UART communication** for PC-to-MCU image transfer
- **DCMI interface** with DMA for zero-copy camera capture

#### Platform Abstraction
- Board-specific memory managers (STM32F7: CCM RAM, ESP32: PSRAM, HOST: malloc)
- FFT acceleration (STM32F7: CMSIS-DSP, ESP32: ESP-DSP, HOST: standard implementations)
- Portable device interfaces for camera, display, and serial communication

### Changed

#### Breaking Changes
- **`createImage()`** signature changed to return status code with out parameter
  - Old: `Image* createImage(ImageResolution resolution, ImageFormat format)`
  - New: `embeddip_status_t createImage(ImageResolution resolution, ImageFormat format, Image** out_image)`
  - Migration: Use `createImage_legacy()` wrapper or update to new API

- **`createImageWH()`** signature changed to return status code with out parameter
  - Old: `Image* createImageWH(int width, int height, ImageFormat format)`
  - New: `embeddip_status_t createImageWH(int width, int height, ImageFormat format, Image** out_image)`
  - Migration: Use `createImageWH_legacy()` wrapper or update to new API

- **`createChals()`** now returns `embeddip_status_t` instead of `bool`
  - Check for `EMBEDDIP_OK` instead of `true`

- **Color conversion functions** (`cvtColor`, `hueThreshold`, `inRange`) now return status codes
  - Add error checking after each call

- **C++ wrapper classes** now throw exceptions instead of calling `assert()`
  - `ImageWrapper` constructors throw on failure
  - `SerialWrapper` methods throw `std::runtime_error`
  - `MemoryManager` throws `std::bad_alloc()` on OOM

### Fixed
- **Camera initialization**: Complete OV5640 power-up sequence and register configuration
- **Memory management**: Proper cleanup on error paths, preventing leaks
- **Input validation**: Added bounds checking to all public APIs
- **Assert removal**: Replaced ~30 assert() calls in public APIs with proper error handling
- **Linker errors**: Fixed missing `-fno-exceptions` flag for embedded C++ builds

### Improved
- **CMake structure**: Explicit file lists instead of GLOB, proper install targets, find_package() support
- **Error messages**: Human-readable error descriptions via `embeddip_status_str()`
- **Code organization**: Clean separation of core, imgproc, device, board, and wrapper modules
- **Documentation**: Comprehensive README, IMPROVEMENTS.md with implementation details

### Removed
- **Assert-based error handling**: Replaced with graceful error returns in release builds
- **Hardcoded test data**: Moved `image_data.h` (605KB) to `samples/` directory
- **Mixed language comments**: Standardized on English throughout codebase

---

## Project Information

### License
This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

### Version Support
- **C Standard**: C11 or higher
- **C++ Standard**: C++17 or higher
- **CMake**: 3.15 or higher
- **Platforms**: STM32F7, ESP32, x86/x64 (HOST)

### Build Instructions

#### For STM32F7
```bash
mkdir build && cd build
cmake .. -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake \
    -DEMBEDDIP_TARGET_PLATFORM=STM32F7 \
    -DCMAKE_BUILD_TYPE=Release
ninja
```

#### For ESP32
```bash
mkdir build && cd build
cmake .. \
    -DEMBEDDIP_TARGET_PLATFORM=ESP32 \
    -DCMAKE_BUILD_TYPE=Release
make
```

#### For HOST (x86 PC Testing)
```bash
mkdir build && cd build
cmake .. \
    -DEMBEDDIP_TARGET_PLATFORM=HOST \
    -DEMBEDDIP_BUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Debug
make
ctest --output-on-failure
```

### Migration Guide from Pre-1.0

**Old Code:**
```c
Image* img = createImage(IMAGE_RES_VGA, IMAGE_FORMAT_RGB565);
if (!img) {
    // Handle error
}
```

**New Code (Recommended):**
```c
Image* img = NULL;
embeddip_status_t status = createImage(IMAGE_RES_VGA, IMAGE_FORMAT_RGB565, &img);
if (status != EMBEDDIP_OK) {
    printf("Error: %s\n", embeddip_status_str(status));
    return status;
}
// Use img...
freeImage(&img);
```

**Or Use Legacy Wrapper (Temporary Compatibility):**
```c
Image* img = createImage_legacy(IMAGE_RES_VGA, IMAGE_FORMAT_RGB565);
if (!img) {
    // Handle error
}
```

### Contributors
- Ozan Durgut ([@ozan956](https://github.com/ozan956)) - Lead Developer
- EmbedDIP Project Team

### Links
- **Repository**: [github.com/EmbedDIP/embedDIP](https://github.com/EmbedDIP/embedDIP)
- **Issues**: [github.com/EmbedDIP/embedDIP/issues](https://github.com/EmbedDIP/embedDIP/issues)
- **Examples**: [github.com/EmbedDIP/examples-stm32](https://github.com/EmbedDIP/examples-stm32)

---

[Unreleased]: https://github.com/EmbedDIP/embedDIP/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/EmbedDIP/embedDIP/releases/tag/v0.1.0
