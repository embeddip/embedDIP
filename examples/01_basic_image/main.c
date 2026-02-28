/* ========================================================================== */
/*  Example 01: Basic Image Creation and Management                          */
/*  Demonstrates core embedDIP functionality                                  */
/*  SPDX-License-Identifier: MIT                                              */
/* ========================================================================== */

#include "embedDIP.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    printf("=== embedDIP Example 01: Basic Image Creation ===\n\n");

    // Initialize serial communication (HOST uses stdout)
    embeddip_status_t status = UART_Init(115200);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to initialize UART: %s\n", embeddip_status_str(status));
        return EXIT_FAILURE;
    }

    /* ===== 1. Create Image with Predefined Resolution ===== */
    printf("1. Creating VGA image (640x480) in RGB565 format...\n");
    Image *img_vga = NULL;
    status = createImage(IMAGE_RES_VGA, IMAGE_FORMAT_RGB565, &img_vga);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create image: %s\n", embeddip_status_str(status));
        return EXIT_FAILURE;
    }
    printf("   ✓ Created: %dx%d, format=%d, channels=%d\n",
           img_vga->width,
           img_vga->height,
           img_vga->format,
           img_vga->numChals);

    /* ===== 2. Create Image with Custom Dimensions ===== */
    printf("\n2. Creating custom 320x240 grayscale image...\n");
    Image *img_gray = NULL;
    status = createImageWH(320, 240, IMAGE_FORMAT_GRAYSCALE, &img_gray);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create custom image: %s\n", embeddip_status_str(status));
        freeImage(&img_vga);
        return EXIT_FAILURE;
    }
    printf("   ✓ Created: %dx%d, format=%d\n",
           img_gray->width,
           img_gray->height,
           img_gray->format);

    /* ===== 3. Fill Image with Pattern ===== */
    printf("\n3. Filling grayscale image with gradient pattern...\n");
    for (int y = 0; y < img_gray->height; y++) {
        for (int x = 0; x < img_gray->width; x++) {
            int idx = y * img_gray->width + x;
            // Create horizontal gradient
            img_gray->channels[0]->data[idx] = (uint8_t)((x * 255) / img_gray->width);
        }
    }
    printf("   ✓ Pattern applied (horizontal gradient 0-255)\n");

    /* ===== 4. Calculate Statistics ===== */
    printf("\n4. Calculating image statistics...\n");
    uint32_t sum = 0;
    uint8_t min_val = 255, max_val = 0;
    for (int i = 0; i < img_gray->width * img_gray->height; i++) {
        uint8_t pixel = img_gray->channels[0]->data[i];
        sum += pixel;
        if (pixel < min_val) min_val = pixel;
        if (pixel > max_val) max_val = pixel;
    }
    float mean = (float)sum / (img_gray->width * img_gray->height);
    printf("   Min: %d, Max: %d, Mean: %.2f\n", min_val, max_val, mean);

    /* ===== 5. Memory Information ===== */
    printf("\n5. Memory usage:\n");
    size_t vga_size = img_vga->width * img_vga->height * 2;  // RGB565 = 2 bytes/pixel
    size_t gray_size = img_gray->width * img_gray->height;   // Grayscale = 1 byte/pixel
    printf("   VGA RGB565:  %zu bytes (%.2f KB)\n", vga_size, vga_size / 1024.0f);
    printf("   320x240 Gray: %zu bytes (%.2f KB)\n", gray_size, gray_size / 1024.0f);
    printf("   Total:        %zu bytes (%.2f KB)\n",
           vga_size + gray_size,
           (vga_size + gray_size) / 1024.0f);

    /* ===== 6. Cleanup ===== */
    printf("\n6. Cleaning up resources...\n");
    freeImage(&img_vga);
    freeImage(&img_gray);
    UART_DeInit();
    printf("   ✓ All resources freed\n");

    printf("\n=== Example completed successfully! ===\n");
    return EXIT_SUCCESS;
}
