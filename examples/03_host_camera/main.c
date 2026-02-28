/* ========================================================================== */
/*  Example 03: HOST Camera Input and Display Output                         */
/*  Demonstrates file-based image I/O for PC testing                         */
/*  SPDX-License-Identifier: MIT                                              */
/* ========================================================================== */

#include "embedDIP.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a test pattern and save to file
 */
static embeddip_status_t create_test_pattern(const char *filename, int width, int height)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to create test file: %s\n", filename);
        return EMBEDDIP_ERROR_DEVICE_ERROR;
    }

    printf("Creating test pattern: %dx%d grayscale image...\n", width, height);

    // Create checkerboard pattern
    int block_size = 16;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int block_x = x / block_size;
            int block_y = y / block_size;
            uint8_t value = ((block_x + block_y) % 2) ? 255 : 64;
            fwrite(&value, 1, 1, fp);
        }
    }

    fclose(fp);
    printf("✓ Test pattern saved to %s\n", filename);
    return EMBEDDIP_OK;
}

int main(void)
{
    printf("=== embedDIP Example 03: HOST Camera and Display ===\n\n");

    embeddip_status_t status;
    const char *input_file = "camera_input.raw";
    const char *output_file = "display_output.raw";
    const int width = 320;
    const int height = 240;

    /* ===== 1. Create Test Input File ===== */
    printf("1. Setting up test environment...\n");
    status = create_test_pattern(input_file, width, height);
    if (status != EMBEDDIP_OK) {
        return EXIT_FAILURE;
    }

    /* ===== 2. Initialize Camera (File Input) ===== */
    printf("\n2. Initializing HOST camera...\n");
    CameraConfig camera_config = {.format = IMAGE_FORMAT_GRAYSCALE, .resolution = IMAGE_RES_CUSTOM};

    // Set input file via environment variable (simulates camera source)
    setenv("EMBEDDIP_CAMERA_INPUT", input_file, 1);

    status = CAMERA_Init(&camera_config);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Camera init failed: %s\n", embeddip_status_str(status));
        return EXIT_FAILURE;
    }

    /* ===== 3. Initialize Display (File Output) ===== */
    printf("\n3. Initializing HOST display...\n");

    // Set output file via environment variable (simulates display target)
    setenv("EMBEDDIP_DISPLAY_OUTPUT", output_file, 1);

    status = DISPLAY_Init();
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Display init failed: %s\n", embeddip_status_str(status));
        CAMERA_DeInit();
        return EXIT_FAILURE;
    }

    /* ===== 4. Create Image Buffer ===== */
    printf("\n4. Creating image buffer (%dx%d)...\n", width, height);
    Image *img = NULL;
    status = createImageWH(width, height, IMAGE_FORMAT_GRAYSCALE, &img);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create image: %s\n", embeddip_status_str(status));
        DISPLAY_DeInit();
        CAMERA_DeInit();
        return EXIT_FAILURE;
    }
    printf("   ✓ Buffer allocated: %zu bytes\n", (size_t)(width * height));

    /* ===== 5. Capture Image from Camera ===== */
    printf("\n5. Capturing image from camera...\n");
    status = camera_capture(img);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Capture failed: %s\n", embeddip_status_str(status));
        freeImage(&img);
        DISPLAY_DeInit();
        CAMERA_DeInit();
        return EXIT_FAILURE;
    }
    printf("   ✓ Image captured successfully\n");

    /* ===== 6. Process Image (Simple Threshold) ===== */
    printf("\n6. Processing image (threshold at 128)...\n");
    int pixels_below = 0, pixels_above = 0;
    for (int i = 0; i < width * height; i++) {
        uint8_t pixel = img->channels[0]->data[i];
        if (pixel < 128) {
            pixels_below++;
            img->channels[0]->data[i] = 0;  // Black
        } else {
            pixels_above++;
            img->channels[0]->data[i] = 255;  // White
        }
    }
    printf("   ✓ Threshold applied: %d dark pixels, %d bright pixels\n", pixels_below, pixels_above);

    /* ===== 7. Display Image ===== */
    printf("\n7. Sending image to display...\n");
    status = DISPLAY_ShowImage(img);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Display failed: %s\n", embeddip_status_str(status));
        freeImage(&img);
        DISPLAY_DeInit();
        CAMERA_DeInit();
        return EXIT_FAILURE;
    }
    printf("   ✓ Image displayed successfully\n");

    /* ===== 8. Verify Output ===== */
    printf("\n8. Verifying output file...\n");
    FILE *fp = fopen(output_file, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fclose(fp);
        printf("   ✓ Output file exists: %s (%ld bytes)\n", output_file, file_size);
        printf("   Tip: View with: python3 -c \"import numpy as np; from PIL import Image; "
               "img=np.fromfile('%s',dtype=np.uint8).reshape(%d,%d); "
               "Image.fromarray(img).save('output.png')\"\n",
               output_file,
               height,
               width);
    } else {
        fprintf(stderr, "   ⚠ Output file not found\n");
    }

    /* ===== 9. Cleanup ===== */
    printf("\n9. Cleaning up...\n");
    freeImage(&img);
    DISPLAY_DeInit();
    CAMERA_DeInit();
    printf("   ✓ All resources released\n");

    printf("\n=== Example completed successfully! ===\n");
    printf("\nGenerated files:\n");
    printf("  - %s (input, checkerboard pattern)\n", input_file);
    printf("  - %s (output, thresholded result)\n", output_file);

    return EXIT_SUCCESS;
}
