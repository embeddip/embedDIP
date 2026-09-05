/* Host reference (== on-device code): FAST keypoints + optional ORB orientation
 * + BRIEF/rBRIEF descriptors for one raw grayscale frame. Prints one line per
 * keypoint: "x y angle <64 hex chars>". Args: raw W H oriented(0/1) [cap] [thr]. */
#include <stdio.h>
#include <stdlib.h>

#include "core/image.h"
#include "cv/keypoint.h"

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: %s raw.u8 W H oriented [cap] [fast_thr]\n", argv[0]);
        return 2;
    }
    int W = atoi(argv[2]), H = atoi(argv[3]), oriented = atoi(argv[4]);
    size_t cap = (argc > 5) ? (size_t)atoi(argv[5]) : 1000;
    int thr = (argc > 6) ? atoi(argv[6]) : 20;

    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    uint8_t *buf = malloc((size_t)W * H);
    if (fread(buf, 1, (size_t)W * H, f) != (size_t)W * H) return 2;
    fclose(f);

    ImageView v = {.pixels = buf, .width = (uint32_t)W, .height = (uint32_t)H,
                   .row_stride_bytes = (uint32_t)W, .format = IMAGE_FORMAT_GRAYSCALE,
                   .depth = IMAGE_DEPTH_U8, .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
                   .flags = 0u};

    CvKeypoint *kps = malloc(cap * sizeof(CvKeypoint));
    uint8_t *desc = malloc(cap * CV_BRIEF_BYTES);
    size_t n = 0;
    if (oriented == 2) {
        /* scale-invariant ORB pyramid */
        CvOrbConfig ocfg = {.nlevels = 4, .fast_threshold = (uint8_t)thr};
        cv_orb_detect_and_describe(&v, &ocfg, kps, desc, cap, &n);
    } else {
        cv_fast_detect(&v, (uint8_t)thr, true, kps, cap, &n);
        if (oriented)
            cv_keypoint_orient(&v, kps, n);
        cv_brief_describe(&v, kps, n, desc, cap * CV_BRIEF_BYTES);
    }

    for (size_t i = 0; i < n; ++i) {
        printf("%d %d %.4f ", kps[i].x, kps[i].y, kps[i].angle);
        for (int b = 0; b < CV_BRIEF_BYTES; ++b)
            printf("%02x", desc[i * CV_BRIEF_BYTES + b]);
        printf("\n");
    }
    return 0;
}
