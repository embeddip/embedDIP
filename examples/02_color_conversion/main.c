/* ========================================================================== */
/*  Example 02: Color Space Conversions                                      */
/*  Demonstrates RGB, HSV, YUV, and Grayscale conversions                    */
/*  SPDX-License-Identifier: MIT                                              */
/* ========================================================================== */

#include "embedDIP.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    printf("=== embedDIP Example 02: Color Space Conversions ===\n\n");

    embeddip_status_t status;

    /* ===== 1. Create RGB888 Source Image ===== */
    printf("1. Creating 160x120 RGB888 source image...\n");
    Image *img_rgb888 = NULL;
    status = createImageWH(160, 120, IMAGE_FORMAT_RGB888, &img_rgb888);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create RGB888 image: %s\n", embeddip_status_str(status));
        return EXIT_FAILURE;
    }

    // Fill with colored pattern (RGB gradient)
    printf("   Filling with RGB gradient pattern...\n");
    for (int y = 0; y < img_rgb888->height; y++) {
        for (int x = 0; x < img_rgb888->width; x++) {
            int idx = (y * img_rgb888->width + x) * 3;
            img_rgb888->channels[0]->data[idx + 0] = (x * 255) / img_rgb888->width;  // R
            img_rgb888->channels[0]->data[idx + 1] = (y * 255) / img_rgb888->height; // G
            img_rgb888->channels[0]->data[idx + 2] = 128;                             // B
        }
    }
    printf("   ✓ Source image created: %dx%d RGB888\n", img_rgb888->width, img_rgb888->height);

    /* ===== 2. Convert RGB888 to RGB565 ===== */
    printf("\n2. Converting RGB888 → RGB565...\n");
    Image *img_rgb565 = NULL;
    status = createImageWH(img_rgb888->width, img_rgb888->height, IMAGE_FORMAT_RGB565, &img_rgb565);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create RGB565 image: %s\n", embeddip_status_str(status));
        freeImage(&img_rgb888);
        return EXIT_FAILURE;
    }

    status = cvtColor(img_rgb888, img_rgb565, COLOR_RGB888_TO_RGB565);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Conversion failed: %s\n", embeddip_status_str(status));
        freeImage(&img_rgb888);
        freeImage(&img_rgb565);
        return EXIT_FAILURE;
    }
    printf("   ✓ Converted to RGB565 (16-bit per pixel)\n");

    /* ===== 3. Convert RGB888 to Grayscale ===== */
    printf("\n3. Converting RGB888 → Grayscale...\n");
    Image *img_gray = NULL;
    status = createImageWH(img_rgb888->width, img_rgb888->height, IMAGE_FORMAT_GRAYSCALE, &img_gray);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create grayscale image: %s\n", embeddip_status_str(status));
        freeImage(&img_rgb888);
        freeImage(&img_rgb565);
        return EXIT_FAILURE;
    }

    status = cvtColor(img_rgb888, img_gray, COLOR_RGB888_TO_GRAY);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Conversion failed: %s\n", embeddip_status_str(status));
        freeImage(&img_rgb888);
        freeImage(&img_rgb565);
        freeImage(&img_gray);
        return EXIT_FAILURE;
    }
    printf("   ✓ Converted to Grayscale (8-bit per pixel)\n");

    /* ===== 4. Sample Pixel Values ===== */
    printf("\n4. Comparing pixel values (center pixel)...\n");
    int center_x = img_rgb888->width / 2;
    int center_y = img_rgb888->height / 2;

    // RGB888
    int rgb888_idx = (center_y * img_rgb888->width + center_x) * 3;
    uint8_t r = img_rgb888->channels[0]->data[rgb888_idx + 0];
    uint8_t g = img_rgb888->channels[0]->data[rgb888_idx + 1];
    uint8_t b = img_rgb888->channels[0]->data[rgb888_idx + 2];
    printf("   RGB888 [%d,%d]: R=%d G=%d B=%d\n", center_x, center_y, r, g, b);

    // RGB565
    int rgb565_idx = (center_y * img_rgb565->width + center_x) * 2;
    uint16_t rgb565_val =
        (img_rgb565->channels[0]->data[rgb565_idx] << 8) | img_rgb565->channels[0]->data[rgb565_idx + 1];
    uint8_t r5 = (rgb565_val >> 11) & 0x1F;
    uint8_t g6 = (rgb565_val >> 5) & 0x3F;
    uint8_t b5 = rgb565_val & 0x1F;
    printf("   RGB565 [%d,%d]: R=%d (5-bit) G=%d (6-bit) B=%d (5-bit)\n", center_x, center_y, r5, g6, b5);

    // Grayscale
    int gray_idx = center_y * img_gray->width + center_x;
    uint8_t gray_val = img_gray->channels[0]->data[gray_idx];
    printf("   Gray   [%d,%d]: %d (0.299*R + 0.587*G + 0.114*B)\n", center_x, center_y, gray_val);

    /* ===== 5. Memory Comparison ===== */
    printf("\n5. Memory usage comparison:\n");
    size_t rgb888_size = img_rgb888->width * img_rgb888->height * 3;
    size_t rgb565_size = img_rgb565->width * img_rgb565->height * 2;
    size_t gray_size = img_gray->width * img_gray->height;

    printf("   RGB888:    %zu bytes (100%%)\n", rgb888_size);
    printf("   RGB565:    %zu bytes (%.1f%% of RGB888)\n",
           rgb565_size,
           (float)rgb565_size / rgb888_size * 100);
    printf("   Grayscale: %zu bytes (%.1f%% of RGB888)\n", gray_size, (float)gray_size / rgb888_size * 100);

    /* ===== 6. Cleanup ===== */
    printf("\n6. Cleaning up...\n");
    freeImage(&img_rgb888);
    freeImage(&img_rgb565);
    freeImage(&img_gray);
    printf("   ✓ All images freed\n");

    printf("\n=== Example completed successfully! ===\n");
    return EXIT_SUCCESS;
}
