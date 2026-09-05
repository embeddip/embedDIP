#include <assert.h>
#include <stdint.h>

#include <cv/integral.h>

int main(void)
{
    uint8_t pixels[10] = {1u, 2u, 3u, 99u, 99u, 4u, 5u, 6u, 99u, 99u};
    uint32_t values[8] = {0u, 0u, 0u, 0xdeadbeefu, 0u, 0u, 0u, 0xdeadbeefu};
    uint8_t dummy_pixel = 0u;
    uint32_t dummy_value = 0u;
    uint64_t sum = 0u;
    ImageView src = {
        .pixels = pixels,
        .width = 3u,
        .height = 2u,
        .row_stride_bytes = 5u,
        .format = IMAGE_FORMAT_GRAYSCALE,
        .depth = IMAGE_DEPTH_U8,
        .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
        .flags = 0u,
    };
    CvIntegralU32 table = {
        .values = values,
        .width = 3u,
        .height = 2u,
        .row_stride_values = 4u,
    };
    Rectangle full = {.x = 0, .y = 0, .width = 3, .height = 2};
    Rectangle middle_right = {.x = 1, .y = 0, .width = 2, .height = 2};
    Rectangle negative = {.x = -1, .y = 0, .width = 1, .height = 1};
    Rectangle out_of_bounds = {.x = 2, .y = 0, .width = 2, .height = 1};
    ImageView oversized_src = {
        .pixels = &dummy_pixel,
        .width = 65536u,
        .height = 65536u,
        .row_stride_bytes = 65536u,
        .format = IMAGE_FORMAT_GRAYSCALE,
        .depth = IMAGE_DEPTH_U8,
        .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
        .flags = 0u,
    };
    CvIntegralU32 oversized_table = {
        .values = &dummy_value,
        .width = 65536u,
        .height = 65536u,
        .row_stride_values = 65536u,
    };
    CvIntegralU32 unaddressable_table = {
        .values = &dummy_value,
        .width = UINT32_MAX,
        .height = UINT32_MAX,
        .row_stride_values = UINT32_MAX,
    };
    Rectangle origin_pixel = {.x = 0, .y = 0, .width = 1, .height = 1};

    assert(cv_integral_u8_u32(&src, &table) == EMBEDDIP_OK);
    assert(values[0] == 1u);
    assert(values[1] == 3u);
    assert(values[2] == 6u);
    assert(values[3] == 0xdeadbeefu);
    assert(values[4] == 5u);
    assert(values[5] == 12u);
    assert(values[6] == 21u);
    assert(values[7] == 0xdeadbeefu);

    assert(cv_integral_sum_u32(&table, full, &sum) == EMBEDDIP_OK);
    assert(sum == 21u);
    assert(cv_integral_sum_u32(&table, middle_right, &sum) == EMBEDDIP_OK);
    assert(sum == 16u);
    assert(cv_integral_sum_u32(&table, negative, &sum) == EMBEDDIP_ERROR_OUT_OF_RANGE);
    assert(cv_integral_sum_u32(&table, out_of_bounds, &sum) ==
           EMBEDDIP_ERROR_OUT_OF_RANGE);

    assert(cv_integral_u8_u32(&oversized_src, &oversized_table) ==
           EMBEDDIP_ERROR_OVERFLOW);
    assert(cv_integral_sum_u32(&unaddressable_table, origin_pixel, &sum) ==
           EMBEDDIP_ERROR_OVERFLOW);

    assert(cv_integral_u8_u32(NULL, &table) == EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_integral_u8_u32(&src, NULL) == EMBEDDIP_ERROR_NULL_PTR);
    table.values = NULL;
    assert(cv_integral_u8_u32(&src, &table) == EMBEDDIP_ERROR_NULL_PTR);
    table.values = values;
    table.width = 0u;
    assert(cv_integral_u8_u32(&src, &table) == EMBEDDIP_ERROR_INVALID_SIZE);
    table.width = 3u;
    table.row_stride_values = 2u;
    assert(cv_integral_u8_u32(&src, &table) == EMBEDDIP_ERROR_INVALID_SIZE);
    table.row_stride_values = 4u;
    table.height = 1u;
    assert(cv_integral_u8_u32(&src, &table) == EMBEDDIP_ERROR_INVALID_SIZE);
    table.height = 2u;

    src.format = IMAGE_FORMAT_RGB888;
    assert(cv_integral_u8_u32(&src, &table) == EMBEDDIP_ERROR_INVALID_FORMAT);
    src.format = IMAGE_FORMAT_GRAYSCALE;
    src.depth = IMAGE_DEPTH_F32;
    assert(cv_integral_u8_u32(&src, &table) == EMBEDDIP_ERROR_INVALID_DEPTH);
    src.depth = IMAGE_DEPTH_U8;

    assert(cv_integral_sum_u32(NULL, full, &sum) == EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_integral_sum_u32(&table, full, NULL) == EMBEDDIP_ERROR_NULL_PTR);
    full.width = 0;
    assert(cv_integral_sum_u32(&table, full, &sum) == EMBEDDIP_ERROR_OUT_OF_RANGE);

    return 0;
}
