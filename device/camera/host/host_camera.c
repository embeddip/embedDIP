/* ========================================================================== */
/*  File: host_camera.c                                                       */
/*  Brief: HOST platform camera implementation (file-based input)             */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "device/camera/camera.h"
#include "core/error.h"
#include "core/image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TARGET_BOARD_HOST

/**
 * @brief HOST camera state
 */
static struct
{
    bool initialized;
    char input_path[256];
    ImageFormat format;
    ImageResolution resolution;
} host_camera = {
    .initialized = false, .input_path = {0}, .format = IMAGE_FORMAT_RGB565, .resolution = IMAGE_RES_VGA};

/**
 * @brief Initialize HOST camera (set input file path)
 * @param Resolution Camera resolution (ImageResolution enum)
 * @param Format Image format (ImageFormat enum)
 * @return 0 on success, -1 otherwise
 */
uint8_t CAMERA_Init(uint32_t Resolution, uint8_t Format)
{
    // Get input path from environment or use default
    const char *input_file = getenv("EMBEDDIP_CAMERA_INPUT");
    if (input_file) {
        strncpy(host_camera.input_path, input_file, sizeof(host_camera.input_path) - 1);
        host_camera.input_path[sizeof(host_camera.input_path) - 1] = '\0';
    } else {
        // Default input file
        strncpy(host_camera.input_path, "camera_input.raw", sizeof(host_camera.input_path) - 1);
    }

    host_camera.format = (ImageFormat)Format;
    host_camera.resolution = (ImageResolution)Resolution;
    host_camera.initialized = true;

    printf("[HOST Camera] Initialized: input=%s, format=%d, resolution=%d\n",
           host_camera.input_path,
           Format,
           Resolution);

    return 0;  // CAMERA_OK
}

/**
 * @brief Capture image from file
 * @param mode Capture mode (ignored for HOST)
 * @param img Output image (must be pre-allocated)
 * @return 0 on success, -1 otherwise
 */
int camera_capture(captureMode mode, Image *img)
{
    (void)mode;  // Unused for HOST

    if (!host_camera.initialized) {
        fprintf(stderr, "[HOST Camera] Error: Not initialized\n");
        return -1;
    }

    if (!img || !img->pixels) {
        return -1;
    }

    // Open input file
    FILE *fp = fopen(host_camera.input_path, "rb");
    if (!fp) {
        fprintf(stderr,
                "[HOST Camera] Error: Cannot open input file '%s'\n",
                host_camera.input_path);
        fprintf(stderr,
                "[HOST Camera] Hint: Set EMBEDDIP_CAMERA_INPUT environment variable\n");
        return -1;
    }

    // Calculate expected size based on format
    size_t bytes_per_pixel = 0;
    switch (host_camera.format) {
    case IMAGE_FORMAT_GRAYSCALE:
        bytes_per_pixel = 1;
        break;
    case IMAGE_FORMAT_RGB565:
    case IMAGE_FORMAT_YUV:
        bytes_per_pixel = 2;
        break;
    case IMAGE_FORMAT_RGB888:
        bytes_per_pixel = 3;
        break;
    case IMAGE_FORMAT_HSI:
        bytes_per_pixel = 3;  // Assuming 3 channels
        break;
    default:
        fclose(fp);
        return -1;
    }

    size_t expected_size = img->width * img->height * bytes_per_pixel;

    // Read raw image data
    size_t bytes_read = fread(img->pixels, 1, expected_size, fp);
    fclose(fp);

    if (bytes_read != expected_size) {
        fprintf(stderr,
                "[HOST Camera] Error: Read %zu bytes, expected %zu\n",
                bytes_read,
                expected_size);
        return -1;
    }

    printf("[HOST Camera] Captured %dx%d image (%zu bytes) from %s\n",
           img->width,
           img->height,
           bytes_read,
           host_camera.input_path);

    return 0;
}

/**
 * @brief Set camera input file path
 * @param path Path to raw image file
 * @return 0 on success, -1 otherwise
 */
int CAMERA_SetInputFile(const char *path)
{
    if (!path) {
        return -1;
    }

    strncpy(host_camera.input_path, path, sizeof(host_camera.input_path) - 1);
    host_camera.input_path[sizeof(host_camera.input_path) - 1] = '\0';

    printf("[HOST Camera] Input file set to: %s\n", host_camera.input_path);
    return 0;
}

#endif /* TARGET_BOARD_HOST */
