#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <core/image.h>
#include <cv/tracker_template.h>

#define FRAME_W 20u
#define FRAME_H 20u

static void fill_frame(uint8_t *pixels, int32_t block_x, int32_t block_y)
{
    memset(pixels, 0u, FRAME_W * FRAME_H);
    for (int32_t y = block_y; y < block_y + 4; ++y) {
        for (int32_t x = block_x; x < block_x + 4; ++x) {
            pixels[y * (int32_t)FRAME_W + x] = 255u;
        }
    }
}

static void test_set_null(void)
{
    ImageView view = {0};
    Rectangle roi = {0, 0, 4, 4};
    assert(cv_template_set(NULL, &view, roi) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_match_without_set(void)
{
    CvTemplateState state;
    memset(&state, 0, sizeof(state));
    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 0, 0);
    ImageView frame = {.pixels = pixels,
                       .width = FRAME_W,
                       .height = FRAME_H,
                       .row_stride_bytes = FRAME_W,
                       .format = IMAGE_FORMAT_GRAYSCALE,
                       .depth = IMAGE_DEPTH_U8};
    Rectangle out;
    assert(cv_template_match(&state, &frame, &out) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_finds_moved_block(void)
{
    uint8_t template_pixels[FRAME_W * FRAME_H];
    fill_frame(template_pixels, 2, 2);
    ImageView template_view = {.pixels = template_pixels,
                               .width = FRAME_W,
                               .height = FRAME_H,
                               .row_stride_bytes = FRAME_W,
                               .format = IMAGE_FORMAT_GRAYSCALE,
                               .depth = IMAGE_DEPTH_U8};
    Rectangle roi = {2, 2, 4, 4};

    CvTemplateState state;
    assert(cv_template_set(&state, &template_view, roi) == EMBEDDIP_OK);

    uint8_t frame_pixels[FRAME_W * FRAME_H];
    fill_frame(frame_pixels, 10, 8); /* block moved to (10,8) */
    ImageView frame = {.pixels = frame_pixels,
                       .width = FRAME_W,
                       .height = FRAME_H,
                       .row_stride_bytes = FRAME_W,
                       .format = IMAGE_FORMAT_GRAYSCALE,
                       .depth = IMAGE_DEPTH_U8};

    Rectangle out;
    assert(cv_template_match(&state, &frame, &out) == EMBEDDIP_OK);
    assert(out.x == 10 && out.y == 8);
    assert(out.width == 4 && out.height == 4);
}

int main(void)
{
    test_set_null();
    test_match_without_set();
    test_finds_moved_block();
    return 0;
}
