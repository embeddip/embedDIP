#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <cv/detect.h>

static int near(int a, int b, int tol) { int d = a - b; return (d < 0 ? -d : d) <= tol; }

/* ---- YOLO: 2x2 grid, 1 anchor, 2 classes, 64x64 input. One strong box in
 * cell (r=0,c=1) of class 1; all other cells have near-zero objectness. ---- */
static void test_yolo(void)
{
    const int gw = 2, gh = 2, na = 1, nc = 2, stride = 5 + 2;
    float pred[2 * 2 * 1 * (5 + 2)];
    for (int i = 0; i < gw * gh * na; ++i) {
        float *b = pred + i * stride;
        b[0] = b[1] = b[2] = b[3] = 0.0f;
        b[4] = -10.0f;           /* objectness ~ 0 */
        b[5] = 0.0f; b[6] = 0.0f;
    }
    float *tgt = pred + (0 * gw + 1) * na * stride; /* cell (0,1), anchor 0 */
    tgt[0] = 0.0f; tgt[1] = 0.0f; tgt[2] = 0.0f; tgt[3] = 0.0f; /* sig=0.5, exp=1 */
    tgt[4] = 10.0f;              /* objectness ~ 1 */
    tgt[5] = 0.0f; tgt[6] = 10.0f; /* class 1 wins */

    float anchors[2] = {1.0f, 1.0f}; /* grid-cell units */
    CvYoloConfig cfg = {.grid_w = gw, .grid_h = gh, .num_anchors = na, .num_classes = nc,
                        .anchors = anchors, .in_width = 64, .in_height = 64,
                        .conf_threshold = 0.5f, .iou_threshold = 0.45f};
    CvNnDetection out[16];
    size_t n = 0;
    assert(cv_detect_yolo_decode(pred, &cfg, out, 16, &n) == EMBEDDIP_OK);
    assert(n == 1);
    assert(out[0].cls == 1);
    assert(out[0].score > 0.9f);
    /* center bx=(1+0.5)/2*64=48, by=(0+0.5)/2*64=16, bw=bh=1/2*64=32 -> (32,0,32,32) */
    assert(near(out[0].box.x, 32, 1) && near(out[0].box.y, 0, 1));
    assert(near(out[0].box.width, 32, 1) && near(out[0].box.height, 32, 1));
}

/* ---- SSD: 2 priors, 3 classes (bg,1,2), 64x64. Prior 0 (identity offsets) is
 * class 1 at high score; prior 1 is background. ---- */
static void test_ssd(void)
{
    const int np = 2, nc = 3;
    float priors[2 * 4] = {0.5f, 0.5f, 0.25f, 0.25f,   /* prior 0 */
                           0.1f, 0.1f, 0.10f, 0.10f};  /* prior 1 */
    float loc[2 * 4] = {0, 0, 0, 0,   0, 0, 0, 0};     /* no adjustment */
    float conf[2 * 3] = {0.1f, 0.9f, 0.0f,             /* prior 0 -> class 1 */
                         0.8f, 0.1f, 0.1f};            /* prior 1 -> background */
    CvSsdConfig cfg = {.num_priors = np, .num_classes = nc, .priors = priors,
                       .var_xy = 0.1f, .var_wh = 0.2f, .in_width = 64, .in_height = 64,
                       .conf_threshold = 0.5f, .iou_threshold = 0.45f};
    CvNnDetection out[16];
    size_t n = 0;
    assert(cv_detect_ssd_decode(loc, conf, &cfg, out, 16, &n) == EMBEDDIP_OK);
    assert(n == 1);
    assert(out[0].cls == 1);
    assert(fabsf(out[0].score - 0.9f) < 1e-5f);
    /* box: cx,cy,w,h = 0.5,0.5,0.25,0.25 -> (0.375*64, .., 0.25*64) = (24,24,16,16) */
    assert(near(out[0].box.x, 24, 1) && near(out[0].box.y, 24, 1));
    assert(near(out[0].box.width, 16, 1) && near(out[0].box.height, 16, 1));

    /* Threshold above the top score -> nothing. */
    cfg.conf_threshold = 0.95f;
    assert(cv_detect_ssd_decode(loc, conf, &cfg, out, 16, &n) == EMBEDDIP_OK);
    assert(n == 0);
}

int main(void)
{
    test_yolo();
    test_ssd();

    /* Argument validation. */
    float dummy = 0.0f, anchors[2] = {1, 1};
    CvYoloConfig yc = {.grid_w = 2, .grid_h = 2, .num_anchors = 1, .num_classes = 2,
                       .anchors = anchors, .in_width = 64, .in_height = 64,
                       .conf_threshold = 0.5f, .iou_threshold = 0.45f};
    CvNnDetection out[4];
    size_t n = 0;
    assert(cv_detect_yolo_decode(NULL, &yc, out, 4, &n) == EMBEDDIP_ERROR_NULL_PTR);
    yc.grid_w = 0;
    assert(cv_detect_yolo_decode(&dummy, &yc, out, 4, &n) == EMBEDDIP_ERROR_INVALID_SIZE);

    CvSsdConfig sc = {.num_priors = 1, .num_classes = 1, .priors = &dummy, .var_xy = 0.1f,
                      .var_wh = 0.2f, .in_width = 64, .in_height = 64,
                      .conf_threshold = 0.5f, .iou_threshold = 0.45f};
    assert(cv_detect_ssd_decode(&dummy, &dummy, &sc, out, 4, &n) == EMBEDDIP_ERROR_INVALID_ARG);
    return 0;
}
