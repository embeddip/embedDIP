# embedDIP

**embedDIP** is a portable, embedded digital image processing library written in C/C++ for use on microcontrollers like STM32 and ESP32. It provides foundational and advanced image processing functionality with support for camera input, display output, and PC communication.

---

## 📁 Directory Structure

| Folder         | Description                                                                 |
|----------------|-----------------------------------------------------------------------------|
| `board/`       | Board-specific drivers and implementations (e.g., STM32F7, ESP32).          |
| `core/`        | Core image processing utilities (image struct, memory manager, histogram).  |
| `device/`      | Peripheral device drivers (e.g., OV5640 camera, LCD, UART, etc.).           |
| `imgproc/`     | Processing modules (e.g., color conversion, FFT, filtering).                |
| `samples/`     | Example projects and demos for testing features on hardware.                |
| `tests/`       | Unit tests for core and HAL modules.                                        |
| `wrapper/`     | C++ class wrappers for high-level MCU applications.                         |

---

## 🛠️ Features

- Camera support (OV5640, ESP-EYE)
- Display output (RGB565 via LTDC/DMA2D)
- Color format conversion (RGB888, RGB565, YUV, HSV, Grayscale)
- Histogram, Thresholding, Gamma Correction
- Template Matching & Segmentation
- Frequency domain filtering using FFT
- PC-to-MCU UART communication (image, 1D signal)

---

## 🚀 Getting Started

### Requirements

- STM32CubeIDE or ESP-IDF/Arduino
- CMSIS-DSP or ESP-DSP (for FFT/optimized math)
- UART & display-compatible MCU board (e.g., STM32F746G-DISCO)

### Add as Submodule

```bash
git submodule add https://github.com/EmbedDIP/embedDIP libraries/embedDIP
git submodule update --init --recursive
```

### Include in Your Project

In your main CMakeLists.txt or Arduino `.ino` project:

```cpp
#include "embedDIP.h"
```

Use the provided `createImage()`, `cvtColor()`, `fft()`, etc. APIs.

---

## 📸 Examples

Check the `examples/` directory for examples.
---

## 📜 License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## 🙋‍♂️ Author

Developed and maintained by [Ozan Durgut](https://github.com/ozan956) for embedded systems and digital image processing research.

Feel free to fork, contribute, or suggest enhancements via [issues](https://github.com/EmbedDIP/embedDIP/issues).

---

> © 2024–2025 [EmbedDIP](https://github.com/EmbedDIP).
