#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include <cv/hog.h>

#define W 16u
#define H 16u
#define STRIDE 20u /* padded row stride > width */

static void fill_ramp(uint8_t *buf, uint32_t stride)
{
    for (uint32_t y = 0u; y < H; ++y) {
        for (uint32_t x = 0u; x < W; ++x) {
            /* Horizontal ramp: value grows with x, constant in y. */
            buf[y * stride + x] = (uint8_t)(x * 16u);
        }
    }
}

static void check_extract(uint32_t stride)
{
    uint8_t pixels[H * STRIDE];
    float descriptor[512];
    CvHogConfig config = {.cell_size = 4u, .l2_hys_clip = 0.2f};
    Rectangle roi = {.x = 0, .y = 0, .width = (int32_t)W, .height = (int32_t)H};
    size_t length = 0u;
    size_t computed = 0u;

    fill_ramp(pixels, stride);
    ImageView src = {
        .pixels = pixels,
        .width = W,
        .height = H,
        .row_stride_bytes = stride,
        .format = IMAGE_FORMAT_GRAYSCALE,
        .depth = IMAGE_DEPTH_U8,
        .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
        .flags = 0u,
    };

    assert(cv_hog_descriptor_size(roi, &config, &computed) == EMBEDDIP_OK);
    assert(computed == (4u - 1u) * (4u - 1u) * 36u); /* 324 */

    assert(cv_hog_extract(&src, roi, &config, descriptor, 512u, &length) == EMBEDDIP_OK);
    assert(length == computed);

    /* All finite; each 36-value block has L2 norm <= 1 + eps after clipping. */
    for (size_t b = 0u; b + CV_HOG_BLOCK_SIZE <= length; b += CV_HOG_BLOCK_SIZE) {
        double norm_sq = 0.0;
        for (size_t k = 0u; k < CV_HOG_BLOCK_SIZE; ++k) {
            float v = descriptor[b + k];
            assert(isfinite(v));
            assert(v >= 0.0f);
            norm_sq += (double)v * (double)v;
        }
        assert(norm_sq <= 1.0 + 1e-4);
    }

    /* First block, first cell: bin 0 (horizontal) should dominate its 9 bins. */
    float max_v = descriptor[0];
    size_t max_i = 0u;
    for (size_t k = 1u; k < CV_HOG_BINS; ++k) {
        if (descriptor[k] > max_v) {
            max_v = descriptor[k];
            max_i = k;
        }
    }
    assert(max_i == 0u);
    assert(max_v > 0.0f);
}

int main(void)
{
    check_extract(W);      /* tight stride */
    check_extract(STRIDE); /* padded stride */

    /* Rejection cases. */
    uint8_t pixels[H * W];
    float descriptor[512];
    CvHogConfig config = {.cell_size = 4u, .l2_hys_clip = 0.2f};
    Rectangle roi = {.x = 0, .y = 0, .width = (int32_t)W, .height = (int32_t)H};
    size_t length = 0u;
    size_t computed = 0u;

    fill_ramp(pixels, W);
    ImageView src = {
        .pixels = pixels,
        .width = W,
        .height = H,
        .row_stride_bytes = W,
        .format = IMAGE_FORMAT_GRAYSCALE,
        .depth = IMAGE_DEPTH_U8,
        .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
        .flags = 0u,
    };

    CvHogConfig zero_cell = {.cell_size = 0u, .l2_hys_clip = 0.2f};
    assert(cv_hog_descriptor_size(roi, &zero_cell, &computed) == EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_hog_extract(&src, roi, &zero_cell, descriptor, 512u, &length) ==
           EMBEDDIP_ERROR_INVALID_ARG);

    /* ROI outside the image. */
    Rectangle outside = {.x = 4, .y = 0, .width = (int32_t)W, .height = (int32_t)H};
    assert(cv_hog_extract(&src, outside, &config, descriptor, 512u, &length) ==
           EMBEDDIP_ERROR_OUT_OF_RANGE);

    /* ROI smaller than 2x2 cells (only 1 cell wide). */
    Rectangle too_small = {.x = 0, .y = 0, .width = 4, .height = 16};
    assert(cv_hog_extract(&src, too_small, &config, descriptor, 512u, &length) ==
           EMBEDDIP_ERROR_INVALID_SIZE);

    /* Null output. */
    assert(cv_hog_extract(&src, roi, &config, NULL, 512u, &length) ==
           EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_hog_descriptor_size(roi, &config, NULL) == EMBEDDIP_ERROR_NULL_PTR);

    /* Insufficient capacity: 323 < 324. */
    assert(cv_hog_extract(&src, roi, &config, descriptor, 323u, &length) ==
           EMBEDDIP_ERROR_INVALID_SIZE);

    return 0;
}
