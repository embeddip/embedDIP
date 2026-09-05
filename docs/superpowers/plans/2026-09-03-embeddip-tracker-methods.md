# embedDIP Tracker Method Expansion — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add four survey-identified trackers to embedDIP's `cv/` library — SORT association, a proper particle-filter appearance likelihood, mean shift, and KCF online update — by porting the tested `object-trackers/` reference code into the library contract, then demo on STM32H7.

**Architecture:** Each method is a `cv/` module (or an extension of an existing one) with a caller-owned state struct, portable C math, and an assert-based host test. A shared histogram/Bhattacharyya helper (`cv/track_hist`) backs the two histogram trackers. A final CubeIDE project under `object-trackers/` links the library and streams frames from the PC via PyDIPLink for on-hardware validation.

**Tech Stack:** C11, CMake, CTest (host), `<math.h>` (portable core), STM32H7 HAL + PyDIPLink (demo only). Reference sources: `../object-trackers/{F429_Tracker,H7_KCF}/Core/Src/*.c`.

## Global Constraints

- Input formats: W1/W4 grayscale only (`IMAGE_FORMAT_GRAYSCALE`/`MASK`, `IMAGE_DEPTH_U8`, `row_stride_bytes >= width`). W2/W3 (histogram trackers) also accept `IMAGE_FORMAT_RGB565`; RGB565 is the color path (2 B/px), each 5/6/5 channel quantized to a fixed bin count (3×32).
- No new per-frame heap allocation in `cv/`. State in caller-visible structs; working memory is a caller-owned scratch buffer. Reference globals become state fields.
- Portable math in the core path (`<math.h>`), never `arm_*`/CMSIS in the portable `cv/` source. SIMD only behind `arch/` if ever needed.
- Every module ships one assert-based host test in `tests/`, registered in `tests/CMakeLists.txt`, and its source listed in the root `CMakeLists.txt` `CV_SOURCES` block (lines 93–115).
- Doc-comment headers match existing `cv/*.h` Doxygen style. SPDX + copyright header on every new file:
  `// SPDX-License-Identifier: MIT` / `// Copyright (c) 2025 EmbedDIP`.
- Error codes: return `EMBEDDIP_OK` (0); `EMBEDDIP_ERROR_NULL_PTR` (-2), `EMBEDDIP_ERROR_INVALID_ARG` (-3), `EMBEDDIP_ERROR_INVALID_FORMAT` (-4), `EMBEDDIP_ERROR_INVALID_SIZE` (-5), `EMBEDDIP_ERROR_NOT_INITIALIZED` (-8) as appropriate.
- Host build/test recipe (already configured in `build/`):
  `cmake -S . -B build -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build`
- Commit per task. Message style follows repo: `feat:`/`fix:` prefix. **No `Co-Authored-By: Claude` trailer** (repo rule).

Reference structs (already in the tree):
- `ImageView { uint8_t *pixels; uint32_t width,height,row_stride_bytes; ImageFormat format; ImageDepth depth; embeddip_memory_region_t region; uint32_t flags; }` (`core/image.h`)
- `Rectangle { int32_t x,y,width,height; }` (`core/image.h`)
- `CvDetection { Rectangle box; int32_t score; }` (`cv/detect.h`)
- `CvKalmanState` with `cv_kalman_init(CvKalmanState*, Rectangle)`, `cv_kalman_predict(CvKalmanState*, Rectangle *out_box)`, `cv_kalman_update(CvKalmanState*, Rectangle measured_box)` (`cv/tracker_kalman.h`)

---

### Task 1: SORT association (`cv/track_assoc`)

Greedy IoU association between Haar detections and Kalman-backed tracks. No reference port — written fresh.

**Files:**
- Create: `cv/track_assoc.h`, `cv/track_assoc.c`
- Test: `tests/test_cv_track_assoc.c`
- Modify: `CMakeLists.txt` (add to `CV_SOURCES`), `tests/CMakeLists.txt` (register test)

**Interfaces:**
- Consumes: `Rectangle`, `CvDetection`, `CvKalmanState` + its three functions.
- Produces:
```c
#define CV_TRACK_MAX 16u
typedef struct {
    CvKalmanState kf;
    Rectangle box;      /* last estimate */
    int32_t id;         /* stable track id, >=0 */
    uint16_t age;       /* frames since spawn */
    uint16_t misses;    /* consecutive frames without a match */
    bool active;
} CvTrack;
typedef struct {
    CvTrack tracks[CV_TRACK_MAX];
    int32_t next_id;
    float iou_threshold;   /* match if IoU >= this (e.g. 0.3) */
    uint16_t max_misses;   /* drop track after this many misses (e.g. 5) */
} CvTracker;

embeddip_status_t cv_track_init(CvTracker *t, float iou_threshold, uint16_t max_misses);
/* Advance one frame: predict all tracks, greedily match to detections by IoU,
 * correct matched tracks, spawn tracks for unmatched detections, age/drop the rest. */
embeddip_status_t cv_track_step(CvTracker *t, const CvDetection *dets, size_t det_count);
float cv_track_iou(Rectangle a, Rectangle b);  /* exposed for testing */
```

- [ ] **Step 1: Write the failing test** — `tests/test_cv_track_assoc.c`

```c
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <core/image.h>
#include <cv/track_assoc.h>

static Rectangle R(int x,int y,int w,int h){ Rectangle r={x,y,w,h}; return r; }

static void test_iou(void){
    assert(cv_track_iou(R(0,0,10,10), R(0,0,10,10)) > 0.99f);
    assert(cv_track_iou(R(0,0,10,10), R(100,100,10,10)) == 0.0f);
    float half = cv_track_iou(R(0,0,10,10), R(5,0,10,10)); /* overlap 50/150 */
    assert(half > 0.32f && half < 0.34f);
}

static void test_null(void){
    assert(cv_track_init(NULL, 0.3f, 5) == EMBEDDIP_ERROR_NULL_PTR);
    CvTracker t; cv_track_init(&t,0.3f,5);
    assert(cv_track_step(NULL, NULL, 0) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_spawn_and_persist(void){
    CvTracker t; assert(cv_track_init(&t,0.3f,5)==EMBEDDIP_OK);
    CvDetection d = { R(50,50,20,20), 100 };
    assert(cv_track_step(&t,&d,1)==EMBEDDIP_OK);
    /* one track spawned with id 0 */
    int active=0, id=-1;
    for(size_t i=0;i<CV_TRACK_MAX;i++) if(t.tracks[i].active){active++; id=t.tracks[i].id;}
    assert(active==1 && id==0);
    /* move detection slightly; same track keeps its id */
    CvDetection d2 = { R(53,52,20,20), 100 };
    assert(cv_track_step(&t,&d2,1)==EMBEDDIP_OK);
    active=0; for(size_t i=0;i<CV_TRACK_MAX;i++) if(t.tracks[i].active){active++; assert(t.tracks[i].id==0);}
    assert(active==1);
}

static void test_miss_then_drop(void){
    CvTracker t; cv_track_init(&t,0.3f,2);
    CvDetection d = { R(10,10,20,20), 100 };
    cv_track_step(&t,&d,1);
    /* no detections for max_misses+1 frames -> track dropped */
    for(int k=0;k<3;k++) cv_track_step(&t,NULL,0);
    for(size_t i=0;i<CV_TRACK_MAX;i++) assert(!t.tracks[i].active);
}

int main(void){ test_iou(); test_null(); test_spawn_and_persist(); test_miss_then_drop(); return 0; }
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build -R cv_track_assoc -V`
Expected: build FAILS — `cv/track_assoc.h` not found.

- [ ] **Step 3: Write the header** `cv/track_assoc.h`

Full SPDX header, include guard `EMBEDDIP_CV_TRACK_ASSOC_H`, includes `core/error.h`, `core/image.h`, `cv/detect.h`, `cv/tracker_kalman.h`, `extern "C"` guard, then the Interfaces block above with Doxygen comments.

- [ ] **Step 4: Write the implementation** `cv/track_assoc.c`

Key logic (no external algorithm — write directly):
- `cv_track_iou`: intersection area / union area; return 0 on non-overlap or non-positive union.
- `cv_track_init`: null-check, zero the struct, store thresholds, `next_id=0`.
- `cv_track_step`:
  1. For each active track: `cv_kalman_predict(&trk->kf, &trk->box)`.
  2. Build IoU for every (active track, detection) pair. Greedy: repeatedly pick the highest IoU ≥ `iou_threshold` among unused pairs; mark that track+detection matched.
  3. Matched track: `cv_kalman_update(&trk->kf, det.box)`, set `trk->box=det.box`, `misses=0`, `age++`.
  4. Unmatched detection: find a free slot (`!active`), `cv_kalman_init(&trk->kf, det.box)`, `id=next_id++`, `active=true`, `age=0`, `misses=0`. If no free slot, drop the detection.
  5. Unmatched track: `misses++`; if `misses > max_misses` set `active=false`.
- `ponytail:` greedy O(n·m) match; upgrade to Hungarian only if measured ID-switch rate demands.

- [ ] **Step 5: Register in build** — add `cv/track_assoc.c` + `cv/track_assoc.h` to `CV_SOURCES` in `CMakeLists.txt` (after the `cv/detect.*` lines), and to `tests/CMakeLists.txt`:

```cmake
add_executable(embeddip_test_cv_track_assoc test_cv_track_assoc.c)
target_link_libraries(embeddip_test_cv_track_assoc PRIVATE embedDIP)
add_test(NAME embeddip.cv_track_assoc COMMAND embeddip_test_cv_track_assoc)
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cmake -S . -B build -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build -R cv_track_assoc -V`
Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add cv/track_assoc.h cv/track_assoc.c tests/test_cv_track_assoc.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add SORT-style IoU/Kalman track association (cv/track_assoc)"
```

---

### Task 2: Shared histogram + Bhattacharyya helper (`cv/track_hist`)

DRY foundation for W2 and W3. Ports the histogram math from `../object-trackers/F429_Tracker/Core/Src/Bhattacharya.c` (`ColorHist`, `BhattacharyaDistance`) adapted to RGB565/grayscale and portable libm.

**Files:**
- Create: `cv/track_hist.h`, `cv/track_hist.c`
- Test: `tests/test_cv_track_hist.c`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ImageView`, `Rectangle`, `ImageFormat`.
- Produces:
```c
/* Grayscale: 32 bins. RGB565: 3x32 = 96 bins (R,G,B blocks). */
#define CV_HIST_GRAY_BINS 32u
#define CV_HIST_COLOR_BINS 96u
#define CV_HIST_MAX_BINS 96u

/* Build a normalized (sum=1) histogram over roi of a grayscale or RGB565 view.
 * out must hold CV_HIST_MAX_BINS floats. out_nbins reports bins actually used
 * (32 gray, 96 color). roi is clamped to the image. */
embeddip_status_t cv_hist_build(const ImageView *img, Rectangle roi,
                                float *out, uint32_t *out_nbins);

/* Bhattacharyya similarity in [0,1]: exp(-k * sqrt(1 - sum(sqrt(p*q)))).
 * p and q are normalized histograms of nbins each. Higher = more similar. */
float cv_hist_bhattacharyya(const float *p, const float *q, uint32_t nbins);
```

- [ ] **Step 1: Write the failing test** — `tests/test_cv_track_hist.c`

```c
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <math.h>
#include <string.h>
#include <core/image.h>
#include <cv/track_hist.h>

int main(void){
    /* 8x8 grayscale, left half 0, right half 255 */
    uint8_t g[64];
    for(int y=0;y<8;y++) for(int x=0;x<8;x++) g[y*8+x] = (x<4)?0:255;
    ImageView gv = {g,8,8,8,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    Rectangle full = {0,0,8,8};
    float h1[CV_HIST_MAX_BINS], h2[CV_HIST_MAX_BINS]; uint32_t nb=0;
    assert(cv_hist_build(&gv, full, h1, &nb)==EMBEDDIP_OK && nb==CV_HIST_GRAY_BINS);
    float sum=0; for(uint32_t i=0;i<nb;i++) sum+=h1[i];
    assert(fabsf(sum-1.0f) < 1e-4f);                 /* normalized */
    /* identical histogram -> similarity ~1 */
    memcpy(h2,h1,sizeof(float)*nb);
    assert(cv_hist_bhattacharyya(h1,h2,nb) > 0.99f);
    /* all-black roi -> disjoint from h1 -> low similarity */
    uint8_t b[64]; memset(b,0,64);
    ImageView bv = {b,8,8,8,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    cv_hist_build(&bv, full, h2, &nb);
    assert(cv_hist_bhattacharyya(h1,h2,nb) < 0.8f);
    /* RGB565 path reports 96 bins */
    uint16_t c[16]; for(int i=0;i<16;i++) c[i]=0xF800; /* pure red */
    ImageView cv = {(uint8_t*)c,4,4,8,IMAGE_FORMAT_RGB565,IMAGE_DEPTH_U16,0,0};
    Rectangle r4={0,0,4,4};
    assert(cv_hist_build(&cv, r4, h1, &nb)==EMBEDDIP_OK && nb==CV_HIST_COLOR_BINS);
    assert(cv_hist_build(NULL, r4, h1, &nb)==EMBEDDIP_ERROR_NULL_PTR);
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build -R cv_track_hist -V`
Expected: build FAILS — header not found.

- [ ] **Step 3: Write header + implementation**

`cv/track_hist.h`: SPDX, guard `EMBEDDIP_CV_TRACK_HIST_H`, the Interfaces block.

`cv/track_hist.c` logic:
- `cv_hist_build`: null-check; accept only `IMAGE_FORMAT_GRAYSCALE`/`MASK` (U8) or `IMAGE_FORMAT_RGB565` (U16) else `EMBEDDIP_ERROR_INVALID_FORMAT`. Clamp roi to image. Zero the bins.
  - Grayscale: bin = `pixel >> 3` (256→32). One block of 32.
  - RGB565: for each 16-bit pixel, `r5=(px>>11)&0x1F; g6=(px>>5)&0x3F; b5=px&0x1F;` bins `r5` (0..31), `32 + (g6>>1)` (0..31), `64 + b5` (0..31). (Row stride is in bytes; index pixels as `((uint16_t*)(row_base))[x]`.)
  - Normalize: divide every bin by total pixel-channel count so `sum==1`.
- `cv_hist_bhattacharyya`: `bc = sum_i sqrt(p_i*q_i)`; `d = sqrt(fmaxf(0,1-bc))`; return `expf(-k*d)` with `k=3.0f` (portable; reference used exp(-100*·) on unnormalized-distance — retune constant so identical→~1, disjoint→low; the test pins the direction, not the exact value). Use `<math.h>` only.

- [ ] **Step 4: Register in build** (`CV_SOURCES` + tests/CMakeLists.txt, same pattern as Task 1 Step 5, names `cv/track_hist.*`, test `embeddip_test_cv_track_hist`).

- [ ] **Step 5: Run test to verify it passes**

Run: `cmake -S . -B build -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build -R cv_track_hist -V`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add cv/track_hist.h cv/track_hist.c tests/test_cv_track_hist.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add RGB565/grayscale histogram + Bhattacharyya helper (cv/track_hist)"
```

---

### Task 3: Particle-filter appearance likelihood (`cv/tracker_particle`)

Replace the edge-magnitude weighting with a histogram Bhattacharyya likelihood (ref [1] SIR-PF). Reference: `../object-trackers/F429_Tracker/Core/Src/Bhattacharya.c`.

**Files:**
- Modify: `cv/tracker_particle.h`, `cv/tracker_particle.c`
- Test: `tests/test_cv_tracker_particle.c` (extend existing)

**Interfaces:**
- Consumes: `cv/track_hist` (`cv_hist_build`, `cv_hist_bhattacharyya`, `CV_HIST_MAX_BINS`).
- Produces (add to `CvParticleState`, keep existing fields + behavior):
```c
/* Appended to CvParticleState: */
float template_hist[CV_HIST_MAX_BINS];  /* target model, filled at init */
uint32_t hist_nbins;                     /* 0 = use legacy gradient weighting */
/* New init variant that captures the target histogram from the init frame: */
embeddip_status_t cv_particle_init_hist(CvParticleState *state, uint16_t particle_count,
                                        float *particle_buffer, const ImageView *frame,
                                        Rectangle roi);
```
`cv_particle_update` unchanged in signature. When `hist_nbins>0` it weights each particle by `cv_hist_bhattacharyya(candidate_hist, template_hist)` over the particle's box; otherwise it keeps the existing gradient weighting (back-compat).

- [ ] **Step 1: Write the failing test** — append to `tests/test_cv_tracker_particle.c`

```c
static void test_hist_likelihood_tracks_bright_block(void){
    /* frame: dark bg, one 20x20 bright block; block moves; PF should follow */
    enum { W=128, H=128, NP=200 };
    static uint8_t px[W*H];
    static float pbuf[NP*2];
    CvParticleState st; memset(&st,0,sizeof st);
    /* block at (40,40) */
    memset(px,0,sizeof px);
    for(int y=40;y<60;y++) for(int x=40;x<60;x++) px[y*W+x]=255;
    ImageView f={px,W,H,W,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    Rectangle roi={40,40,20,20};
    assert(cv_particle_init_hist(&st,NP,pbuf,&f,roi)==EMBEDDIP_OK);
    assert(st.hist_nbins==CV_HIST_GRAY_BINS);
    Rectangle out;
    /* move block to (70,70) over a few frames */
    for(int step=0; step<=30; step+=10){
        memset(px,0,sizeof px);
        for(int y=40+step;y<60+step;y++) for(int x=40+step;x<60+step;x++) px[y*W+x]=255;
        assert(cv_particle_update(&st,&f,&out)==EMBEDDIP_OK);
    }
    int cx = out.x + out.width/2, cy = out.y + out.height/2;
    assert(cx > 60 && cy > 60);   /* estimate migrated toward (80,80) */
    cv_particle_free(&st);
}
/* add test_hist_likelihood_tracks_bright_block(); to main() */
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build -R cv_tracker_particle -V`
Expected: build FAILS — `cv_particle_init_hist` / `hist_nbins` undefined.

- [ ] **Step 3: Implement**

- Header: add the two fields and `cv_particle_init_hist` prototype with Doxygen. Include `cv/track_hist.h`.
- `.c`:
  - `cv_particle_init_hist`: call existing init to seed particles, then `cv_hist_build(frame, roi, state->template_hist, &state->hist_nbins)`. Validate format (grayscale or RGB565).
  - In `cv_particle_update`, after diffusing particles, when `hist_nbins>0`: for each particle build its candidate box (center = particle x,y; size = box_width/height, clamp to frame), `cv_hist_build` into a stack scratch histogram, weight = `cv_hist_bhattacharyya(cand, template_hist, hist_nbins)`. Normalize weights, compute weighted-centroid `out_box`, then SIR-resample into the caller buffer (port the resample loop from `Bhattacharya.c`). Reuse the module's existing `xorshift32` RNG (`state->rng_state`), not HAL RNG.
  - Legacy path (`hist_nbins==0`) unchanged.
  - No heap: candidate histogram is a stack `float[CV_HIST_MAX_BINS]`; resample scratch reuses `particle_buffer` with a second pass or a bounded stack index array (document the bound).

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build -R cv_tracker_particle -V`
Expected: PASS (new + existing particle tests).

- [ ] **Step 5: Commit**

```bash
git add cv/tracker_particle.h cv/tracker_particle.c tests/test_cv_tracker_particle.c
git commit -m "feat: add histogram Bhattacharyya likelihood to particle tracker"
```

---

### Task 4: Mean shift tracker (`cv/tracker_meanshift`)

Port `../object-trackers/F429_Tracker/Core/Src/MeanShift.c` to grayscale/RGB565, caller-owned buffers, classic centroid mode-seek.

**Files:**
- Create: `cv/tracker_meanshift.h`, `cv/tracker_meanshift.c`
- Test: `tests/test_cv_tracker_meanshift.c`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `cv/track_hist`.
- Produces:
```c
typedef struct {
    float template_hist[CV_HIST_MAX_BINS];
    uint32_t hist_nbins;
    int32_t box_width, box_height;
    int32_t center_x, center_y;
    uint16_t max_iters;      /* e.g. 5 */
    bool initialized;
} CvMeanShiftState;

embeddip_status_t cv_meanshift_init(CvMeanShiftState *state, const ImageView *frame, Rectangle roi);
/* Iterate: backproject the target histogram to per-pixel weights over the search
 * box, shift the box center to the weighted centroid, repeat up to max_iters or
 * until the shift is < 1 px. Return the converged box. */
embeddip_status_t cv_meanshift_update(CvMeanShiftState *state, const ImageView *frame, Rectangle *out_box);
```

- [ ] **Step 1: Write the failing test** — `tests/test_cv_tracker_meanshift.c`

```c
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <string.h>
#include <core/image.h>
#include <cv/tracker_meanshift.h>

int main(void){
    enum { W=128, H=128 };
    static uint8_t px[W*H];
    CvMeanShiftState st; memset(&st,0,sizeof st);
    memset(px,0,sizeof px);
    for(int y=40;y<60;y++) for(int x=40;x<60;x++) px[y*W+x]=200;
    ImageView f={px,W,H,W,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    Rectangle roi={40,40,20,20};
    assert(cv_meanshift_init(NULL,&f,roi)==EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_meanshift_init(&st,&f,roi)==EMBEDDIP_OK);
    Rectangle out;
    /* shift block by +8,+8 */
    memset(px,0,sizeof px);
    for(int y=48;y<68;y++) for(int x=48;x<68;x++) px[y*W+x]=200;
    assert(cv_meanshift_update(&st,&f,&out)==EMBEDDIP_OK);
    int cx=out.x+out.width/2, cy=out.y+out.height/2;
    assert(cx>=44 && cx<=64 && cy>=44 && cy<=64);   /* tracked toward (58,58) */
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build -R cv_tracker_meanshift -V`
Expected: build FAILS — header not found.

- [ ] **Step 3: Implement**

- `cv_meanshift_init`: validate (null, format grayscale/RGB565, positive roi); store center/size; `cv_hist_build` the target histogram.
- `cv_meanshift_update`: loop up to `max_iters`:
  - Over the current search box (clamped to frame), for each pixel compute its bin, weight = `sqrtf(template_hist[bin])` (backprojection; reference uses `sqrt(q/p)` — use `sqrt(target_bin)` which is the standard mean-shift weight and needs no per-frame candidate histogram), accumulate weighted centroid.
  - Move center to `round(centroid)`; if displacement < 1 px, stop.
  - Clamp center so the box stays in the frame.
  - Write `out_box` (center ± size/2). Set `initialized` check → `EMBEDDIP_ERROR_NOT_INITIALIZED` if not.
- No heap: single pass accumulators (`sum_x,sum_y,sum_w`), stack only.

- [ ] **Step 4: Register in build** (`CV_SOURCES` + tests, names `cv/tracker_meanshift.*`, test `embeddip_test_cv_tracker_meanshift`).

- [ ] **Step 5: Run test to verify it passes**

Run: `cmake -S . -B build -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build -R cv_tracker_meanshift -V`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add cv/tracker_meanshift.h cv/tracker_meanshift.c tests/test_cv_tracker_meanshift.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add mean shift tracker (cv/tracker_meanshift)"
```

---

### Task 5: KCF online update (`cv/tracker_kcf`)

Make `cv_kcf_update` adapt the learned template instead of holding it fixed. Reference: `../object-trackers/H7_KCF/Core/Src/KCFTracker.c`.

**Files:**
- Modify: `cv/tracker_kcf.h` (add learning-rate field + doc), `cv/tracker_kcf.c`
- Test: `tests/test_cv_tracker_kcf.c` (extend existing)

**Interfaces:**
- Produces: add `float learn_rate;` to `CvKcfState` (0 = no adaptation = current behavior; default 0.075f set in `cv_kcf_init`). `cv_kcf_update` signature unchanged.

- [ ] **Step 1: Write the failing test** — append to `tests/test_cv_tracker_kcf.c`

```c
static void test_online_update_survives_appearance_drift(void){
    /* target block whose intensity ramps down over frames while translating;
     * with adaptation the peak stays locked, tracked center keeps up. */
    static uint8_t px[FRAME_W*FRAME_H];
    CvKcfState st; memset(&st,0,sizeof st);
    Rectangle roi={40,40,20,20};
    /* init frame: bright block at 40,40 */
    fill_frame(px,40,40);
    ImageView f={px,FRAME_W,FRAME_H,FRAME_W,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    assert(cv_kcf_init(&st,&f,roi)==EMBEDDIP_OK);
    assert(st.learn_rate > 0.0f);           /* adaptation on by default */
    Rectangle out;
    int bx=40;
    for(int k=0;k<5;k++){
        bx += 6;
        memset(px,0,sizeof px);
        int val = 255 - k*30;               /* appearance drifts */
        for(int y=40;y<60;y++) for(int x=bx;x<bx+20;x++) px[y*FRAME_W+x]=(uint8_t)val;
        assert(cv_kcf_update(&st,&f,&out)==EMBEDDIP_OK);
    }
    int cx=out.x+out.width/2;
    assert(cx > 55);                        /* followed the moving block */
    cv_kcf_free(&st);
}
/* add call in main() */
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build -R cv_tracker_kcf -V`
Expected: FAIL — `learn_rate` undefined (build) or center didn't track (assert).

- [ ] **Step 3: Implement**

- Header: add `float learn_rate;` with Doxygen (0 disables adaptation).
- `cv_kcf_init`: set `state->learn_rate = 0.075f`.
- `cv_kcf_update`: after locating the peak and computing the new center, when `learn_rate>0`, extract the patch at the new center, FFT it (reuse the existing spectrum path), and interpolate the stored template spectrum:
  `template[i] = (1-η)*template[i] + η*new_spectrum[i]` for real and imag channels. Keep the accumulation in the existing `Image` float channels (already higher precision than U8 input — survey caveat ii satisfied). Reuse buffers created in the function; free them before return exactly as the current code does. No new persistent allocation beyond `template_spectrum` (already owned).

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build -R cv_tracker_kcf -V`
Expected: PASS (new + existing kcf tests).

- [ ] **Step 5: Commit**

```bash
git add cv/tracker_kcf.h cv/tracker_kcf.c tests/test_cv_tracker_kcf.c
git commit -m "fix: add online template adaptation to KCF tracker"
```

---

### Task 6: STM32H7 tracking demo project (`object-trackers/H7_EmbedDIP`)

Integrate the new library modules on real M2 hardware; frames streamed from the PC via PyDIPLink; Haar cascade feeds W1 association. This task is validated on-hardware, not by CTest.

**Files:**
- Create: `../object-trackers/H7_EmbedDIP/` — templated from `../object-trackers/Tracker_H7/` (copy `.ioc`, `.cproject`, `.project`, linker scripts, `Core/`, `Drivers/`, `Middlewares/`), then:
  - Replace the standalone tracker `.c`s with a thin `Core/Src/tracker_app.c` that calls the embedDIP library.
  - Add embedDIP as a sibling include/source path (mirror how `examples-stm32` pulls `embedDIP/`), building the `cv/` and `imgproc/` sources for `EMBEDDIP_TARGET_BOARD=STM32H7S`/`CORTEX_M7`.
- Reference host tool: `../PyDIPLink/` (`pydiplink` receiver; `FROM_PC` frame-streaming pattern from `../object-trackers/Tracker_H7/Core/Src/Sequences.c`).

**Interfaces:**
- Consumes all of Tasks 1–5: `cv_track_*`, `cv_particle_*`, `cv_meanshift_*`, `cv_kcf_*`, plus `cv/detect` + `cv/haar` + `cv/integral` for the detection front end.

- [ ] **Step 1:** Copy `Tracker_H7` to `H7_EmbedDIP`; open the `.ioc`, confirm UART + RNG + DMA2D config builds unchanged. Build the untouched copy once in CubeIDE to confirm the baseline compiles.

- [ ] **Step 2:** Wire embedDIP sources into the project build (add `cv/`, `imgproc/`, `core/` to include paths and source folders; define `EMBEDDIP_TARGET_BOARD` for H7). Build; resolve include paths until it links.

- [ ] **Step 3:** Write `Core/Src/tracker_app.c`: receive a frame over UART (PyDIPLink), convert to an `ImageView` (grayscale for KCF/assoc, RGB565 for meanshift/particle), run one selected tracker per build-time `#define TRACKER_*`, send the resulting box back over UART for host overlay.

- [ ] **Step 4:** For the association demo: build the integral image (`cv/integral`), run `cv_detect_scan` + `cv_detect_nms` with a Haar cascade, feed detections to `cv_track_step`, return active track boxes.

- [ ] **Step 5:** On the host, run `pydiplink` to stream a test sequence and record the returned boxes; visually confirm tracking for each of the four methods. Capture latency via the DWT cycle counter and log it over UART.

- [ ] **Step 6: Commit** (in the object-trackers repo/worktree)

```bash
git add object-trackers/H7_EmbedDIP
git commit -m "feat: add STM32H7 embedDIP tracking demo (Haar+SORT, KCF, mean shift, particle)"
```

---

## Self-Review

**Spec coverage:**
- Contract (grayscale W1/W4, RGB565 W2/W3, no per-frame heap, portable libm, host test each, doc headers) → Global Constraints + per-task format checks. ✓
- W1 SORT (greedy IoU + Kalman, Haar source) → Task 1 + Task 6 Step 4. ✓
- W2 particle likelihood (Bhattacharyya, RGB565/gray, SIR) → Task 3 (+ shared Task 2). ✓
- W3 mean shift (backprojection mode-seek, RGB565/gray) → Task 4 (+ shared Task 2). ✓
- W4 KCF online update (η interpolation, higher precision) → Task 5. ✓
- H7 validation over PyDIPLink, Haar detector, new object-trackers project → Task 6. ✓
- Phase 2 measurement harness → deferred by spec; DWT latency logging seeded in Task 6 Step 5, full harness is a later spec. ✓
- W5 Belief/keypoint-voting → deferred by spec, no task (correct). ✓

**Placeholder scan:** No TBD/TODO. Test bodies are concrete; implementation steps name exact fields, formulas (bin shifts, Bhattacharyya, η interpolation), and reference files. ✓

**Type consistency:** `CvTracker`/`CvTrack`/`cv_track_step`/`cv_track_iou`, `cv_hist_build`/`cv_hist_bhattacharyya`/`CV_HIST_*`, `cv_particle_init_hist`/`hist_nbins`/`template_hist`, `CvMeanShiftState`/`cv_meanshift_init`/`cv_meanshift_update`, `learn_rate` — names match across the tasks that consume them. ✓
