/* ========================================================================== */
/*  File: host_display.c                                                      */
/*  Brief: HOST platform display implementation (file-based output)           */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "device/display/display.h"
#include "core/error.h"
#include "core/image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TARGET_BOARD_HOST

/**
 * @brief HOST display state
 */
static struct
{
    bool initialized;
    char output_path[256];
    uint32_t frame_count;
} host_display = {.initialized = false, .output_path = {0}, .frame_count = 0};

/**
 * @brief Initialize HOST display (set output file path)
 * @return 0 on success, -1 otherwise
 */
int DISPLAY_Init(void)
{
    // Get output path from environment or use default
    const char *output_file = getenv("EMBEDDIP_DISPLAY_OUTPUT");
    if (output_file) {
        strncpy(host_display.output_path, output_file, sizeof(host_display.output_path) - 1);
        host_display.output_path[sizeof(host_display.output_path) - 1] = '\0';
    } else {
        // Default output file
        strncpy(host_display.output_path, "display_output.raw", sizeof(host_display.output_path) - 1);
    }

    host_display.frame_count = 0;
    host_display.initialized = true;

    printf("[HOST Display] Initialized: output=%s\n", host_display.output_path);

    return 0;
}

/**
 * @brief Write image to output file
 * @param img Input image
 * @return 0 on success, -1 otherwise
 */
int DISPLAY_ShowImage(const Image *img)
{
    if (!host_display.initialized) {
        fprintf(stderr, "[HOST Display] Error: Not initialized\n");
        return -1;
    }

    if (!img || !img->pixels) {
        return -1;
    }

    // Generate output filename with frame number
    char filename[300];
    if (host_display.frame_count > 0) {
        snprintf(filename,
                 sizeof(filename),
                 "%s.%04u",
                 host_display.output_path,
                 host_display.frame_count);
    } else {
        strncpy(filename, host_display.output_path, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }

    // Open output file
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "[HOST Display] Error: Cannot open output file '%s'\n", filename);
        return -1;
    }

    // Calculate size based on format
    size_t bytes_per_pixel = 0;
    switch (img->format) {
    case IMAGE_FORMAT_GRAYSCALE:
        bytes_per_pixel = 1;
        break;
    case IMAGE_FORMAT_RGB565:
    case IMAGE_FORMAT_YUV:
        bytes_per_pixel = 2;
        break;
    case IMAGE_FORMAT_RGB888:
    case IMAGE_FORMAT_HSI:
        bytes_per_pixel = 3;
        break;
    default:
        fclose(fp);
        return -1;
    }

    size_t data_size = img->width * img->height * bytes_per_pixel;

    // Write raw image data
    size_t bytes_written = fwrite(img->pixels, 1, data_size, fp);
    fclose(fp);

    if (bytes_written != data_size) {
        fprintf(stderr,
                "[HOST Display] Error: Wrote %zu bytes, expected %zu\n",
                bytes_written,
                data_size);
        return -1;
    }

    printf("[HOST Display] Wrote %dx%d image (%zu bytes) to %s\n",
           img->width,
           img->height,
           bytes_written,
           filename);

    host_display.frame_count++;

    return 0;
}

/**
 * @brief Set display output file path
 * @param path Path to output file
 * @return 0 on success, -1 otherwise
 */
int DISPLAY_SetOutputFile(const char *path)
{
    if (!path) {
        return -1;
    }

    strncpy(host_display.output_path, path, sizeof(host_display.output_path) - 1);
    host_display.output_path[sizeof(host_display.output_path) - 1] = '\0';

    printf("[HOST Display] Output file set to: %s\n", host_display.output_path);
    return 0;
}

/**
 * @brief Clear framebuffer (no-op for HOST)
 * @param color Background color (ignored)
 * @return 0
 */
int DISPLAY_Clear(uint32_t color)
{
    (void)color;  // Unused
    return 0;
}

/**
 * @brief De-initialize HOST display
 * @return 0 on success
 */
int DISPLAY_DeInit(void)
{
    host_display.initialized = false;
    host_display.frame_count = 0;
    memset(host_display.output_path, 0, sizeof(host_display.output_path));
    printf("[HOST Display] De-initialized (%u frames written)\n", host_display.frame_count);
    return 0;
}

#endif /* TARGET_BOARD_HOST */
