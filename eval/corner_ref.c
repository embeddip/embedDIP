/* Host reference (== on-device code): run embedDIP Harris on a raw grayscale
 * frame. Prints "x y" per corner. Args: raw W H [cap] [harris_thr] [nms]. */
#include <stdio.h>
#include <stdlib.h>

#include "core/image.h"
#include "imgproc/corner.h"

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s raw.u8 W H [cap] [thr] [nms]\n", argv[0]);
        return 2;
    }
    int W = atoi(argv[2]), H = atoi(argv[3]);
    size_t cap = (argc > 4) ? (size_t)atoi(argv[4]) : 64;
    float thr = (argc > 5) ? (float)atof(argv[5]) : 100000.0f;
    int nms = (argc > 6) ? atoi(argv[6]) : 3;

    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    uint8_t *buf = malloc((size_t)W * H);
    if (fread(buf, 1, (size_t)W * H, f) != (size_t)W * H) return 2;
    fclose(f);

    ImageView v = {.pixels = buf, .width = (uint32_t)W, .height = (uint32_t)H,
                   .row_stride_bytes = (uint32_t)W, .format = IMAGE_FORMAT_GRAYSCALE,
                   .depth = IMAGE_DEPTH_U8, .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
                   .flags = 0u};
    CvCornerConfig cfg = {.method = CV_CORNER_HARRIS, .block_size = 3, .k = 0.04f,
                          .threshold = thr, .nms_radius = nms};
    CvCorner *c = malloc(cap * sizeof(CvCorner));
    size_t n = 0;
    cv_corner_detect(&v, &cfg, c, cap, &n);
    for (size_t i = 0; i < n; ++i)
        printf("%d %d\n", c[i].x, c[i].y);
    return 0;
}
