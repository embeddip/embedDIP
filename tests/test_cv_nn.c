#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include <cv/nn.h>
#include <runtime/runtime.h>

static int approx(float a, float b) { return fabsf(a - b) < 1e-4f; }

static void test_image_to_tensor(void)
{
    uint8_t pixels[4] = {0u, 255u, 51u, 204u};
    ImageView src = {
        .pixels = pixels, .width = 2u, .height = 2u, .row_stride_bytes = 2u,
        .format = IMAGE_FORMAT_GRAYSCALE, .depth = IMAGE_DEPTH_U8,
        .region = EMBEDDIP_MEMORY_REGION_DEFAULT, .flags = 0u};

    /* F32 path: pixel/255. */
    float fbuf[4] = {0};
    cv_tensor_t ft = {.data = fbuf, .bytes = sizeof(fbuf), .width = 2u, .height = 2u,
                      .channels = 1u, .type = CV_TENSOR_F32, .layout = CV_TENSOR_HWC,
                      .scale = 0.0f, .zero_point = 0};
    assert(cv_nn_image_to_tensor(&src, &ft) == EMBEDDIP_OK);
    assert(approx(fbuf[0], 0.0f));
    assert(approx(fbuf[1], 1.0f));
    assert(approx(fbuf[2], 51.0f / 255.0f));

    /* I8 path: scale 1/255, zero_point -128 -> normalized in [0,1] maps to [-128,127]. */
    int8_t ibuf[4] = {0};
    cv_tensor_t it = {.data = ibuf, .bytes = sizeof(ibuf), .width = 2u, .height = 2u,
                      .channels = 1u, .type = CV_TENSOR_I8, .layout = CV_TENSOR_HWC,
                      .scale = 1.0f / 255.0f, .zero_point = -128};
    assert(cv_nn_image_to_tensor(&src, &it) == EMBEDDIP_OK);
    assert(ibuf[0] == -128);           /* 0/255 -> 0 -> -128 */
    assert(ibuf[1] == 127);            /* 255/255=1 -> 255 -128=127 */

    /* U8 path: scale 1/255, zero_point 0 -> identity to pixel value. */
    uint8_t ubuf[4] = {0};
    cv_tensor_t ut = {.data = ubuf, .bytes = sizeof(ubuf), .width = 2u, .height = 2u,
                      .channels = 1u, .type = CV_TENSOR_U8, .layout = CV_TENSOR_HWC,
                      .scale = 1.0f / 255.0f, .zero_point = 0};
    assert(cv_nn_image_to_tensor(&src, &ut) == EMBEDDIP_OK);
    assert(ubuf[0] == 0 && ubuf[1] == 255 && ubuf[2] == 51 && ubuf[3] == 204);

    /* Rejections. */
    ft.width = 3u;
    assert(cv_nn_image_to_tensor(&src, &ft) == EMBEDDIP_ERROR_INVALID_SIZE);
    ft.width = 2u;
    ft.channels = 3u;
    assert(cv_nn_image_to_tensor(&src, &ft) == EMBEDDIP_ERROR_NOT_SUPPORTED);
    ft.channels = 1u;
    it.scale = 0.0f;
    assert(cv_nn_image_to_tensor(&src, &it) == EMBEDDIP_ERROR_INVALID_ARG);
    cv_tensor_t nul = ft;
    nul.data = NULL;
    assert(cv_nn_image_to_tensor(&src, &nul) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_argmax_softmax(void)
{
    float scores[4] = {0.1f, 0.7f, 0.7f, 0.2f};
    size_t idx = 99u;
    float val = 0.0f;

    assert(cv_nn_argmax(scores, 4u, &idx, &val) == EMBEDDIP_OK);
    assert(idx == 1u); /* tie -> lowest index */
    assert(approx(val, 0.7f));
    assert(cv_nn_argmax(scores, 0u, &idx, &val) == EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_nn_argmax(NULL, 4u, &idx, &val) == EMBEDDIP_ERROR_NULL_PTR);

    float logits[3] = {1.0f, 2.0f, 3.0f};
    assert(cv_nn_softmax(logits, 3u) == EMBEDDIP_OK);
    float sum = logits[0] + logits[1] + logits[2];
    assert(approx(sum, 1.0f));
    assert(logits[2] > logits[1] && logits[1] > logits[0]);

    /* Large logits: no overflow thanks to max subtraction. */
    float big[2] = {1000.0f, 1001.0f};
    assert(cv_nn_softmax(big, 2u) == EMBEDDIP_OK);
    assert(approx(big[0] + big[1], 1.0f));
    assert(big[1] > big[0]);
}

static void test_segmentation(void)
{
    /* 2x1 image, 3 classes, HWC. Pixel0 -> class 2, pixel1 -> class 0. */
    float hwc[6] = {
        0.1f, 0.2f, 0.9f, /* pixel 0: class 2 */
        0.8f, 0.1f, 0.1f, /* pixel 1: class 0 */
    };
    cv_tensor_t hwc_t = {.data = hwc, .bytes = sizeof(hwc), .width = 2u, .height = 1u,
                         .channels = 3u, .type = CV_TENSOR_F32, .layout = CV_TENSOR_HWC,
                         .scale = 0.0f, .zero_point = 0};
    uint8_t map[2] = {0};
    assert(cv_nn_segmentation_argmax(&hwc_t, map, 2u) == EMBEDDIP_OK);
    assert(map[0] == 2u && map[1] == 0u);

    /* Same data in CHW layout. */
    float chw[6] = {
        0.1f, 0.8f, /* channel 0 */
        0.2f, 0.1f, /* channel 1 */
        0.9f, 0.1f, /* channel 2 */
    };
    cv_tensor_t chw_t = hwc_t;
    chw_t.data = chw;
    chw_t.layout = CV_TENSOR_CHW;
    assert(cv_nn_segmentation_argmax(&chw_t, map, 2u) == EMBEDDIP_OK);
    assert(map[0] == 2u && map[1] == 0u);

    /* Capacity rejection. */
    assert(cv_nn_segmentation_argmax(&hwc_t, map, 1u) == EMBEDDIP_ERROR_INVALID_SIZE);

    /* Colorize. */
    uint8_t palette[9] = {255, 0, 0, /*c0*/ 0, 255, 0, /*c1*/ 0, 0, 255 /*c2*/};
    uint8_t rgb[6] = {0};
    uint8_t cmap[2] = {2u, 0u};
    assert(cv_nn_colorize(cmap, 2u, 1u, palette, 3u, rgb, sizeof(rgb)) == EMBEDDIP_OK);
    assert(rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 255);   /* class 2 = blue */
    assert(rgb[3] == 255 && rgb[4] == 0 && rgb[5] == 0);   /* class 0 = red */

    /* Class index beyond palette -> out of range. */
    uint8_t bad[2] = {5u, 0u};
    assert(cv_nn_colorize(bad, 2u, 1u, palette, 3u, rgb, sizeof(rgb)) ==
           EMBEDDIP_ERROR_OUT_OF_RANGE);
    /* Capacity rejection. */
    assert(cv_nn_colorize(cmap, 2u, 1u, palette, 3u, rgb, 5u) ==
           EMBEDDIP_ERROR_INVALID_SIZE);
}

int main(void)
{
    test_image_to_tensor();
    test_argmax_softmax();
    test_segmentation();
    return 0;
}
