#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <cv/keypoint.h>

/* 60x60 bright square [20..39] on black: FAST fires at the four corners. */
#define WD 60
#define SQ0 20
#define SQ1 39
static uint8_t img[WD * WD];

static void make_square(int shift)
{
    for (int y = 0; y < WD; ++y)
        for (int x = 0; x < WD; ++x) {
            int xx = x - shift;
            img[y * WD + x] =
                (xx >= SQ0 && xx <= SQ1 && y >= SQ0 && y <= SQ1) ? 255u : 0u;
        }
}

static int near_corner(int x, int y, int shift, int tol)
{
    const int cx[4] = {SQ0 + shift, SQ1 + shift, SQ0 + shift, SQ1 + shift};
    const int cy[4] = {SQ0, SQ0, SQ1, SQ1};
    for (int i = 0; i < 4; ++i) {
        int dx = x - cx[i], dy = y - cy[i];
        if (dx * dx + dy * dy <= tol * tol)
            return i;
    }
    return -1;
}

static ImageView view(void)
{
    ImageView v = {.pixels = img, .width = WD, .height = WD, .row_stride_bytes = WD,
                   .format = IMAGE_FORMAT_GRAYSCALE, .depth = IMAGE_DEPTH_U8,
                   .region = EMBEDDIP_MEMORY_REGION_DEFAULT, .flags = 0u};
    return v;
}

int main(void)
{
    /* --- FAST detection --- */
    make_square(0);
    ImageView v = view();
    CvKeypoint kps[64];
    size_t n = 0;
    assert(cv_fast_detect(&v, 20u, true, kps, 64, &n) == EMBEDDIP_OK);
    assert(n >= 4);
    int hit[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < n; ++i) {
        int c = near_corner(kps[i].x, kps[i].y, 0, 3);
        assert(c >= 0);        /* no spurious corners */
        hit[c] = 1;
    }
    assert(hit[0] && hit[1] && hit[2] && hit[3]);
    for (size_t i = 1; i < n; ++i)
        assert(kps[i - 1].score >= kps[i].score); /* sorted desc */

    /* --- BRIEF describe + self Hamming match (distance 0) --- */
    uint8_t desc[64 * CV_BRIEF_BYTES];
    assert(cv_brief_describe(&v, kps, n, desc, sizeof(desc)) == EMBEDDIP_OK);

    CvMatch m[64];
    size_t nm = 0;
    assert(cv_hamming_match(desc, n, desc, n, 0u, m, 64, &nm) == EMBEDDIP_OK);
    assert(nm == n);
    for (size_t i = 0; i < nm; ++i) {
        assert(m[i].query_idx == m[i].train_idx); /* best match is itself */
        assert(m[i].distance == 0u);
    }

    /* distinct corners must have distinct descriptors (non-zero distance) */
    assert(memcmp(desc + 0 * CV_BRIEF_BYTES, desc + 1 * CV_BRIEF_BYTES, CV_BRIEF_BYTES) != 0);

    /* --- Translation invariance: same corners, image shifted +5 in x --- */
    make_square(0);
    CvKeypoint k0[64];
    size_t n0 = 0;
    assert(cv_fast_detect(&v, 20u, true, k0, 64, &n0) == EMBEDDIP_OK);
    uint8_t d0[64 * CV_BRIEF_BYTES];
    assert(cv_brief_describe(&v, k0, n0, d0, sizeof(d0)) == EMBEDDIP_OK);

    make_square(5);
    CvKeypoint k1[64];
    size_t n1 = 0;
    assert(cv_fast_detect(&v, 20u, true, k1, 64, &n1) == EMBEDDIP_OK);
    uint8_t d1[64 * CV_BRIEF_BYTES];
    assert(cv_brief_describe(&v, k1, n1, d1, sizeof(d1)) == EMBEDDIP_OK);

    /* Each shifted corner should match a corner in the original at small
     * Hamming distance and the matched pair differs by ~+5 in x. */
    CvMatch mm[64];
    size_t nmm = 0;
    assert(cv_hamming_match(d1, n1, d0, n0, 40u, mm, 64, &nmm) == EMBEDDIP_OK);
    assert(nmm >= 4);
    for (size_t i = 0; i < nmm; ++i) {
        int dx = k1[mm[i].query_idx].x - k0[mm[i].train_idx].x;
        int dy = k1[mm[i].query_idx].y - k0[mm[i].train_idx].y;
        assert(dx >= 3 && dx <= 7); /* recovered the +5 x-shift */
        assert(dy >= -2 && dy <= 2);
    }

    /* --- ORB orientation (intensity centroid) sanity, known GT --- */
    static uint8_t hp[WD * WD];
    /* bright right half -> centroid points +x -> angle ~ 0 */
    for (int y = 0; y < WD; ++y)
        for (int x = 0; x < WD; ++x)
            hp[y * WD + x] = (x > WD / 2) ? 255u : 0u;
    ImageView hv = view();
    hv.pixels = hp;
    CvKeypoint kc = {.x = WD / 2, .y = WD / 2, .score = 1.0f, .angle = 0.0f};
    assert(cv_keypoint_orient(&hv, &kc, 1) == EMBEDDIP_OK);
    assert(fabsf(kc.angle) < 0.2f); /* points toward +x */
    /* bright bottom half -> centroid points +y -> angle ~ +pi/2 */
    for (int y = 0; y < WD; ++y)
        for (int x = 0; x < WD; ++x)
            hp[y * WD + x] = (y > WD / 2) ? 255u : 0u;
    kc.angle = 0.0f;
    assert(cv_keypoint_orient(&hv, &kc, 1) == EMBEDDIP_OK);
    assert(fabsf(kc.angle - (float)M_PI / 2.0f) < 0.2f);

    /* --- rBRIEF rotation invariance: exact 90 deg rotation about center --- */
    /* WD is even, so use an odd square with a fixed textured, asymmetric pattern
     * whose centre pixel is invariant under a 90 deg rotation. */
#define RW 61
    static uint8_t A[RW * RW], B[RW * RW];
    for (int y = 0; y < RW; ++y)
        for (int x = 0; x < RW; ++x)
            A[y * RW + x] = (uint8_t)((x * 7 + y * 13 + ((x * x + 3 * y) & 31) * 5) & 0xFF);
    /* B = A rotated 90 deg CCW about centre: B[y][x] = A[x][RW-1-y] */
    for (int y = 0; y < RW; ++y)
        for (int x = 0; x < RW; ++x)
            B[y * RW + x] = A[x * RW + (RW - 1 - y)];

    ImageView av = {.pixels = A, .width = RW, .height = RW, .row_stride_bytes = RW,
                    .format = IMAGE_FORMAT_GRAYSCALE, .depth = IMAGE_DEPTH_U8,
                    .region = EMBEDDIP_MEMORY_REGION_DEFAULT, .flags = 0u};
    ImageView bv = av;
    bv.pixels = B;
    CvKeypoint ka = {.x = RW / 2, .y = RW / 2, .score = 1.0f, .angle = 0.0f};
    CvKeypoint kb = ka; /* centre pixel maps to itself under the rotation */

    uint8_t da[CV_BRIEF_BYTES], db[CV_BRIEF_BYTES];
    CvMatch mo[1];
    size_t one = 0;

    /* plain BRIEF (angle 0): rotated content -> large Hamming distance */
    assert(cv_brief_describe(&av, &ka, 1, da, sizeof(da)) == EMBEDDIP_OK);
    assert(cv_brief_describe(&bv, &kb, 1, db, sizeof(db)) == EMBEDDIP_OK);
    cv_hamming_match(da, 1, db, 1, 256u, mo, 1, &one);
    uint32_t plain_dist = mo[0].distance;

    /* oriented rBRIEF: steering should cancel the rotation -> small distance */
    assert(cv_keypoint_orient(&av, &ka, 1) == EMBEDDIP_OK);
    assert(cv_keypoint_orient(&bv, &kb, 1) == EMBEDDIP_OK);
    assert(cv_brief_describe(&av, &ka, 1, da, sizeof(da)) == EMBEDDIP_OK);
    assert(cv_brief_describe(&bv, &kb, 1, db, sizeof(db)) == EMBEDDIP_OK);
    cv_hamming_match(da, 1, db, 1, 256u, mo, 1, &one);
    uint32_t oriented_dist = mo[0].distance;

    /* the two orientations must differ by ~90 deg, and steering must make the
     * oriented descriptor far closer than the unsteered one. */
    assert(oriented_dist < plain_dist);
    assert(oriented_dist * 2 < plain_dist); /* clear margin, not marginal */

    /* --- ORB pyramid: runs, in-bounds, sorted, scale-invariant match --- */
    CvKeypoint ok[128];
    static uint8_t od[128 * CV_BRIEF_BYTES];
    size_t no = 0;
    CvOrbConfig ocfg = {.nlevels = 3, .fast_threshold = 15u};
    assert(cv_orb_detect_and_describe(&av, &ocfg, ok, od, 128, &no) == EMBEDDIP_OK);
    assert(no > 0);
    for (size_t i = 0; i < no; ++i) {
        assert(ok[i].x >= 0 && ok[i].x < RW && ok[i].y >= 0 && ok[i].y < RW);
        if (i)
            assert(ok[i - 1].score >= ok[i].score);
    }
    /* Scale invariance: A2 = A downsampled 2x. ORB(A) includes level-1 features
     * that live at A2's level-0 scale, so cross-matching A<->A2 must find some
     * low-distance correspondences (single-scale BRIEF across 2x cannot). */
#define HW (RW / 2)
    static uint8_t A2[HW * HW];
    for (int y = 0; y < HW; ++y)
        for (int x = 0; x < HW; ++x) {
            int s = A[(2 * y) * RW + 2 * x] + A[(2 * y) * RW + 2 * x + 1] +
                    A[(2 * y + 1) * RW + 2 * x] + A[(2 * y + 1) * RW + 2 * x + 1];
            A2[y * HW + x] = (uint8_t)((s + 2) / 4);
        }
    ImageView a2v = {.pixels = A2, .width = HW, .height = HW, .row_stride_bytes = HW,
                     .format = IMAGE_FORMAT_GRAYSCALE, .depth = IMAGE_DEPTH_U8,
                     .region = EMBEDDIP_MEMORY_REGION_DEFAULT, .flags = 0u};
    CvKeypoint ok2[128];
    static uint8_t od2[128 * CV_BRIEF_BYTES];
    size_t no2 = 0;
    assert(cv_orb_detect_and_describe(&a2v, &ocfg, ok2, od2, 128, &no2) == EMBEDDIP_OK);
    assert(no2 > 0);
    CvMatch mm2[128];
    size_t nmm2 = 0;
    assert(cv_hamming_match(od, no, od2, no2, 40u, mm2, 128, &nmm2) == EMBEDDIP_OK);
    assert(nmm2 > 0); /* some feature matches survive the 2x scale gap */

    /* Argument validation for ORB. */
    ocfg.nlevels = 0;
    assert(cv_orb_detect_and_describe(&av, &ocfg, ok, od, 128, &no) == EMBEDDIP_ERROR_INVALID_ARG);

    /* --- Argument validation --- */
    assert(cv_fast_detect(NULL, 20u, true, kps, 64, &n) == EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_fast_detect(&v, 20u, true, kps, 0, &n) == EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_brief_describe(&v, kps, n, desc, 1u) == EMBEDDIP_ERROR_INVALID_SIZE);
    v.depth = IMAGE_DEPTH_F32;
    assert(cv_fast_detect(&v, 20u, true, kps, 64, &n) == EMBEDDIP_ERROR_INVALID_DEPTH);

    return 0;
}
