#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <core/image.h>
#include <cv/track_hist.h>
#include <cv/tracker_particle.h>

#define FRAME_W 32u
#define FRAME_H 32u

static void fill_frame(uint8_t *pixels, int32_t block_x, int32_t block_y)
{
    memset(pixels, 0u, FRAME_W * FRAME_H);
    for (int32_t y = block_y; y < block_y + 6; ++y) {
        for (int32_t x = block_x; x < block_x + 6; ++x) {
            pixels[y * (int32_t)FRAME_W + x] = 255u;
        }
    }
}

static void test_init_null(void)
{
    Rectangle roi = {0, 0, 6, 6};
    assert(cv_particle_init(NULL, 10u, NULL, roi) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_init_zero_particles(void)
{
    CvParticleState state;
    float buf[2];
    Rectangle roi = {0, 0, 6, 6};
    assert(cv_particle_init(&state, 0u, buf, roi) == EMBEDDIP_ERROR_INVALID_ARG);
}

static void test_update_without_init(void)
{
    CvParticleState state;
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
    assert(cv_particle_update(&state, &frame, &out) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_tracks_toward_block(void)
{
    CvParticleState state;
    float particle_buf[100 * 2];
    /* Seed away from the block's center so a dead (image-blind) tracker,
     * which just reports the mean of the diffusing particles, stays near
     * the seed instead of moving toward the block's actual edges. */
    Rectangle roi = {6, 6, 6, 6};
    assert(cv_particle_init(&state, 100u, particle_buf, roi) == EMBEDDIP_OK);

    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 22, 22); /* block far from seed */
    ImageView frame = {.pixels = pixels,
                       .width = FRAME_W,
                       .height = FRAME_H,
                       .row_stride_bytes = FRAME_W,
                       .format = IMAGE_FORMAT_GRAYSCALE,
                       .depth = IMAGE_DEPTH_U8};

    Rectangle out;
    embeddip_status_t status = EMBEDDIP_ERROR_UNKNOWN;
    for (int frame_i = 0; frame_i < 30; ++frame_i) {
        status = cv_particle_update(&state, &frame, &out);
        assert(status == EMBEDDIP_OK);
    }
    /* The tracked box must have moved from the seed (center ~9,9) toward
     * the block (center ~25,25); a gradient-blind tracker would stay near
     * the seed's diffusion cloud instead. */
    int32_t center_x = out.x + out.width / 2;
    int32_t center_y = out.y + out.height / 2;
    assert(center_x > 15 && center_y > 15);

    cv_particle_free(&state);
    assert(!state.initialized);
}

static void test_hist_likelihood_tracks_bright_block(void)
{
    /* frame: dark bg, one 20x20 bright block; block moves; PF should follow */
    enum { W = 128, H = 128, NP = 200 };
    static uint8_t px[W * H];
    static float pbuf[NP * 2];
    CvParticleState st;
    memset(&st, 0, sizeof st);
    /* block at (40,40) */
    memset(px, 0, sizeof px);
    for (int y = 40; y < 60; y++) {
        for (int x = 40; x < 60; x++) {
            px[y * W + x] = 255;
        }
    }
    ImageView f = {px, W, H, W, IMAGE_FORMAT_GRAYSCALE, IMAGE_DEPTH_U8, 0, 0};
    Rectangle roi = {40, 40, 20, 20};
    assert(cv_particle_init_hist(&st, NP, pbuf, &f, roi) == EMBEDDIP_OK);
    assert(st.hist_nbins == CV_HIST_GRAY_BINS);
    Rectangle out;
    /* move block to (70,70) over a few frames */
    for (int step = 0; step <= 30; step += 10) {
        memset(px, 0, sizeof px);
        for (int y = 40 + step; y < 60 + step; y++) {
            for (int x = 40 + step; x < 60 + step; x++) {
                px[y * W + x] = 255;
            }
        }
        assert(cv_particle_update(&st, &f, &out) == EMBEDDIP_OK);
    }
    int cx = out.x + out.width / 2, cy = out.y + out.height / 2;
    assert(cx > 60 && cy > 60); /* estimate migrated toward (80,80) */
    cv_particle_free(&st);
}

int main(void)
{
    test_init_null();
    test_init_zero_particles();
    test_update_without_init();
    test_tracks_toward_block();
    test_hist_likelihood_tracks_bright_block();
    return 0;
}
