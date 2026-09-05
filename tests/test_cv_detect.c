#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cv/detect.h>
#include <cv/haar.h>
#include <cv/integral.h>

/* A cascade with zero stages passes every window (all stages trivially pass). */
static const CvHaarCascade kAlwaysCascade = {
    .stages = NULL,
    .stage_count = 0u,
    .window = {0, 0, 0, 0},
};

static void test_scan(void)
{
    /* 4x4 integral (values irrelevant: always-pass cascade). */
    uint32_t values[16] = {0};
    CvIntegralU32 table = {
        .values = values, .width = 4u, .height = 4u, .row_stride_values = 4u};
    /* 2x2 window, step 1 -> origins x,y in {0,1,2} -> 3x3 = 9 windows. */
    CvScanConfig scan = {.window_width = 2u, .window_height = 2u, .step_x = 1u,
                         .step_y = 1u};
    CvDetection out[16];
    size_t count = 0u;

    assert(cv_detect_scan(&table, &kAlwaysCascade, &scan, out, 16u, &count) ==
           EMBEDDIP_OK);
    assert(count == 9u);
    /* First window at origin (0,0), size 2x2. */
    assert(out[0].box.x == 0 && out[0].box.y == 0);
    assert(out[0].box.width == 2 && out[0].box.height == 2);
    /* Last window at origin (2,2). */
    assert(out[8].box.x == 2 && out[8].box.y == 2);

    /* Appends: a second scan keeps prior detections. */
    assert(cv_detect_scan(&table, &kAlwaysCascade, &scan, out, 16u, &count) ==
           EMBEDDIP_OK);
    assert(count == 16u); /* 9 + 9 = 18 clipped to capacity 16 */

    /* Window larger than table -> zero new detections, no error. */
    CvScanConfig too_big = {.window_width = 5u, .window_height = 5u, .step_x = 1u,
                            .step_y = 1u};
    size_t big_count = 0u;
    assert(cv_detect_scan(&table, &kAlwaysCascade, &too_big, out, 16u, &big_count) ==
           EMBEDDIP_OK);
    assert(big_count == 0u);

    /* Step 2: origins {0,2} -> 2x2 = 4 windows. */
    CvScanConfig step2 = {.window_width = 2u, .window_height = 2u, .step_x = 2u,
                          .step_y = 2u};
    size_t s2 = 0u;
    assert(cv_detect_scan(&table, &kAlwaysCascade, &step2, out, 16u, &s2) == EMBEDDIP_OK);
    assert(s2 == 4u);

    /* Rejections. */
    size_t c = 0u;
    CvScanConfig zero_step = {.window_width = 2u, .window_height = 2u, .step_x = 0u,
                              .step_y = 1u};
    assert(cv_detect_scan(&table, &kAlwaysCascade, &zero_step, out, 16u, &c) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_detect_scan(NULL, &kAlwaysCascade, &scan, out, 16u, &c) ==
           EMBEDDIP_ERROR_NULL_PTR);
}

static void test_nms(void)
{
    /* Three boxes: A and B heavily overlap (B lower score, suppressed);
     * C is far away and kept. */
    CvDetection det[3] = {
        {.box = {0, 0, 10, 10}, .score = 50},   /* A */
        {.box = {1, 1, 10, 10}, .score = 90},   /* B, higher score, overlaps A */
        {.box = {100, 100, 10, 10}, .score = 30}, /* C, disjoint */
    };
    size_t kept = 0u;

    assert(cv_detect_nms(det, 3u, 0.3f, &kept) == EMBEDDIP_OK);
    assert(kept == 2u);
    /* Highest score first: B (90), then C (30). A suppressed by B. */
    assert(det[0].score == 90);
    assert(det[1].score == 30);

    /* High IoU threshold keeps overlapping boxes (nothing suppressed). */
    CvDetection det2[3] = {
        {.box = {0, 0, 10, 10}, .score = 50},
        {.box = {1, 1, 10, 10}, .score = 90},
        {.box = {100, 100, 10, 10}, .score = 30},
    };
    assert(cv_detect_nms(det2, 3u, 1.0f, &kept) == EMBEDDIP_OK);
    assert(kept == 3u);
    assert(det2[0].score == 90 && det2[1].score == 50 && det2[2].score == 30);

    /* Tie scores keep earlier (lower-index) box first. */
    CvDetection tie[2] = {
        {.box = {0, 0, 5, 5}, .score = 40},
        {.box = {50, 50, 5, 5}, .score = 40},
    };
    assert(cv_detect_nms(tie, 2u, 0.5f, &kept) == EMBEDDIP_OK);
    assert(kept == 2u);
    assert(tie[0].box.x == 0);  /* first-index tie winner stays first */
    assert(tie[1].box.x == 50);

    /* Empty input. */
    assert(cv_detect_nms(det, 0u, 0.5f, &kept) == EMBEDDIP_OK);
    assert(kept == 0u);

    /* Rejections. */
    assert(cv_detect_nms(NULL, 3u, 0.5f, &kept) == EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_detect_nms(det, 3u, -0.1f, &kept) == EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_detect_nms(det, 3u, 1.1f, &kept) == EMBEDDIP_ERROR_INVALID_ARG);
}

int main(void)
{
    test_scan();
    test_nms();
    return 0;
}
