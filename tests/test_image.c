/* ========================================================================== */
/*  File: test_image.c                                                        */
/*  Brief: Unit tests for image creation and management                       */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "unity.h"
#include "board/common.h"
#include "core/image.h"
#include "core/error.h"

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

void test_createImage_valid_grayscale(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &img);

    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_UINT(320, img->width);
    TEST_ASSERT_EQUAL_UINT(240, img->height);
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_GRAYSCALE, img->format);
    TEST_ASSERT_NOT_NULL(img->pixels);

    deleteImage(img);
}

void test_createImage_valid_rgb565(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImage(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565, &img);

    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_UINT(160, img->width);
    TEST_ASSERT_EQUAL_UINT(120, img->height);
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_RGB565, img->format);
    TEST_ASSERT_EQUAL_INT(IMAGE_DEPTH_U16, img->depth);
    TEST_ASSERT_NOT_NULL(img->pixels);

    deleteImage(img);
}

void test_createImage_valid_rgb888(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImage(IMAGE_RES_96X96, IMAGE_FORMAT_RGB888, &img);

    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_UINT(96, img->width);
    TEST_ASSERT_EQUAL_UINT(96, img->height);
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_RGB888, img->format);
    TEST_ASSERT_EQUAL_INT(IMAGE_DEPTH_U24, img->depth);
    TEST_ASSERT_NOT_NULL(img->pixels);

    deleteImage(img);
}

void test_createImage_null_output_pointer(void)
{
    embeddip_status_t status = createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, NULL);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);
}

void test_createImageWH_valid_custom_size(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, &img);

    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_UINT(640, img->width);
    TEST_ASSERT_EQUAL_UINT(480, img->height);
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_GRAYSCALE, img->format);
    TEST_ASSERT_NOT_NULL(img->pixels);

    deleteImage(img);
}

void test_createImageWH_invalid_width(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(0, 480, IMAGE_FORMAT_GRAYSCALE, &img);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_INVALID_SIZE, status);
    TEST_ASSERT_NULL(img);
}

void test_createImageWH_invalid_height(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(640, 0, IMAGE_FORMAT_GRAYSCALE, &img);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_INVALID_SIZE, status);
    TEST_ASSERT_NULL(img);
}

void test_createImageWH_negative_width(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(-100, 480, IMAGE_FORMAT_GRAYSCALE, &img);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_INVALID_SIZE, status);
    TEST_ASSERT_NULL(img);
}

void test_createImageWH_null_output_pointer(void)
{
    embeddip_status_t status = createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, NULL);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);
}

void test_createImageWH_large_size(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(1920, 1080, IMAGE_FORMAT_RGB888, &img);

    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_UINT(1920, img->width);
    TEST_ASSERT_EQUAL_UINT(1080, img->height);
    TEST_ASSERT_EQUAL_INT(IMAGE_FORMAT_RGB888, img->format);

    deleteImage(img);
}

void test_deleteImage_null_pointer(void)
{
    /* Should not crash */
    deleteImage(NULL);
    TEST_ASSERT_TRUE(1); /* If we get here, test passes */
}

void test_createChals_valid(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_RGB888, &img);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createChals(img, 3);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);
    TEST_ASSERT_TRUE(img->is_chals);
    TEST_ASSERT_NOT_NULL(img->chals);

    deleteImage(img);
}

void test_createChals_null_image(void)
{
    embeddip_status_t status = createChals(NULL, 3);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_ERROR_NULL_PTR, status);
}

void test_isChalsEmpty_no_channels(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_GRAYSCALE, &img);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    TEST_ASSERT_TRUE(isChalsEmpty(img));

    deleteImage(img);
}

void test_isChalsEmpty_with_channels(void)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(100, 100, IMAGE_FORMAT_GRAYSCALE, &img);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    status = createChals(img, 1);
    TEST_ASSERT_EQUAL_INT(EMBEDDIP_OK, status);

    TEST_ASSERT_FALSE(isChalsEmpty(img));

    deleteImage(img);
}

void test_legacy_createImage_valid(void)
{
    Image *img = createImage_legacy(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_UINT(320, img->width);
    TEST_ASSERT_EQUAL_UINT(240, img->height);

    deleteImage(img);
}

void test_legacy_createImageWH_valid(void)
{
    Image *img = createImageWH_legacy(200, 150, IMAGE_FORMAT_RGB565);

    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_UINT(200, img->width);
    TEST_ASSERT_EQUAL_UINT(150, img->height);

    deleteImage(img);
}

void test_image_num_channels_helper(void)
{
    TEST_ASSERT_EQUAL_UINT(1, image_num_channels(IMAGE_FORMAT_GRAYSCALE));
    TEST_ASSERT_EQUAL_UINT(3, image_num_channels(IMAGE_FORMAT_RGB888));
    TEST_ASSERT_EQUAL_UINT(3, image_num_channels(IMAGE_FORMAT_RGB565));
    TEST_ASSERT_EQUAL_UINT(3, image_num_channels(IMAGE_FORMAT_YUV));
    TEST_ASSERT_EQUAL_UINT(3, image_num_channels(IMAGE_FORMAT_HSI));
}

void test_image_pixel_size_bytes_helper(void)
{
    TEST_ASSERT_EQUAL_UINT(1, image_pixel_size_bytes(IMAGE_FORMAT_GRAYSCALE, IMAGE_DEPTH_U8));
    TEST_ASSERT_EQUAL_UINT(2, image_pixel_size_bytes(IMAGE_FORMAT_RGB565, IMAGE_DEPTH_U16));
    TEST_ASSERT_EQUAL_UINT(3, image_pixel_size_bytes(IMAGE_FORMAT_RGB888, IMAGE_DEPTH_U24));
    TEST_ASSERT_EQUAL_UINT(4, image_pixel_size_bytes(IMAGE_FORMAT_HSI, IMAGE_DEPTH_F32));
}

void test_image_res_width_helper(void)
{
    TEST_ASSERT_EQUAL_UINT(96, image_res_width(IMAGE_RES_96X96));
    TEST_ASSERT_EQUAL_UINT(160, image_res_width(IMAGE_RES_QQVGA));
    TEST_ASSERT_EQUAL_UINT(320, image_res_width(IMAGE_RES_QVGA));
    TEST_ASSERT_EQUAL_UINT(640, image_res_width(IMAGE_RES_VGA));
    TEST_ASSERT_EQUAL_UINT(1920, image_res_width(IMAGE_RES_FHD));
}

void test_image_res_height_helper(void)
{
    TEST_ASSERT_EQUAL_UINT(96, image_res_height(IMAGE_RES_96X96));
    TEST_ASSERT_EQUAL_UINT(120, image_res_height(IMAGE_RES_QQVGA));
    TEST_ASSERT_EQUAL_UINT(240, image_res_height(IMAGE_RES_QVGA));
    TEST_ASSERT_EQUAL_UINT(480, image_res_height(IMAGE_RES_VGA));
    TEST_ASSERT_EQUAL_UINT(1080, image_res_height(IMAGE_RES_FHD));
}

/* Main Test Runner */
int main(void)
{
    UnityBegin("test_image.c");

    RUN_TEST(test_createImage_valid_grayscale);
    RUN_TEST(test_createImage_valid_rgb565);
    RUN_TEST(test_createImage_valid_rgb888);
    RUN_TEST(test_createImage_null_output_pointer);
    RUN_TEST(test_createImageWH_valid_custom_size);
    RUN_TEST(test_createImageWH_invalid_width);
    RUN_TEST(test_createImageWH_invalid_height);
    RUN_TEST(test_createImageWH_negative_width);
    RUN_TEST(test_createImageWH_null_output_pointer);
    RUN_TEST(test_createImageWH_large_size);
    RUN_TEST(test_deleteImage_null_pointer);
    RUN_TEST(test_createChals_valid);
    RUN_TEST(test_createChals_null_image);
    RUN_TEST(test_isChalsEmpty_no_channels);
    RUN_TEST(test_isChalsEmpty_with_channels);
    RUN_TEST(test_legacy_createImage_valid);
    RUN_TEST(test_legacy_createImageWH_valid);
    RUN_TEST(test_image_num_channels_helper);
    RUN_TEST(test_image_pixel_size_bytes_helper);
    RUN_TEST(test_image_res_width_helper);
    RUN_TEST(test_image_res_height_helper);

    return UnityEnd();
}
