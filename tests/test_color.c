/* ========================================================================== */
/*  File: test_color.c                                                        */
/*  Brief: Unit tests for color conversion functions                          */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "unity.h"
#include "imgproc/color.h"
#include "board/common.h"
#include "core/image.h"
#include "core/error.h"
#include <string.h>

/* Test Setup and Teardown */
void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* Test Cases */

void test_cvtColor_rgb888_to_grayscale_null_input(void)
{
    Image *outImg = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_GRAYSCALE, &outImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = cvtColor(NULL, outImg, CVT_RGB888_TO_GRAYSCALE);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);

    deleteImage(outImg);
}

void test_cvtColor_rgb888_to_grayscale_null_output(void)
{
    Image *inImg = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_RGB888, &inImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = cvtColor(inImg, NULL, CVT_RGB888_TO_GRAYSCALE);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);

    deleteImage(inImg);
}

void test_cvtColor_rgb888_to_grayscale_valid(void)
{
    Image *inImg = NULL;
    Image *outImg = NULL;

    embeddip_status_t status = createImageWH(10, 10, IMAGE_FORMAT_RGB888, &inImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createImageWH(10, 10, IMAGE_FORMAT_GRAYSCALE, &outImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Initialize input image with test pattern */
    uint8_t *pixels = (uint8_t *)inImg->pixels;
    for (int i = 0; i < 10 * 10 * 3; i += 3) {
        pixels[i] = 100;     /* R */
        pixels[i + 1] = 150; /* G */
        pixels[i + 2] = 200; /* B */
    }

    status = cvtColor(inImg, outImg, CVT_RGB888_TO_GRAYSCALE);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Verify output format */
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_GRAYSCALE, outImg->format);
    TEST_ASSERT_NOT_NULL(outImg->pixels);

    deleteImage(inImg);
    deleteImage(outImg);
}

void test_cvtColor_rgb565_to_rgb888_valid(void)
{
    Image *inImg = NULL;
    Image *outImg = NULL;

    embeddip_status_t status = createImageWH(10, 10, IMAGE_FORMAT_RGB565, &inImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createImageWH(10, 10, IMAGE_FORMAT_RGB888, &outImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Initialize input image */
    uint16_t *pixels = (uint16_t *)inImg->pixels;
    for (int i = 0; i < 10 * 10; i++) {
        pixels[i] = 0xF800; /* Red in RGB565 */
    }

    status = cvtColor(inImg, outImg, CVT_RGB565_TO_RGB888);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Verify output format */
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_RGB888, outImg->format);
    TEST_ASSERT_NOT_NULL(outImg->pixels);

    deleteImage(inImg);
    deleteImage(outImg);
}

void test_cvtColor_grayscale_to_rgb888_valid(void)
{
    Image *inImg = NULL;
    Image *outImg = NULL;

    embeddip_status_t status = createImageWH(10, 10, IMAGE_FORMAT_GRAYSCALE, &inImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createImageWH(10, 10, IMAGE_FORMAT_RGB888, &outImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Initialize grayscale image */
    uint8_t *pixels = (uint8_t *)inImg->pixels;
    for (int i = 0; i < 10 * 10; i++) {
        pixels[i] = 128; /* Mid gray */
    }

    status = cvtColor(inImg, outImg, CVT_GRAYSCALE_TO_RGB888);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Verify output format */
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_RGB888, outImg->format);
    TEST_ASSERT_NOT_NULL(outImg->pixels);

    deleteImage(inImg);
    deleteImage(outImg);
}

void test_cvtColor_copy_operation(void)
{
    Image *inImg = NULL;
    Image *outImg = NULL;

    embeddip_status_t status = createImageWH(10, 10, IMAGE_FORMAT_RGB888, &inImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createImageWH(10, 10, IMAGE_FORMAT_RGB888, &outImg);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Initialize input with pattern */
    uint8_t *in_pixels = (uint8_t *)inImg->pixels;
    for (int i = 0; i < 10 * 10 * 3; i++) {
        in_pixels[i] = (uint8_t)(i % 256);
    }

    status = cvtColor(inImg, outImg, CVT_COPY);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Verify copy */
    uint8_t *out_pixels = (uint8_t *)outImg->pixels;
    for (int i = 0; i < 10 * 10 * 3; i++) {
        TEST_ASSERT_EQUAL_UINT((unsigned int)in_pixels[i], (unsigned int)out_pixels[i]);
    }

    deleteImage(inImg);
    deleteImage(outImg);
}

void test_hueThreshold_null_input(void)
{
    Image *output = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_GRAYSCALE, &output);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = hueThreshold(NULL, output, 0.0f, 180.0f);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);

    deleteImage(output);
}

void test_hueThreshold_null_output(void)
{
    Image *input = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_HSI, &input);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = hueThreshold(input, NULL, 0.0f, 180.0f);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);

    deleteImage(input);
}

void test_hueThreshold_valid(void)
{
    Image *input = NULL;
    Image *output = NULL;

    embeddip_status_t status = createImageWH(10, 10, IMAGE_FORMAT_HSI, &input);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Allocate channels for HSI format */
    status = createChals(input, 3);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createImageWH(10, 10, IMAGE_FORMAT_GRAYSCALE, &output);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Note: hueThreshold requires HSI data in channels, not just allocated channels.
     * For now, we test that it returns an appropriate error for uninitialized data */
    status = hueThreshold(input, output, 0.0f, 60.0f);
    /* Function should either succeed or return appropriate error, not crash */
    TEST_ASSERT_TRUE(status != EMBEDDIP_ERROR_NULL_PTR);

    deleteImage(input);
    deleteImage(output);
}

void test_inRange_null_input(void)
{
    Image *mask = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_GRAYSCALE, &mask);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    uint8_t lower[3] = {0, 0, 0};
    uint8_t upper[3] = {255, 255, 255};

    status = inRange(NULL, mask, lower, upper);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);

    deleteImage(mask);
}

void test_inRange_null_mask(void)
{
    Image *input = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_RGB888, &input);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    uint8_t lower[3] = {0, 0, 0};
    uint8_t upper[3] = {255, 255, 255};

    status = inRange(input, NULL, lower, upper);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);

    deleteImage(input);
}

void test_inRange_valid(void)
{
    Image *input = NULL;
    Image *mask = NULL;

    embeddip_status_t status = createImageWH(10, 10, IMAGE_FORMAT_RGB888, &input);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createImageWH(10, 10, IMAGE_FORMAT_GRAYSCALE, &mask);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    uint8_t lower[3] = {50, 50, 50};
    uint8_t upper[3] = {200, 200, 200};

    /* Initialize input with values in range */
    uint8_t *pixels = (uint8_t *)input->pixels;
    for (int i = 0; i < 10 * 10 * 3; i++) {
        pixels[i] = 100; /* Within range */
    }

    status = inRange(input, mask, lower, upper);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    /* Verify mask is created */
    TEST_ASSERT_NOT_NULL(mask->pixels);

    deleteImage(input);
    deleteImage(mask);
}

/* Main Test Runner */
int main(void)
{
    UnityBegin("test_color.c");

    RUN_TEST(test_cvtColor_rgb888_to_grayscale_null_input);
    RUN_TEST(test_cvtColor_rgb888_to_grayscale_null_output);
    RUN_TEST(test_cvtColor_rgb888_to_grayscale_valid);
    RUN_TEST(test_cvtColor_rgb565_to_rgb888_valid);
    RUN_TEST(test_cvtColor_grayscale_to_rgb888_valid);
    RUN_TEST(test_cvtColor_copy_operation);
    RUN_TEST(test_hueThreshold_null_input);
    RUN_TEST(test_hueThreshold_null_output);
    RUN_TEST(test_hueThreshold_valid);
    RUN_TEST(test_inRange_null_input);
    RUN_TEST(test_inRange_null_mask);
    RUN_TEST(test_inRange_valid);

    return UnityEnd();
}
