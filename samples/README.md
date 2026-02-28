# embedDIP Test Samples

This directory contains test images and sample data for developing and testing image processing algorithms.

## Sample Images

| File | Size | Description | Use Case |
|------|------|-------------|----------|
| `boat.png` | 44KB | Grayscale boat scene | Edge detection, filtering |
| `cameraman.tif` | 257KB | Classic cameraman test image | General purpose testing |
| `cat.png` | 35KB | Cat photo | Object detection, segmentation |
| `city.png` | 55KB | Urban scene | Complex scene processing |
| `landscape.png` | 71KB | Natural landscape | Color processing, HDR |
| `mandrill.png` | 300KB | Color mandrill face | Color conversion, detail |
| `mandrill_grayscale.png` | 70KB | Grayscale mandrill | Grayscale algorithms |
| `plane.png` | 53KB | Airplane image | Motion detection |
| `tulips.png` | 268KB | Flower photograph | Color analysis |
| `tree_crowns.png` | 3.3MB | High-resolution forest | Large image handling |

## Test Data

| File | Size | Description |
|------|------|-------------|
| `test_image_data.h` | 605KB | Embedded 272x470 grayscale array | Microcontroller testing without filesystem |

## Usage

### For PC/HOST Testing

```c
#include "embedDIP.h"
#include <stdio.h>

// Load from PNG/TIF using your preferred image loader
// (e.g., stb_image, OpenCV, libpng)
```

### For Embedded Testing (No Filesystem)

If you need embedded test data without filesystem access:

```c
// Include the header with test data
#include "samples/test_image_data.h"

// Create image from embedded data
Image* img = NULL;
embeddip_status_t status = createImageWH(
    IMAGE_WIDTH,  // 272
    IMAGE_HEIGHT, // 470
    IMAGE_FORMAT_GRAYSCALE,
    &img
);

if (status == EMBEDDIP_OK) {
    // Copy embedded data to image
    memcpy(img->channels[0]->data, image_data, IMAGE_WIDTH * IMAGE_HEIGHT);

    // Process the image...

    // Clean up
    freeImage(&img);
}
```

## Notes

- **Large Files**: `tree_crowns.png` (3.3MB) is too large for most embedded systems. Use for HOST testing only.
- **Embedded Use**: For microcontrollers, prefer small images (< 100KB) or use `test_image_data.h` for testing without external storage.
- **Formats**: Use appropriate image loading libraries for your platform:
  - **STM32**: JPEG middleware (already included), or custom loaders
  - **ESP32**: ESP32 image decoders
  - **HOST**: stb_image, OpenCV, libpng, libjpeg

## Adding Your Own Test Images

1. Keep files small (< 500KB) for repository size
2. Use standard formats (PNG, JPEG)
3. Add description to this README
4. Consider Git LFS for files > 1MB

## License

Test images are provided for development and testing purposes. Original sources:
- Public domain test images
- Standard image processing datasets
