#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <core/image.h>
#include <cv/tracker_kcf.h>

#define FRAME_W 128u
#define FRAME_H 128u

static void fill_frame(uint8_t *pixels, int32_t block_x, int32_t block_y)
{
    memset(pixels, 0u, FRAME_W * FRAME_H);
    for (int32_t y = block_y; y < block_y + 20; ++y) {
        for (int32_t x = block_x; x < block_x + 20; ++x) {
            pixels[y * (int32_t)FRAME_W + x] = 255u;
        }
    }
}

static void test_init_null(void)
{
    Rectangle roi = {40, 40, 20, 20};
    assert(cv_kcf_init(NULL, NULL, roi) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_update_without_init(void)
{
    CvKcfState state;
    memset(&state, 0, sizeof(state));
    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 40, 40);
    ImageView frame = {.pixels = pixels,
                       .width = FRAME_W,
                       .height = FRAME_H,
                       .row_stride_bytes = FRAME_W,
                       .format = IMAGE_FORMAT_GRAYSCALE,
                       .depth = IMAGE_DEPTH_U8};
    Rectangle out;
    assert(cv_kcf_update(&state, &frame, &out) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_init_and_update_stationary(void)
{
    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 40, 40);
    ImageView view = {.pixels = pixels,
                      .width = FRAME_W,
                      .height = FRAME_H,
                      .row_stride_bytes = FRAME_W,
                      .format = IMAGE_FORMAT_GRAYSCALE,
                      .depth = IMAGE_DEPTH_U8};
    Rectangle roi = {40, 40, 20, 20};

    CvKcfState state;
    assert(cv_kcf_init(&state, &view, roi) == EMBEDDIP_OK);

    Rectangle out;
    assert(cv_kcf_update(&state, &view, &out) == EMBEDDIP_OK);
    /* Stationary target: the recovered box should stay near the original. */
    assert(out.width == 20 && out.height == 20);
    assert(out.x > 20 && out.x < 60);
    assert(out.y > 20 && out.y < 60);

    assert(cv_kcf_free(&state) == EMBEDDIP_OK);
}

/* Target moves between init and update: verify the tracker actually follows
 * the motion, not just reports a stationary/no-op box. */
static void test_init_and_update_moving(void)
{
    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 40, 40);
    ImageView view = {.pixels = pixels,
                      .width = FRAME_W,
                      .height = FRAME_H,
                      .row_stride_bytes = FRAME_W,
                      .format = IMAGE_FORMAT_GRAYSCALE,
                      .depth = IMAGE_DEPTH_U8};
    /* ROI is larger than the 20x20 block and includes surrounding
     * background, so the template patch has real edge structure to
     * correlate on (a ROI exactly matching the block would sample a
     * uniform white square with nothing to distinguish position). */
    Rectangle roi = {20, 20, 60, 60};

    CvKcfState state;
    assert(cv_kcf_init(&state, &view, roi) == EMBEDDIP_OK);

    /* Move the block 10px right and down. */
    fill_frame(pixels, 50, 50);

    Rectangle out;
    assert(cv_kcf_update(&state, &view, &out) == EMBEDDIP_OK);
    assert(out.width == 60 && out.height == 60);
    /* Tracker must move toward the new block location, not stay at the old
     * box (x=20..80,y=20..80) or report a no-op. */
    assert(out.x > 24 && out.x < 36);
    assert(out.y > 24 && out.y < 36);

    assert(cv_kcf_free(&state) == EMBEDDIP_OK);
}

static void test_online_update_survives_appearance_drift(void)
{
    /* target block whose intensity ramps down over frames while translating;
     * with adaptation the peak stays locked, tracked center keeps up. */
    static uint8_t px[FRAME_W * FRAME_H];
    CvKcfState st;
    memset(&st, 0, sizeof st);
    Rectangle roi = {40, 40, 20, 20};
    /* init frame: bright block at 40,40 */
    fill_frame(px, 40, 40);
    ImageView f = {px, FRAME_W, FRAME_H, FRAME_W, IMAGE_FORMAT_GRAYSCALE, IMAGE_DEPTH_U8, 0, 0};
    assert(cv_kcf_init(&st, &f, roi) == EMBEDDIP_OK);
    assert(st.learn_rate > 0.0f); /* adaptation on by default */
    Rectangle out;
    int bx = 40;
    for (int k = 0; k < 5; k++) {
        bx += 6;
        memset(px, 0, sizeof px);
        int val = 255 - k * 30; /* appearance drifts */
        for (int y = 40; y < 60; y++)
            for (int x = bx; x < bx + 20; x++) px[y * FRAME_W + x] = (uint8_t)val;
        assert(cv_kcf_update(&st, &f, &out) == EMBEDDIP_OK);
    }
    int cx = out.x + out.width / 2;
    assert(cx > 55); /* followed the moving block */
    cv_kcf_free(&st);
}

int main(void)
{
    test_init_null();
    test_update_without_init();
    test_init_and_update_stationary();
    test_init_and_update_moving();
    test_online_update_survives_appearance_drift();
    return 0;
}
