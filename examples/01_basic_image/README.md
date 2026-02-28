# Example 01: Basic Image Creation and Management

This example demonstrates the fundamental image operations in embedDIP:
- Creating images with predefined resolutions
- Creating images with custom dimensions
- Direct pixel manipulation
- Image statistics calculation
- Memory management

## Learning Objectives

1. **Image Creation API**: Using `createImage()` and `createImageWH()`
2. **Error Handling**: Checking return status codes
3. **Memory Access**: Direct pixel manipulation
4. **Resource Management**: Proper cleanup with `freeImage()`

## Building and Running

### For HOST (PC)

```bash
cd embedDIP
mkdir build && cd build
cmake .. -DEMBEDDIP_TARGET_PLATFORM=HOST -DEMBEDDIP_BUILD_EXAMPLES=ON
make
./examples/01_basic_image/example_01_basic_image
```

### Expected Output

```
=== embedDIP Example 01: Basic Image Creation ===

1. Creating VGA image (640x480) in RGB565 format...
   ✓ Created: 640x480, format=2, channels=1

2. Creating custom 320x240 grayscale image...
   ✓ Created: 320x240, format=0

3. Filling grayscale image with gradient pattern...
   ✓ Pattern applied (horizontal gradient 0-255)

4. Calculating image statistics...
   Min: 0, Max: 255, Mean: 127.50

5. Memory usage:
   VGA RGB565:  614400 bytes (600.00 KB)
   320x240 Gray: 76800 bytes (75.00 KB)
   Total:        691200 bytes (675.00 KB)

6. Cleaning up resources...
   ✓ All resources freed

=== Example completed successfully! ===
```

## Key Concepts

### Status Code Checking

```c
embeddip_status_t status = createImage(IMAGE_RES_VGA, IMAGE_FORMAT_RGB565, &img);
if (status != EMBEDDIP_OK) {
    fprintf(stderr, "Error: %s\n", embeddip_status_str(status));
    return EXIT_FAILURE;
}
```

Always check return values and use `embeddip_status_str()` for human-readable error messages.

### Image Structure

```c
typedef struct Image {
    int width;                  // Image width in pixels
    int height;                 // Image height in pixels
    ImageFormat format;         // Pixel format (RGB565, RGB888, Grayscale, etc.)
    uint8_t numChals;          // Number of channels
    Channel **channels;         // Array of channel pointers
} Image;
```

### Memory Layout

- **Grayscale**: 1 byte per pixel
- **RGB565**: 2 bytes per pixel (5 bits red, 6 bits green, 5 bits blue)
- **RGB888**: 3 bytes per pixel (8 bits per channel)

## Next Steps

- See [Example 02](../02_color_conversion/) for color space conversions
- See [Example 03](../03_host_camera/) for camera input and display output
