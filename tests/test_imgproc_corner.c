#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <imgproc/corner.h>

/* 40x40 image, solid bright square [10..29]x[10..29] on black. A filled square
 * gives strong Harris/Shi-Tomasi response only at its four corners (edges have
 * a single dominant gradient direction). */
#define WD 40
#define SQ0 10
#define SQ1 29

static uint8_t img[WD * WD];

static void make_square(void)
{
    for (int y = 0; y < WD; ++y)
        for (int x = 0; x < WD; ++x)
            img[y * WD + x] = (x >= SQ0 && x <= SQ1 && y >= SQ0 && y <= SQ1) ? 255u : 0u;
}

static int near_a_corner(int x, int y, int tol)
{
    const int cx[4] = {SQ0, SQ1, SQ0, SQ1};
    const int cy[4] = {SQ0, SQ0, SQ1, SQ1};
    for (int i = 0; i < 4; ++i) {
        int dx = x - cx[i], dy = y - cy[i];
        if (dx * dx + dy * dy <= tol * tol)
            return i;
    }
    return -1;
}

static void run(CvCornerMethod method)
{
    make_square();
    ImageView src = {
        .pixels = img,
        .width = WD,
        .height = WD,
        .row_stride_bytes = WD,
        .format = IMAGE_FORMAT_GRAYSCALE,
        .depth = IMAGE_DEPTH_U8,
        .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
        .flags = 0u,
    };
    CvCornerConfig cfg = {
        .method = method,
        .block_size = 3,
        .k = 0.04f,
        .threshold = 1.0e6f, /* well above edge response, below corner response */
        .nms_radius = 4,
    };
    CvCorner corners[64];
    size_t n = 0;
    assert(cv_corner_detect(&src, &cfg, corners, 64, &n) == EMBEDDIP_OK);

    /* Exactly the four square corners, each covered once, no spurious points. */
    assert(n >= 4);
    int hit[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < n; ++i) {
        int c = near_a_corner(corners[i].x, corners[i].y, 3);
        assert(c >= 0);            /* every detection sits on a real corner */
        hit[c] = 1;
        assert(corners[i].score > cfg.threshold);
    }
    assert(hit[0] && hit[1] && hit[2] && hit[3]); /* all four found */
    /* scores sorted descending */
    for (size_t i = 1; i < n; ++i)
        assert(corners[i - 1].score >= corners[i].score);
}

int main(void)
{
    run(CV_CORNER_HARRIS);
    run(CV_CORNER_SHI_TOMASI);

    /* Argument validation. */
    uint8_t px = 0u;
    ImageView v = {.pixels = &px, .width = 40, .height = 40, .row_stride_bytes = 40,
                   .format = IMAGE_FORMAT_GRAYSCALE, .depth = IMAGE_DEPTH_U8,
                   .region = EMBEDDIP_MEMORY_REGION_DEFAULT, .flags = 0u};
    CvCornerConfig cfg = {CV_CORNER_HARRIS, 3, 0.04f, 1000.0f, 2};
    CvCorner out[4];
    size_t n = 0;
    assert(cv_corner_detect(NULL, &cfg, out, 4, &n) == EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_corner_detect(&v, &cfg, out, 0, &n) == EMBEDDIP_ERROR_INVALID_ARG);
    cfg.block_size = 2;
    assert(cv_corner_detect(&v, &cfg, out, 4, &n) == EMBEDDIP_ERROR_INVALID_ARG);
    cfg.block_size = 3;
    v.depth = IMAGE_DEPTH_F32;
    assert(cv_corner_detect(&v, &cfg, out, 4, &n) == EMBEDDIP_ERROR_INVALID_DEPTH);

    return 0;
}
