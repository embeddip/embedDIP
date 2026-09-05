#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <cv/detect.h>

/* 8x8 grid, 3 classes (0=background). Two blobs:
 *   class 1 at cells (1,1),(2,1),(1,2),(2,2)  score 0.9
 *   class 2 at cells (5,5),(6,5)              score 0.7
 * Input image 96x96 -> cell = 12 px. */
#define GW 8
#define GH 8
#define NC 3

static float grid[GW * GH * NC];

static void set_cell(int x, int y, int cls, float score)
{
    float *p = &grid[(y * GW + x) * NC];
    p[cls] = score;
}

int main(void)
{
    /* background everywhere: class 0 wins weakly */
    for (int i = 0; i < GW * GH; ++i) {
        grid[i * NC + 0] = 0.6f;
        grid[i * NC + 1] = 0.0f;
        grid[i * NC + 2] = 0.0f;
    }
    int a[4][2] = {{1, 1}, {2, 1}, {1, 2}, {2, 2}};
    for (int i = 0; i < 4; ++i) {
        set_cell(a[i][0], a[i][1], 1, 0.9f);
        grid[(a[i][1] * GW + a[i][0]) * NC + 0] = 0.0f; /* drop bg so class1 wins */
    }
    set_cell(5, 5, 2, 0.7f);
    grid[(5 * GW + 5) * NC + 0] = 0.0f;
    set_cell(6, 5, 2, 0.7f);
    grid[(5 * GW + 6) * NC + 0] = 0.0f;

    CvNnDetection out[16];
    size_t n = 0;
    assert(cv_detect_fomo_decode(grid, GW, GH, NC, 0, 0.5f, 96, 96, out, 16, &n) ==
           EMBEDDIP_OK);
    assert(n == 2);

    /* Highest score first: class-1 blob. */
    assert(out[0].cls == 1);
    assert(out[0].score == 0.9f);
    assert(out[0].box.x == 12 && out[0].box.y == 12);
    assert(out[0].box.width == 24 && out[0].box.height == 24);

    assert(out[1].cls == 2);
    assert(out[1].score == 0.7f);
    assert(out[1].box.x == 60 && out[1].box.y == 60);
    assert(out[1].box.width == 24 && out[1].box.height == 12);

    /* Threshold above all activations -> no detections. */
    assert(cv_detect_fomo_decode(grid, GW, GH, NC, 0, 0.95f, 96, 96, out, 16, &n) ==
           EMBEDDIP_OK);
    assert(n == 0);

    /* Argument validation. */
    assert(cv_detect_fomo_decode(NULL, GW, GH, NC, 0, 0.5f, 96, 96, out, 16, &n) ==
           EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_detect_fomo_decode(grid, 0, GH, NC, 0, 0.5f, 96, 96, out, 16, &n) ==
           EMBEDDIP_ERROR_INVALID_SIZE);
    assert(cv_detect_fomo_decode(grid, GW, GH, 1, 0, 0.5f, 96, 96, out, 16, &n) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_detect_fomo_decode(grid, GW, GH, NC, 0, 0.5f, 96, 96, out, 0, &n) ==
           EMBEDDIP_ERROR_INVALID_ARG);

    return 0;
}
