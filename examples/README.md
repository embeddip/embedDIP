# embedDIP Examples

This directory contains example programs demonstrating embedDIP library usage.

## Status

⚠️ **Note**: These examples are currently being updated to match the embedDIP API.

The examples were created with placeholder code and need the following updates:

### Required API Updates

1. **Image pixel access**: Use `(uint8_t*)img->pixels` instead of `img->channels[0]->data`
2. **Image cleanup**: Use `deleteImage(img)` instead of `freeImage(&img)`
3. **Color conversion enums**: Use `CVT_RGB888_TO_RGB565` instead of `COLOR_RGB888_TO_RGB565`
4. **Channel count**: Use `image_num_channels(img->format)` instead of `img->numChals`
5. **UART functions**: These may not be available in the current API - examples need to be simplified or use printf directly

### Correct API Usage

```c
// Creating an image
Image *img = NULL;
embeddip_status_t status = createImageWH(320, 240, IMAGE_FORMAT_GRAYSCALE, &img);
if (status != EMBEDDIP_OK) {
    // Handle error
    return -1;
}

// Accessing pixels (grayscale example)
uint8_t *pixels = (uint8_t*)img->pixels;
for (int i = 0; i < img->width * img->height; i++) {
    pixels[i] = 128;  // Set to gray
}

// Cleanup
deleteImage(img);
```

### Color Conversion

```c
// RGB888 to RGB565
Image *src_rgb888 = NULL;
Image *dst_rgb565 = NULL;

createImageWH(320, 240, IMAGE_FORMAT_RGB888, &src_rgb888);
createImageWH(320, 240, IMAGE_FORMAT_RGB565, &dst_rgb565);

// Convert
cvtColor(src_rgb888, dst_rgb565, CVT_RGB888_TO_RGB565);

// Cleanup
deleteImage(src_rgb888);
deleteImage(dst_rgb565);
```

## Building

To disable examples during build:

```bash
cmake .. -DEMBEDDIP_TARGET_PLATFORM=HOST -DEMBEDDIP_BUILD_EXAMPLES=OFF
```

To enable once examples are fixed:

```bash
cmake .. -DEMBEDDIP_TARGET_PLATFORM=HOST -DEMBEDDIP_BUILD_EXAMPLES=ON
```

## Contributing

If you'd like to help update these examples to match the current API, please see [CONTRIBUTING.md](../CONTRIBUTING.md) in the parent directory.

## See Also

- [Unit Tests](../tests/) - Working examples of API usage
- [Library Documentation](../README.md)
- [API Headers](../core/image.h, ../imgproc/color.h)
