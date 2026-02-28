# Example 02: Color Space Conversions

This example demonstrates color space transformations in embedDIP:
- RGB888 to RGB565 conversion
- RGB to Grayscale conversion
- Memory efficiency comparisons
- Pixel format inspection

## Supported Color Conversions

| From | To | Conversion ID |
|------|-----|--------------|
| RGB888 | RGB565 | `COLOR_RGB888_TO_RGB565` |
| RGB888 | Grayscale | `COLOR_RGB888_TO_GRAY` |
| RGB565 | RGB888 | `COLOR_RGB565_TO_RGB888` |
| RGB565 | Grayscale | `COLOR_RGB565_TO_GRAY` |
| Grayscale | RGB888 | `COLOR_GRAY_TO_RGB888` |
| Grayscale | RGB565 | `COLOR_GRAY_TO_RGB565` |

## Building and Running

```bash
cd embedDIP/build
cmake .. -DEMBEDDIP_TARGET_PLATFORM=HOST -DEMBEDDIP_BUILD_EXAMPLES=ON
make
./examples/02_color_conversion/example_02_color_conversion
```

## Expected Output

```
=== embedDIP Example 02: Color Space Conversions ===

1. Creating 160x120 RGB888 source image...
   Filling with RGB gradient pattern...
   ✓ Source image created: 160x120 RGB888

2. Converting RGB888 → RGB565...
   ✓ Converted to RGB565 (16-bit per pixel)

3. Converting RGB888 → Grayscale...
   ✓ Converted to Grayscale (8-bit per pixel)

4. Comparing pixel values (center pixel)...
   RGB888 [80,60]: R=127 G=127 B=128
   RGB565 [80,60]: R=15 (5-bit) G=31 (6-bit) B=16 (5-bit)
   Gray   [80,60]: 127 (0.299*R + 0.587*G + 0.114*B)

5. Memory usage comparison:
   RGB888:    57600 bytes (100%)
   RGB565:    38400 bytes (66.7% of RGB888)
   Grayscale: 19200 bytes (33.3% of RGB888)

6. Cleaning up...
   ✓ All images freed

=== Example completed successfully! ===
```

## Key Concepts

### Color Conversion API

```c
embeddip_status_t cvtColor(
    const Image *src,      // Source image
    Image *dst,            // Destination image (must be pre-allocated)
    ColorConversion code   // Conversion type
);
```

### RGB565 Format

RGB565 packs RGB into 16 bits:
- **Red**: 5 bits (0-31)
- **Green**: 6 bits (0-63) - more bits for human eye sensitivity
- **Blue**: 5 bits (0-31)

```
15 14 13 12 11 | 10 9 8 7 6 5 | 4 3 2 1 0
   Red (5)     |  Green (6)   |  Blue (5)
```

### Grayscale Conversion

Uses ITU-R BT.601 standard weights:
```
Gray = 0.299*R + 0.587*G + 0.114*B
```

Green has the highest weight because human eyes are most sensitive to green light.

## Memory Efficiency

For a 640x480 image:

| Format | Bytes/Pixel | Total Memory | vs RGB888 |
|--------|-------------|--------------|-----------|
| RGB888 | 3 | 921,600 bytes | 100% |
| RGB565 | 2 | 614,400 bytes | 66.7% |
| Grayscale | 1 | 307,200 bytes | 33.3% |

**Tip**: Use RGB565 for embedded displays (saves 33% memory). Use Grayscale for image processing algorithms when color is not needed.

## Next Steps

- See [Example 03](../03_host_camera/) for complete camera-to-display pipeline
- Explore `imgproc/color.h` for more conversion functions
