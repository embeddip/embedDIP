# Example 03: HOST Camera Input and Display Output

This example demonstrates the complete image processing pipeline on PC (HOST platform):
- File-based camera input (simulates hardware camera)
- Image processing (threshold operation)
- File-based display output (simulates hardware display)

This is particularly useful for:
- Algorithm development without hardware
- Automated testing of image pipelines
- Debugging image processing logic
- CI/CD validation

## Building and Running

```bash
cd embedDIP/build
cmake .. -DEMBEDDIP_TARGET_PLATFORM=HOST -DEMBEDDIP_BUILD_EXAMPLES=ON
make
./examples/03_host_camera/example_03_host_camera
```

## Expected Output

```
=== embedDIP Example 03: HOST Camera and Display ===

1. Setting up test environment...
Creating test pattern: 320x240 grayscale image...
✓ Test pattern saved to camera_input.raw

2. Initializing HOST camera...
[HOST Camera] Initialized: input=camera_input.raw, format=0, resolution=0

3. Initializing HOST display...
[HOST Display] Initialized: output=display_output.raw

4. Creating image buffer (320x240)...
   ✓ Buffer allocated: 76800 bytes

5. Capturing image from camera...
[HOST Camera] Captured 320x240 image (76800 bytes) from camera_input.raw
   ✓ Image captured successfully

6. Processing image (threshold at 128)...
   ✓ Threshold applied: 38400 dark pixels, 38400 bright pixels

7. Sending image to display...
[HOST Display] Wrote 320x240 image (76800 bytes) to display_output.raw
   ✓ Image displayed successfully

8. Verifying output file...
   ✓ Output file exists: display_output.raw (76800 bytes)

9. Cleaning up...
   ✓ All resources released

=== Example completed successfully! ===
```

## How It Works

### 1. Camera Simulation (File Input)

The HOST camera reads raw pixel data from a file:

```c
// Set input file
setenv("EMBEDDIP_CAMERA_INPUT", "camera_input.raw", 1);

// Initialize camera
CameraConfig config = {
    .format = IMAGE_FORMAT_GRAYSCALE,
    .resolution = IMAGE_RES_CUSTOM
};
CAMERA_Init(&config);

// Capture from file
camera_capture(img);
```

**Behind the scenes**: `host_camera.c` reads raw bytes from the file into the image buffer.

### 2. Display Simulation (File Output)

The HOST display writes raw pixel data to a file:

```c
// Set output file
setenv("EMBEDDIP_DISPLAY_OUTPUT", "display_output.raw", 1);

// Initialize display
DISPLAY_Init();

// Write to file
DISPLAY_ShowImage(img);
```

**Behind the scenes**: `host_display.c` writes the image buffer directly to disk.

### 3. File Format

Raw image files store pixels sequentially without headers:

```
For 320x240 grayscale:
  Byte 0:     Pixel [0,0]
  Byte 1:     Pixel [1,0]
  Byte 319:   Pixel [319,0]
  Byte 320:   Pixel [0,1]
  ...
  Byte 76799: Pixel [319,239]
```

## Viewing Output Images

### Using Python + PIL

```bash
python3 << EOF
import numpy as np
from PIL import Image

# Read raw file
data = np.fromfile('display_output.raw', dtype=np.uint8)
img = data.reshape(240, 320)  # height x width

# Save as PNG
Image.fromarray(img).save('output.png')
print('Saved output.png')
EOF
```

### Using ImageMagick

```bash
# Convert raw grayscale to PNG
convert -size 320x240 -depth 8 gray:display_output.raw output.png
```

### Using GIMP

1. Open GIMP
2. File → Open → Select `display_output.raw`
3. Set: Width=320, Height=240, Image Type=Gray, Offset=0
4. Export as PNG

## Using Real Images as Input

### Convert PNG to Raw Format

```bash
# Using ImageMagick
convert input.png -resize 320x240! -colorspace Gray -depth 8 gray:camera_input.raw

# Using Python
python3 << EOF
from PIL import Image
import numpy as np

img = Image.open('input.png')
img = img.resize((320, 240))
img = img.convert('L')  # Grayscale
np.array(img).tofile('camera_input.raw')
EOF
```

### Run with Custom Input

```bash
# Set environment variable
export EMBEDDIP_CAMERA_INPUT="my_custom_image.raw"
export EMBEDDIP_DISPLAY_OUTPUT="my_processed_output.raw"

# Run example
./example_03_host_camera
```

## Integration with CI/CD

This pattern enables automated testing:

```yaml
# .github/workflows/test-pipeline.yml
- name: Run Image Pipeline Test
  run: |
    # Generate test input
    python3 generate_test_pattern.py

    # Run embedDIP pipeline
    ./examples/03_host_camera/example_03_host_camera

    # Verify output
    python3 verify_output.py display_output.raw
```

## Porting to Hardware

The same code structure works on STM32/ESP32:

| Platform | Camera Source | Display Target |
|----------|--------------|----------------|
| **HOST** | File read | File write |
| **STM32F7** | DCMI → OV5640 | LTDC → LCD |
| **ESP32** | I2S → OV2640 | SPI → TFT |

**No application code changes needed!** Just recompile with `-DEMBEDDIP_TARGET_PLATFORM=STM32F7`.

## Next Steps

- Modify threshold value and observe results
- Try different image processing operations (filtering, edge detection)
- Use real photographs as input
- Build multi-stage processing pipelines
