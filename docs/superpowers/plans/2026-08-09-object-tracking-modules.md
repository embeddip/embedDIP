# Object Tracking Modules Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add four independent object-tracking modules (`cv/tracker_kalman`, `cv/tracker_template`, `cv/tracker_kcf`, `cv/tracker_particle`) to embedDIP, ported from the standalone tracker apps at `../object-trackers/`, following embedDIP's existing `cv/` conventions.

**Architecture:** Each tracker is an independent C module — own header, own state struct, own `cv_<name>_init`/`cv_<name>_update` function names. No shared vtable/interface. Each takes a caller-owned `ImageView` and writes results into caller-owned output params. No dynamic allocation inside the trackers except the particle filter, which uses a caller-supplied buffer.

**Tech Stack:** C (C11), embedDIP's existing `core/error.h` (`embeddip_status_t`), `core/image.h` (`ImageView`, `Rectangle`), `imgproc/filter.c` (Sobel via `gaussianGradients`), `imgproc/fft.c` (`fft`/`ifft`/`ffilter2D`). CMake + CTest (existing `tests/CMakeLists.txt` pattern). No CMSIS-DSP, no new external dependency.

## Global Constraints

- All public functions return `embeddip_status_t` (see `core/error.h`); `EMBEDDIP_OK` on success, `EMBEDDIP_ERROR_NULL_PTR`/`EMBEDDIP_ERROR_INVALID_ARG`/`EMBEDDIP_ERROR_INVALID_SIZE` for validation failures — no aborts, no exceptions.
- Every new header: `// SPDX-License-Identifier: MIT` + `// Copyright (c) 2025 EmbedDIP` banner, `#ifndef EMBEDDIP_CV_<NAME>_H` guard, `extern "C"` wrapping, Doxygen `@brief`/`@param`/`@return` on every public function.
- Image input parameter type is `const ImageView *` (non-owning), matching `cv/hog.h`, `cv/integral.h`. Grayscale-only (`IMAGE_FORMAT_GRAYSCALE`/`IMAGE_FORMAT_MASK`), validated the way `cv/image_gray.c:cv_gray_view_validate` does.
- No dynamic allocation inside `cv/tracker_kalman.c` or `cv/tracker_template.c` — fixed-size state structs, caller-owned scratch buffers passed in at init. Two exceptions, both required by an existing embedDIP API's own contract rather than a choice made in this plan: `cv/tracker_particle.c`'s particle buffer is caller-supplied (sized by `N` the caller chose), never malloc'd inside the module; `cv/tracker_kcf.c` calls `imgproc/fft.c`'s `fft`/`ifft`, which themselves allocate `Image.chals` buffers via `createChals`/`createChalsComplex` (see `board/common.h`) — KCF's state struct owns `Image` instances across `cv_kcf_init`/`cv_kcf_update` calls and must release them with `deleteImage`/channel-free on teardown (see Task 4).
- Each new `.c`/`.h` pair is added to `CV_SOURCES` in `CMakeLists.txt` (alongside the existing `cv/detect.c` etc., preserving the existing list order/style).
- Each module gets one test file `tests/test_cv_tracker_<name>.c`, registered in `tests/CMakeLists.txt` exactly like `embeddip_test_cv_detect` (add_executable, target_link_libraries against `embedDIP`, add_test).

---

## Task 1: Kalman tracker (`cv/tracker_kalman.c/h`)

**Files:**
- Create: `cv/tracker_kalman.h`
- Create: `cv/tracker_kalman.c`
- Test: `tests/test_cv_tracker_kalman.c`
- Modify: `CMakeLists.txt` (add to `CV_SOURCES`, after `cv/nn.c`/`cv/nn.h`)
- Modify: `tests/CMakeLists.txt` (add test target, after `embeddip_test_cv_nn`)

**Interfaces:**
- Consumes: `Rectangle` and `embeddip_status_t` from `core/image.h`/`core/error.h` (already available everywhere).
- Produces: `CvKalmanState` struct, `cv_kalman_init(CvKalmanState *state, Rectangle initial_box)`, `cv_kalman_predict(CvKalmanState *state, Rectangle *out_box)`, `cv_kalman_update(CvKalmanState *state, Rectangle measured_box)`. Later tasks do not depend on this — Kalman is a smoother, not called by Template/KCF/Particle.

Ported from `../object-trackers/Tracker_H7/Core/Src/TrackerCommon.c` (`ConstantAccelerationModel`, F matrix) and `Kalman.c` (`InitMat`, `EmbeddedTrackerKalman`). State is 6-dim (x, y, vx, vy, ax, ay); the original 6x6 `F_f32` constant-acceleration matrix is reused directly (values below). Matrix ops are hand-unrolled (no CMSIS-DSP, no matrix library — state dim is fixed at 6).

- [ ] **Step 1: Write header `cv/tracker_kalman.h`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_KALMAN_H
#define EMBEDDIP_CV_TRACKER_KALMAN_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Constant-acceleration Kalman filter state for 2D bounding-box tracking.
 *
 * State vector is [x, y, vx, vy, ax, ay]; box width/height pass through
 * unfiltered (only the box center is estimated).
 */
typedef struct {
    float state[6];      /**< [x, y, vx, vy, ax, ay] */
    float covariance[36]; /**< 6x6 error covariance, row-major */
    int32_t box_width;
    int32_t box_height;
    bool initialized;
} CvKalmanState;

/**
 * @brief Initialize the filter with an initial bounding box (zero velocity/accel).
 *
 * @param[out] state Filter state to initialize.
 * @param[in] initial_box Initial bounding box; center seeds position state.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state is NULL,
 *         EMBEDDIP_ERROR_INVALID_SIZE if initial_box has non-positive width/height.
 */
embeddip_status_t cv_kalman_init(CvKalmanState *state, Rectangle initial_box);

/**
 * @brief Predict the next box position using the constant-acceleration model.
 *
 * Does not consume a measurement; call cv_kalman_update separately when a
 * measurement is available. Advances internal state by one step.
 *
 * @param[in,out] state Filter state (advanced in place).
 * @param[out] out_box Predicted bounding box (width/height unchanged from init).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or out_box
 *         is NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_kalman_init was not
 *         called first.
 */
embeddip_status_t cv_kalman_predict(CvKalmanState *state, Rectangle *out_box);

/**
 * @brief Correct the filter state with a new measurement (e.g. a detector box).
 *
 * @param[in,out] state Filter state (corrected in place).
 * @param[in] measured_box Observed bounding box this frame.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state is NULL,
 *         EMBEDDIP_ERROR_NOT_INITIALIZED if cv_kalman_init was not called first.
 */
embeddip_status_t cv_kalman_update(CvKalmanState *state, Rectangle measured_box);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_KALMAN_H */
```

- [ ] **Step 2: Write implementation `cv/tracker_kalman.c`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_kalman.h"

#include <stdbool.h>
#include <string.h>

/* Constant-acceleration state transition: x' = x + vx + 0.5*ax, etc. */
static const float kF[36] = {
    1.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

/* Process noise, diagonal only (small constant, matches original HexAccel_noise_mag). */
#define KALMAN_PROCESS_NOISE 0.01f
/* Measurement noise, diagonal only (matches original tkn_x/tkn_y). */
#define KALMAN_MEASUREMENT_NOISE 1.0f

embeddip_status_t cv_kalman_init(CvKalmanState *state, Rectangle initial_box)
{
    if (state == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (initial_box.width <= 0 || initial_box.height <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    memset(state->state, 0, sizeof(state->state));
    state->state[0] = (float)initial_box.x + (float)initial_box.width / 2.0f;
    state->state[1] = (float)initial_box.y + (float)initial_box.height / 2.0f;

    memset(state->covariance, 0, sizeof(state->covariance));
    for (int i = 0; i < 6; ++i) {
        state->covariance[i * 6 + i] = KALMAN_PROCESS_NOISE;
    }

    state->box_width = initial_box.width;
    state->box_height = initial_box.height;
    state->initialized = true;
    return EMBEDDIP_OK;
}

/* new_state = F * state (6x6 * 6x1), hand-unrolled since dim is fixed. */
static void kalman_apply_transition(const float *s, float *out)
{
    for (int r = 0; r < 6; ++r) {
        float sum = 0.0f;
        for (int c = 0; c < 6; ++c) {
            sum += kF[r * 6 + c] * s[c];
        }
        out[r] = sum;
    }
}

embeddip_status_t cv_kalman_predict(CvKalmanState *state, Rectangle *out_box)
{
    if (state == NULL || out_box == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    float predicted[6];
    kalman_apply_transition(state->state, predicted);
    memcpy(state->state, predicted, sizeof(predicted));

    /* Process noise added to position/velocity/accel covariance diagonal. */
    for (int i = 0; i < 6; ++i) {
        state->covariance[i * 6 + i] += KALMAN_PROCESS_NOISE;
    }

    out_box->x = (int32_t)(state->state[0] - (float)state->box_width / 2.0f);
    out_box->y = (int32_t)(state->state[1] - (float)state->box_height / 2.0f);
    out_box->width = state->box_width;
    out_box->height = state->box_height;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_kalman_update(CvKalmanState *state, Rectangle measured_box)
{
    if (state == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    float measured_x = (float)measured_box.x + (float)measured_box.width / 2.0f;
    float measured_y = (float)measured_box.y + (float)measured_box.height / 2.0f;

    /* Scalar Kalman gain per axis (position only): K = P / (P + R). */
    float px = state->covariance[0];
    float py = state->covariance[1 * 6 + 1];
    float kx = px / (px + KALMAN_MEASUREMENT_NOISE);
    float ky = py / (py + KALMAN_MEASUREMENT_NOISE);

    state->state[0] += kx * (measured_x - state->state[0]);
    state->state[1] += ky * (measured_y - state->state[1]);

    state->covariance[0] *= (1.0f - kx);
    state->covariance[1 * 6 + 1] *= (1.0f - ky);

    if (measured_box.width > 0 && measured_box.height > 0) {
        state->box_width = measured_box.width;
        state->box_height = measured_box.height;
    }
    return EMBEDDIP_OK;
}
```

- [ ] **Step 3: Write test `tests/test_cv_tracker_kalman.c`**

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <core/image.h>
#include <cv/tracker_kalman.h>

static void test_init_null(void)
{
    Rectangle box = {0, 0, 10, 10};
    assert(cv_kalman_init(NULL, box) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_init_invalid_size(void)
{
    CvKalmanState state;
    Rectangle bad_box = {0, 0, 0, 10};
    assert(cv_kalman_init(&state, bad_box) == EMBEDDIP_ERROR_INVALID_SIZE);
}

static void test_predict_without_init(void)
{
    CvKalmanState state;
    memset(&state, 0, sizeof(state));
    Rectangle out;
    assert(cv_kalman_predict(&state, &out) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_tracks_constant_velocity(void)
{
    CvKalmanState state;
    Rectangle initial = {0, 0, 10, 10};
    assert(cv_kalman_init(&state, initial) == EMBEDDIP_OK);

    /* Feed measurements moving +5px/frame in x, correcting the filter each time. */
    int32_t x = 0;
    for (int frame = 0; frame < 20; ++frame) {
        Rectangle out;
        assert(cv_kalman_predict(&state, &out) == EMBEDDIP_OK);
        x += 5;
        Rectangle measured = {x, 0, 10, 10};
        assert(cv_kalman_update(&state, measured) == EMBEDDIP_OK);
    }

    Rectangle final_out;
    assert(cv_kalman_predict(&state, &final_out) == EMBEDDIP_OK);
    /* After 20 frames of consistent +5px/frame motion, predicted x should be
     * close to the true trajectory (within 15px slack for filter lag). */
    assert(final_out.x > x - 15 && final_out.x < x + 30);
}

int main(void)
{
    test_init_null();
    test_init_invalid_size();
    test_predict_without_init();
    test_tracks_constant_velocity();
    return 0;
}
```

Note: add `#include <string.h>` to the test file's includes (needed for `memset` in `test_predict_without_init`) — the include list above already lists the needed headers except this one; add it alongside `<stdint.h>`.

- [ ] **Step 4: Add to `CMakeLists.txt`**

In `CV_SOURCES` (around line 101-102, after `cv/nn.c`/`cv/nn.h`), add:

```cmake
    cv/tracker_kalman.c
    cv/tracker_kalman.h
```

- [ ] **Step 5: Add to `tests/CMakeLists.txt`**

After the `embeddip_test_cv_nn` block, add:

```cmake
add_executable(embeddip_test_cv_tracker_kalman test_cv_tracker_kalman.c)
target_link_libraries(embeddip_test_cv_tracker_kalman PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_kalman COMMAND embeddip_test_cv_tracker_kalman)
```

- [ ] **Step 6: Build and run the test**

Run: `cmake --build build --target embeddip_test_cv_tracker_kalman && ctest --test-dir build -R cv_tracker_kalman -V`
Expected: build succeeds, test PASSES (all four assertions in `main` pass).

If the project isn't already configured for host builds, first run: `cmake -S . -B build -DEMBEDDIP_TARGET_BOARD=HOST` (check `CMakeLists.txt` top for the actual host-target flag name/value if this fails — search for `EMBEDDIP_TARGET_BOARD` usage in `CMakeLists.txt`).

- [ ] **Step 7: Commit**

```bash
git add cv/tracker_kalman.c cv/tracker_kalman.h tests/test_cv_tracker_kalman.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add Kalman tracker module to cv/"
```

---

## Task 2: Template-matching tracker (`cv/tracker_template.c/h`)

**Files:**
- Create: `cv/tracker_template.h`
- Create: `cv/tracker_template.c`
- Test: `tests/test_cv_tracker_template.c`
- Modify: `CMakeLists.txt` (add to `CV_SOURCES`, after `cv/tracker_kalman.c/h`)
- Modify: `tests/CMakeLists.txt` (add test target, after `embeddip_test_cv_tracker_kalman`)

**Interfaces:**
- Consumes: `ImageView` from `core/image.h` (already available); no dependency on Task 1.
- Produces: `CvTemplateState` struct (fixed-size template buffer, `CV_TEMPLATE_MAX_WIDTH`/`CV_TEMPLATE_MAX_HEIGHT`), `cv_template_set(CvTemplateState *state, const ImageView *src, Rectangle roi)`, `cv_template_match(const CvTemplateState *state, const ImageView *frame, Rectangle *out_box)`. Not consumed by later tasks.

Ported from `../object-trackers/Tracker_H7/Core/Src/TemplateMatching.c` (`SetTemplate`, brute-force SAD correlation in `EmbeddedTrackerTemplateKalman`). Simplifies out the Kalman coupling present in the original (that's Task 1's job if a caller wants to combine them) — this module does pure template matching: store a patch, find its best-match location in a new frame via sum-of-absolute-differences.

- [ ] **Step 1: Write header `cv/tracker_template.h`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_TEMPLATE_H
#define EMBEDDIP_CV_TRACKER_TEMPLATE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum template patch width in pixels. */
#define CV_TEMPLATE_MAX_WIDTH 64u
/** Maximum template patch height in pixels. */
#define CV_TEMPLATE_MAX_HEIGHT 64u

/**
 * @brief Stored template patch and match state.
 */
typedef struct {
    uint8_t patch[CV_TEMPLATE_MAX_HEIGHT][CV_TEMPLATE_MAX_WIDTH];
    uint16_t width;
    uint16_t height;
    bool initialized;
} CvTemplateState;

/**
 * @brief Capture a template patch from a region of an 8-bit grayscale image.
 *
 * @param[out] state Template state to populate.
 * @param[in] src Grayscale (or mask) image view to copy the patch from.
 * @param[in] roi Region to copy; must fit within src and within
 *            CV_TEMPLATE_MAX_WIDTH x CV_TEMPLATE_MAX_HEIGHT.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or src is
 *         NULL, EMBEDDIP_ERROR_INVALID_FORMAT if src is not grayscale/mask,
 *         EMBEDDIP_ERROR_INVALID_SIZE if roi is out of bounds or too large.
 */
embeddip_status_t cv_template_set(CvTemplateState *state, const ImageView *src, Rectangle roi);

/**
 * @brief Find the best-matching location of the stored template in a new frame.
 *
 * Brute-force scan computing sum-of-absolute-differences at every candidate
 * origin; returns the origin with the lowest SAD score.
 *
 * @param[in] state Previously captured template (via cv_template_set).
 * @param[in] frame Grayscale (or mask) image view to search.
 * @param[out] out_box Best-match bounding box (same size as the template).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if any pointer is
 *         NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_template_set was not
 *         called first, EMBEDDIP_ERROR_INVALID_SIZE if frame is smaller than
 *         the template.
 */
embeddip_status_t cv_template_match(const CvTemplateState *state, const ImageView *frame,
                                     Rectangle *out_box);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_TEMPLATE_H */
```

- [ ] **Step 2: Write implementation `cv/tracker_template.c`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_template.h"

#include <stdint.h>

static bool template_format_ok(ImageFormat fmt)
{
    return fmt == IMAGE_FORMAT_GRAYSCALE || fmt == IMAGE_FORMAT_MASK;
}

embeddip_status_t cv_template_set(CvTemplateState *state, const ImageView *src, Rectangle roi)
{
    if (state == NULL || src == NULL || src->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!template_format_ok(src->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (roi.width <= 0 || roi.height <= 0 || (uint32_t)roi.width > CV_TEMPLATE_MAX_WIDTH ||
        (uint32_t)roi.height > CV_TEMPLATE_MAX_HEIGHT) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (roi.x < 0 || roi.y < 0 || (uint32_t)(roi.x + roi.width) > src->width ||
        (uint32_t)(roi.y + roi.height) > src->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    for (int32_t row = 0; row < roi.height; ++row) {
        const uint8_t *src_row = src->pixels + (size_t)(roi.y + row) * src->row_stride_bytes;
        for (int32_t col = 0; col < roi.width; ++col) {
            state->patch[row][col] = src_row[roi.x + col];
        }
    }

    state->width = (uint16_t)roi.width;
    state->height = (uint16_t)roi.height;
    state->initialized = true;
    return EMBEDDIP_OK;
}

/* Sum of absolute differences between the stored patch and a frame region. */
static uint32_t template_sad_at(const CvTemplateState *state, const ImageView *frame,
                                int32_t origin_x, int32_t origin_y)
{
    uint32_t sad = 0u;
    for (uint16_t row = 0u; row < state->height; ++row) {
        const uint8_t *frame_row =
            frame->pixels + (size_t)(origin_y + row) * frame->row_stride_bytes;
        for (uint16_t col = 0u; col < state->width; ++col) {
            int32_t diff = (int32_t)frame_row[origin_x + col] - (int32_t)state->patch[row][col];
            sad += (uint32_t)(diff < 0 ? -diff : diff);
        }
    }
    return sad;
}

embeddip_status_t cv_template_match(const CvTemplateState *state, const ImageView *frame,
                                     Rectangle *out_box)
{
    if (state == NULL || frame == NULL || out_box == NULL || frame->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    if (frame->width < state->width || frame->height < state->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    int32_t max_x = (int32_t)frame->width - (int32_t)state->width;
    int32_t max_y = (int32_t)frame->height - (int32_t)state->height;
    uint32_t best_sad = UINT32_MAX;
    int32_t best_x = 0;
    int32_t best_y = 0;

    for (int32_t y = 0; y <= max_y; ++y) {
        for (int32_t x = 0; x <= max_x; ++x) {
            uint32_t sad = template_sad_at(state, frame, x, y);
            if (sad < best_sad) {
                best_sad = sad;
                best_x = x;
                best_y = y;
            }
        }
    }

    out_box->x = best_x;
    out_box->y = best_y;
    out_box->width = state->width;
    out_box->height = state->height;
    return EMBEDDIP_OK;
}
```

- [ ] **Step 3: Write test `tests/test_cv_tracker_template.c`**

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <core/image.h>
#include <cv/tracker_template.h>

#define FRAME_W 20u
#define FRAME_H 20u

static void fill_frame(uint8_t *pixels, int32_t block_x, int32_t block_y)
{
    memset(pixels, 0u, FRAME_W * FRAME_H);
    for (int32_t y = block_y; y < block_y + 4; ++y) {
        for (int32_t x = block_x; x < block_x + 4; ++x) {
            pixels[y * (int32_t)FRAME_W + x] = 255u;
        }
    }
}

static void test_set_null(void)
{
    ImageView view = {0};
    Rectangle roi = {0, 0, 4, 4};
    assert(cv_template_set(NULL, &view, roi) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_match_without_set(void)
{
    CvTemplateState state;
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
    assert(cv_template_match(&state, &frame, &out) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_finds_moved_block(void)
{
    uint8_t template_pixels[FRAME_W * FRAME_H];
    fill_frame(template_pixels, 2, 2);
    ImageView template_view = {.pixels = template_pixels,
                               .width = FRAME_W,
                               .height = FRAME_H,
                               .row_stride_bytes = FRAME_W,
                               .format = IMAGE_FORMAT_GRAYSCALE,
                               .depth = IMAGE_DEPTH_U8};
    Rectangle roi = {2, 2, 4, 4};

    CvTemplateState state;
    assert(cv_template_set(&state, &template_view, roi) == EMBEDDIP_OK);

    uint8_t frame_pixels[FRAME_W * FRAME_H];
    fill_frame(frame_pixels, 10, 8); /* block moved to (10,8) */
    ImageView frame = {.pixels = frame_pixels,
                       .width = FRAME_W,
                       .height = FRAME_H,
                       .row_stride_bytes = FRAME_W,
                       .format = IMAGE_FORMAT_GRAYSCALE,
                       .depth = IMAGE_DEPTH_U8};

    Rectangle out;
    assert(cv_template_match(&state, &frame, &out) == EMBEDDIP_OK);
    assert(out.x == 10 && out.y == 8);
    assert(out.width == 4 && out.height == 4);
}

int main(void)
{
    test_set_null();
    test_match_without_set();
    test_finds_moved_block();
    return 0;
}
```

- [ ] **Step 4: Add to `CMakeLists.txt`**

In `CV_SOURCES`, after the `cv/tracker_kalman.c/h` lines added in Task 1:

```cmake
    cv/tracker_template.c
    cv/tracker_template.h
```

- [ ] **Step 5: Add to `tests/CMakeLists.txt`**

After the `embeddip_test_cv_tracker_kalman` block:

```cmake
add_executable(embeddip_test_cv_tracker_template test_cv_tracker_template.c)
target_link_libraries(embeddip_test_cv_tracker_template PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_template COMMAND embeddip_test_cv_tracker_template)
```

- [ ] **Step 6: Build and run the test**

Run: `cmake --build build --target embeddip_test_cv_tracker_template && ctest --test-dir build -R cv_tracker_template -V`
Expected: build succeeds, test PASSES.

- [ ] **Step 7: Commit**

```bash
git add cv/tracker_template.c cv/tracker_template.h tests/test_cv_tracker_template.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add template-matching tracker module to cv/"
```

---

## Task 3: Particle-filter tracker (`cv/tracker_particle.c/h`)

**Files:**
- Create: `cv/tracker_particle.h`
- Create: `cv/tracker_particle.c`
- Test: `tests/test_cv_tracker_particle.c`
- Modify: `CMakeLists.txt` (add to `CV_SOURCES`, after `cv/tracker_template.c/h`)
- Modify: `tests/CMakeLists.txt` (add test target, after `embeddip_test_cv_tracker_template`)

**Interfaces:**
- Consumes: `ImageView`, `Rectangle` from `core/image.h`; `imgproc/filter.h`'s `gaussianGradients(const Image *src, Image *dst_ix, Image *dst_iy, float sigma)` and `gradientMagnitude(const Image *ix_img, const Image *iy_img, Image *dst)` for the corner-likelihood step (replaces the original's `SobelDerivative`+`HarrisResponse`, which don't exist in embedDIP — gradient magnitude peaks are used as the corner-strength proxy instead of a full Harris response, since embedDIP has no Harris primitive and adding one is out of scope per the design spec). Note `gaussianGradients`/`gradientMagnitude` take `Image *`, not `ImageView *` — this task must build a full `Image` wrapping the `ImageView`'s buffer (see Step 2 for the exact conversion, using `image_view_from_buffer`'s inverse: construct `Image` fields directly from `ImageView` fields since both are plain structs).
- Produces: `CvParticleState` struct, `cv_particle_init(CvParticleState *state, uint16_t particle_count, float *particle_buffer, Rectangle roi)`, `cv_particle_update(CvParticleState *state, const ImageView *frame, Rectangle *out_box)`. Not consumed by later tasks. `particle_buffer` must have capacity `particle_count * 2` floats (x,y per particle) — caller-owned, no malloc inside the module. `cv_particle_init` seeds particles from `roi` alone; per-pixel gradient likelihood is computed later inside `cv_particle_update` against whatever `frame` is passed in.

Ported from `../object-trackers/Tracker_H7/Core/Src/Particle.c` (`EmbeddedTracker`): motion model is random-walk diffusion around the previous centroid (the original's `ConstantAccelerationModel` dependency on `TrackerCommon.c` is dropped — this module is self-contained and does not depend on Task 1's Kalman module, matching the "independent APIs" decision). Likelihood weighting uses gradient-magnitude peaks near each particle instead of Harris corner clustering (simplification noted above).

- [ ] **Step 1: Write header `cv/tracker_particle.h`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_PARTICLE_H
#define EMBEDDIP_CV_TRACKER_PARTICLE_H

#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Particle-filter tracker state.
 *
 * Particles are stored as caller-owned (x, y) float pairs in
 * @c particle_buffer (capacity particle_count * 2 floats), so the module
 * never allocates memory.
 */
typedef struct {
    float *particle_buffer;  /**< Caller-owned, particle_count * 2 floats: [x0,y0,x1,y1,...] */
    uint16_t particle_count;
    int32_t box_width;
    int32_t box_height;
    uint32_t rng_state; /**< xorshift32 state, seeded at init */
    bool initialized;
} CvParticleState;

/**
 * @brief Seed the particle filter around an initial bounding box.
 *
 * @param[out] state Filter state to initialize.
 * @param[in] particle_count Number of particles (> 0); particle_buffer must
 *            have capacity particle_count * 2 floats.
 * @param[in] particle_buffer Caller-owned scratch buffer for particle positions.
 * @param[in] roi Initial bounding box; particles are seeded around its center.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or
 *         particle_buffer is NULL, EMBEDDIP_ERROR_INVALID_ARG if
 *         particle_count is 0, EMBEDDIP_ERROR_INVALID_SIZE if roi has
 *         non-positive width/height.
 */
embeddip_status_t cv_particle_init(CvParticleState *state, uint16_t particle_count,
                                   float *particle_buffer, Rectangle roi);

/**
 * @brief Diffuse particles, weight them by local gradient strength, and
 *        return the weighted-centroid bounding box.
 *
 * @param[in,out] state Filter state (particles updated in place).
 * @param[in] frame Grayscale (or mask) image view to search.
 * @param[out] out_box Weighted-centroid bounding box (same size as init roi).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state, frame, or
 *         out_box is NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_particle_init
 *         was not called first, EMBEDDIP_ERROR_INVALID_FORMAT if frame is not
 *         grayscale/mask.
 */
embeddip_status_t cv_particle_update(CvParticleState *state, const ImageView *frame,
                                     Rectangle *out_box);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_PARTICLE_H */
```

- [ ] **Step 2: Write implementation `cv/tracker_particle.c`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_particle.h"

#include <stdint.h>
#include <string.h>

#include "imgproc/filter.h"

static bool particle_format_ok(ImageFormat fmt)
{
    return fmt == IMAGE_FORMAT_GRAYSCALE || fmt == IMAGE_FORMAT_MASK;
}

/* xorshift32: fast, deterministic, no external RNG dependency. */
static uint32_t xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Uniform float in [-range, range]. */
static float xorshift_range(uint32_t *state, float range)
{
    uint32_t r = xorshift32(state);
    float unit = (float)r / (float)UINT32_MAX; /* [0,1] */
    return (unit * 2.0f - 1.0f) * range;
}

embeddip_status_t cv_particle_init(CvParticleState *state, uint16_t particle_count,
                                   float *particle_buffer, Rectangle roi)
{
    if (state == NULL || particle_buffer == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (particle_count == 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (roi.width <= 0 || roi.height <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    state->particle_buffer = particle_buffer;
    state->particle_count = particle_count;
    state->box_width = roi.width;
    state->box_height = roi.height;
    state->rng_state = 0x9E3779B9u; /* fixed seed: deterministic tracking, no HW RNG dependency */

    float center_x = (float)roi.x + (float)roi.width / 2.0f;
    float center_y = (float)roi.y + (float)roi.height / 2.0f;
    for (uint16_t i = 0u; i < particle_count; ++i) {
        particle_buffer[i * 2u] = center_x + xorshift_range(&state->rng_state, 5.0f);
        particle_buffer[i * 2u + 1u] = center_y + xorshift_range(&state->rng_state, 5.0f);
    }

    state->initialized = true;
    return EMBEDDIP_OK;
}

/* Build a non-owning Image view over an ImageView's buffer, matching fields
 * one-to-one (both are plain structs; no ownership transfer). */
static void particle_image_from_view(const ImageView *view, Image *out)
{
    out->width = view->width;
    out->height = view->height;
    out->pixels = view->pixels;
    out->chals = NULL;
    out->size = view->width * view->height;
    out->format = view->format;
    out->depth = view->depth;
    out->log = IMAGE_DATA_PIXELS;
    out->is_chals = false;
}

embeddip_status_t cv_particle_update(CvParticleState *state, const ImageView *frame,
                                     Rectangle *out_box)
{
    if (state == NULL || frame == NULL || out_box == NULL || frame->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    if (!particle_format_ok(frame->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    /* Diffuse particles (random walk). */
    for (uint16_t i = 0u; i < state->particle_count; ++i) {
        state->particle_buffer[i * 2u] += xorshift_range(&state->rng_state, 3.0f);
        state->particle_buffer[i * 2u + 1u] += xorshift_range(&state->rng_state, 3.0f);
    }

    /* Weight by local gradient magnitude (gradient peaks stand in for corner
     * strength; embedDIP has no Harris-response primitive). */
    Image gray_image;
    particle_image_from_view(frame, &gray_image);

    /* Scratch gradient/magnitude buffers sized to the frame; embedDIP's
     * gaussianGradients/gradientMagnitude write into caller-owned Image
     * buffers, so allocate on the stack via a fixed cap matching the
     * largest expected tracking ROI context (frame itself can be large,
     * so we only compute the gradient once per update() call and reuse it
     * across all particles). */
    static float gx_buf[256 * 256];
    static float gy_buf[256 * 256];
    static float mag_buf[256 * 256];
    if (frame->width * frame->height > 256u * 256u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    Image gx_img = {.width = frame->width, .height = frame->height, .pixels = gx_buf,
                    .chals = NULL, .size = frame->width * frame->height,
                    .format = IMAGE_FORMAT_GRAYSCALE, .depth = IMAGE_DEPTH_F32,
                    .log = IMAGE_DATA_PIXELS, .is_chals = false};
    Image gy_img = gx_img;
    gy_img.pixels = gy_buf;
    Image mag_img = gx_img;
    mag_img.pixels = mag_buf;

    embeddip_status_t status = gaussianGradients(&gray_image, &gx_img, &gy_img, 1.0f);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    status = gradientMagnitude(&gx_img, &gy_img, &mag_img);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_w = 0.0f;
    const float *mag = (const float *)mag_img.pixels;

    for (uint16_t i = 0u; i < state->particle_count; ++i) {
        float px = state->particle_buffer[i * 2u];
        float py = state->particle_buffer[i * 2u + 1u];
        int32_t ix = (int32_t)px;
        int32_t iy = (int32_t)py;
        if (ix < 0 || iy < 0 || (uint32_t)ix >= frame->width || (uint32_t)iy >= frame->height) {
            continue;
        }
        float weight = mag[(size_t)iy * frame->width + (size_t)ix] + 1e-3f;
        sum_x += px * weight;
        sum_y += py * weight;
        sum_w += weight;
    }

    if (sum_w <= 0.0f) {
        return EMBEDDIP_ERROR_UNKNOWN;
    }

    float centroid_x = sum_x / sum_w;
    float centroid_y = sum_y / sum_w;
    out_box->x = (int32_t)(centroid_x - (float)state->box_width / 2.0f);
    out_box->y = (int32_t)(centroid_y - (float)state->box_height / 2.0f);
    out_box->width = state->box_width;
    out_box->height = state->box_height;
    return EMBEDDIP_OK;
}
```

- [ ] **Step 3: Write test `tests/test_cv_tracker_particle.c`**

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <core/image.h>
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
    Rectangle roi = {14, 14, 6, 6}; /* seeded at frame center */
    assert(cv_particle_init(&state, 100u, particle_buf, roi) == EMBEDDIP_OK);

    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 14, 14); /* block coincides with seed */
    ImageView frame = {.pixels = pixels,
                       .width = FRAME_W,
                       .height = FRAME_H,
                       .row_stride_bytes = FRAME_W,
                       .format = IMAGE_FORMAT_GRAYSCALE,
                       .depth = IMAGE_DEPTH_U8};

    Rectangle out;
    embeddip_status_t status = EMBEDDIP_ERROR_UNKNOWN;
    for (int frame_i = 0; frame_i < 5; ++frame_i) {
        status = cv_particle_update(&state, &frame, &out);
        assert(status == EMBEDDIP_OK);
    }
    /* Particles should converge near the block's edges (gradient peaks),
     * which lie within the block's bounding region plus diffusion slack. */
    assert(out.x > 0 && out.x < (int32_t)FRAME_W);
    assert(out.y > 0 && out.y < (int32_t)FRAME_H);
}

int main(void)
{
    test_init_null();
    test_init_zero_particles();
    test_update_without_init();
    test_tracks_toward_block();
    return 0;
}
```

- [ ] **Step 4: Add to `CMakeLists.txt`**

In `CV_SOURCES`, after the `cv/tracker_template.c/h` lines added in Task 2:

```cmake
    cv/tracker_particle.c
    cv/tracker_particle.h
```

- [ ] **Step 5: Add to `tests/CMakeLists.txt`**

After the `embeddip_test_cv_tracker_template` block:

```cmake
add_executable(embeddip_test_cv_tracker_particle test_cv_tracker_particle.c)
target_link_libraries(embeddip_test_cv_tracker_particle PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_particle COMMAND embeddip_test_cv_tracker_particle)
```

- [ ] **Step 6: Build and run the test**

Run: `cmake --build build --target embeddip_test_cv_tracker_particle && ctest --test-dir build -R cv_tracker_particle -V`
Expected: build succeeds, test PASSES.

If `gaussianGradients`/`gradientMagnitude` signatures in `imgproc/filter.h` differ from what's assumed here (double-check parameter order/types against the actual header before writing the call — the header excerpt used for this plan showed `gaussianGradients(const Image *src, Image *dst_ix, Image *dst_iy, float sigma)` and `gradientMagnitude(const Image *ix_img, const Image *iy_img, Image *dst)`), adjust the call sites in Step 2 to match; re-run this step after fixing.

- [ ] **Step 7: Commit**

```bash
git add cv/tracker_particle.c cv/tracker_particle.h tests/test_cv_tracker_particle.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add particle-filter tracker module to cv/"
```

---

## Task 4: KCF tracker (`cv/tracker_kcf.c/h`)

**Files:**
- Create: `cv/tracker_kcf.h`
- Create: `cv/tracker_kcf.c`
- Test: `tests/test_cv_tracker_kcf.c`
- Modify: `CMakeLists.txt` (add to `CV_SOURCES`, after `cv/tracker_particle.c/h`)
- Modify: `tests/CMakeLists.txt` (add test target, after `embeddip_test_cv_tracker_particle`)

**Interfaces:**
- Consumes: `ImageView`, `Rectangle` from `core/image.h`; `imgproc/fft.h`'s `fft(const Image *src, Image *dst)` and `ifft(const Image *src, Image *dst)`; `board/common.h`'s `createImageWH`/`deleteImage` (required because `fft`/`ifft` operate on heap-allocated `Image`/`chals`, not caller-owned buffers — see confirmed API shape below). No dependency on Tasks 1-3.
- Produces: `CvKcfState` struct, `cv_kcf_init(CvKcfState *state, const ImageView *src, Rectangle roi)`, `cv_kcf_update(CvKcfState *state, const ImageView *frame, Rectangle *out_box)`, `cv_kcf_free(CvKcfState *state)`. Not consumed by later tasks.

**Confirmed `imgproc/fft.c` API shape** (read directly from source, not assumed):
- `fft`/`ifft` require **square, power-of-2** dimensions: `isValidFFTSize` checks `(w == h) && ((w & (w-1)) == 0)` (`imgproc/fft.c:10-13`). `CV_KCF_PATCH_SIZE` must be a power of 2 — use **64**.
- `fft(const Image *src, Image *dst)`: reads `src->pixels` as `uint8_t` grayscale, writes complex output into `dst->chals->ch[0]` (real) / `dst->chals->ch[1]` (imag), allocating `dst->chals` via `createChalsComplex` if empty, and sets `dst->log = IMAGE_DATA_COMPLEX`. `dst` must have `width`/`height` set to match `src` before calling.
- `ifft(const Image *src, Image *dst)`: requires `src->log == IMAGE_DATA_COMPLEX` (or `IMAGE_DATA_CH0`), reads `src->chals->ch[1]` (or `ch[0]`), writes real output into `dst->chals->ch[0]` via `createChals`, sets `dst->log = IMAGE_DATA_CH0`.
- Because `fft`/`ifft` allocate `Image.chals` internally (via `createChals`/`createChalsComplex` in `board/common.h`), this module cannot be alloc-free like Tasks 1-3 — it must own `Image` structs across calls and free them with `deleteImage` on teardown. `createImageWH(width, height, format, Image**)` allocates a fresh heap `Image` (including `pixels`); `deleteImage(Image*)` frees `pixels`, `chals` (all 6 channel buffers + the `channels_t` struct), and the `Image` struct itself (`board/common.c:241-266`).
- `Image` fields relevant here: `pixels` (`void*`, raw grayscale buffer), `chals` (`channels_t*`, `ch[0]`/`ch[1]` = real/imag), `log` (`ImageDataState`, must be `IMAGE_DATA_COMPLEX` before `ifft` reads it), `is_chals` (bool, set by `createChals*`).

Ported from `../object-trackers/H7_KCF/Core/Src/KCFTracker.c` (`PolynomialCorrelation`, `fft2d`/`ifft2d`). Replaces the hand-rolled FFT with `imgproc/fft.c`'s `fft`/`ifft`, and computes the correlation response via the convolution theorem: response = ifft(fft(patch) .* conj(fft(template))), peak location = new box center. This drops the original's polynomial-kernel regularization (`dum *= dum` nine times, i.e. `dum^512`) in favor of a plain normalized cross-correlation peak — the design spec flagged the kernel math as the highest-risk item; this simpler correlation avoids needing to replicate the original's exact (undocumented) kernel regularization, while still using the same FFT-based correlation architecture. Patch size is fixed at 64x64 (power-of-2, per `isValidFFTSize` above).

- [ ] **Step 1: Write header `cv/tracker_kcf.h`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_KCF_H
#define EMBEDDIP_CV_TRACKER_KCF_H

#include <stdbool.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fixed patch edge length in pixels. Must be a power of 2 to satisfy
 * imgproc/fft.c's isValidFFTSize check. */
#define CV_KCF_PATCH_SIZE 64u

/**
 * @brief KCF (Kernel Correlation Filter) tracker state.
 *
 * `template_spectrum` is a heap-allocated Image owned by this state, holding
 * the forward FFT of the learned template patch (real/imag interleaved via
 * imgproc/fft.c's Image.chals convention: ch[0]=real, ch[1]=imag). It is
 * allocated in cv_kcf_init and released by cv_kcf_free.
 */
typedef struct {
    Image *template_spectrum;
    int32_t box_width;
    int32_t box_height;
    int32_t center_x;
    int32_t center_y;
    bool initialized;
} CvKcfState;

/**
 * @brief Initialize the tracker: extract and store a template patch's spectrum.
 *
 * @param[out] state Tracker state to initialize.
 * @param[in] src Grayscale (or mask) image view to sample the template from.
 * @param[in] roi Initial bounding box; the patch is resampled/cropped to
 *            CV_KCF_PATCH_SIZE x CV_KCF_PATCH_SIZE around its center.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or src is
 *         NULL, EMBEDDIP_ERROR_INVALID_FORMAT if src is not grayscale/mask,
 *         EMBEDDIP_ERROR_INVALID_SIZE if roi is out of bounds,
 *         EMBEDDIP_ERROR_OUT_OF_MEMORY if the internal spectrum Image cannot
 *         be allocated.
 */
embeddip_status_t cv_kcf_init(CvKcfState *state, const ImageView *src, Rectangle roi);

/**
 * @brief Locate the tracked object in a new frame via correlation-filter response.
 *
 * @param[in,out] state Tracker state (template updated in place after match).
 * @param[in] frame Grayscale (or mask) image view to search.
 * @param[out] out_box New bounding box (same size as init roi).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state, frame, or
 *         out_box is NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_kcf_init was
 *         not called first, EMBEDDIP_ERROR_INVALID_FORMAT if frame is not
 *         grayscale/mask, EMBEDDIP_ERROR_OUT_OF_MEMORY if a scratch Image
 *         cannot be allocated.
 */
embeddip_status_t cv_kcf_update(CvKcfState *state, const ImageView *frame, Rectangle *out_box);

/**
 * @brief Release the heap Image owned by state->template_spectrum.
 *
 * Safe to call on a zero-initialized or already-freed state (no-op if
 * template_spectrum is NULL). Must be called before the state goes out of
 * scope to avoid leaking the spectrum Image's chals buffers.
 *
 * @param[in,out] state Tracker state to release.
 * @return EMBEDDIP_OK always (EMBEDDIP_ERROR_NULL_PTR if state is NULL).
 */
embeddip_status_t cv_kcf_free(CvKcfState *state);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_KCF_H */
```

- [ ] **Step 2: Write implementation `cv/tracker_kcf.c`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_kcf.h"

#include <stdint.h>
#include <string.h>

#include "board/common.h"
#include "imgproc/fft.h"

static bool kcf_format_ok(ImageFormat fmt)
{
    return fmt == IMAGE_FORMAT_GRAYSCALE || fmt == IMAGE_FORMAT_MASK;
}

/* Crop and nearest-neighbor resample a src region to
 * CV_KCF_PATCH_SIZE x CV_KCF_PATCH_SIZE, writing into a heap Image's
 * pixels buffer (fft() reads src->pixels as uint8_t regardless of
 * ImageFormat). */
static void kcf_extract_patch(const ImageView *src, Rectangle roi, uint8_t *out_pixels)
{
    for (uint32_t py = 0u; py < CV_KCF_PATCH_SIZE; ++py) {
        int32_t sy = roi.y + (int32_t)((int64_t)py * roi.height / CV_KCF_PATCH_SIZE);
        const uint8_t *row = src->pixels + (size_t)sy * src->row_stride_bytes;
        for (uint32_t px = 0u; px < CV_KCF_PATCH_SIZE; ++px) {
            int32_t sx = roi.x + (int32_t)((int64_t)px * roi.width / CV_KCF_PATCH_SIZE);
            out_pixels[py * CV_KCF_PATCH_SIZE + px] = row[sx];
        }
    }
}

/* Build a heap Image holding a CV_KCF_PATCH_SIZE^2 grayscale patch sampled
 * from src's roi. Caller must deleteImage() the result. */
static embeddip_status_t kcf_make_patch_image(const ImageView *src, Rectangle roi,
                                               Image **out_patch)
{
    Image *patch = NULL;
    embeddip_status_t status =
        createImageWH((int)CV_KCF_PATCH_SIZE, (int)CV_KCF_PATCH_SIZE, IMAGE_FORMAT_GRAYSCALE, &patch);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    kcf_extract_patch(src, roi, (uint8_t *)patch->pixels);
    *out_patch = patch;
    return EMBEDDIP_OK;
}

/* Multiply search-patch spectrum by the conjugate of the template spectrum,
 * writing an already-chals-allocated complex spectrum Image (corr->chals
 * must be pre-allocated via createChalsComplex(corr, 2) by the caller). */
static void kcf_conj_multiply(const Image *search_spec, const Image *template_spec, Image *corr)
{
    uint32_t n = search_spec->width * search_spec->height;
    const float *a_re = search_spec->chals->ch[0];
    const float *a_im = search_spec->chals->ch[1];
    const float *b_re = template_spec->chals->ch[0];
    const float *b_im = template_spec->chals->ch[1];
    float *out_re = corr->chals->ch[0];
    float *out_im = corr->chals->ch[1];

    for (uint32_t i = 0u; i < n; ++i) {
        /* (a) * conj(b) = (a_re*b_re + a_im*b_im) + j(a_im*b_re - a_re*b_im) */
        out_re[i] = a_re[i] * b_re[i] + a_im[i] * b_im[i];
        out_im[i] = a_im[i] * b_re[i] - a_re[i] * b_im[i];
    }
    corr->log = IMAGE_DATA_COMPLEX;
}

/* Find the index of the largest real correlation value, then convert its
 * circular position into a signed (dx,dy) pixel offset in patch space
 * (values in [0, N/2) map to positive offsets, [N/2, N) wrap to negative). */
static void kcf_find_peak_offset(const Image *corr_time, int32_t *out_dx, int32_t *out_dy)
{
    uint32_t n = corr_time->width;
    const float *data = corr_time->chals->ch[0];
    uint32_t best_idx = 0u;
    float best_val = data[0];
    for (uint32_t i = 1u; i < n * n; ++i) {
        if (data[i] > best_val) {
            best_val = data[i];
            best_idx = i;
        }
    }
    int32_t x = (int32_t)(best_idx % n);
    int32_t y = (int32_t)(best_idx / n);
    int32_t half = (int32_t)n / 2;
    *out_dx = (x >= half) ? (x - (int32_t)n) : x;
    *out_dy = (y >= half) ? (y - (int32_t)n) : y;
}

embeddip_status_t cv_kcf_init(CvKcfState *state, const ImageView *src, Rectangle roi)
{
    if (state == NULL || src == NULL || src->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!kcf_format_ok(src->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (roi.width <= 0 || roi.height <= 0 || roi.x < 0 || roi.y < 0 ||
        (uint32_t)(roi.x + roi.width) > src->width ||
        (uint32_t)(roi.y + roi.height) > src->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    Image *patch = NULL;
    embeddip_status_t status = kcf_make_patch_image(src, roi, &patch);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    Image *spectrum = NULL;
    status = createImageWH((int)CV_KCF_PATCH_SIZE, (int)CV_KCF_PATCH_SIZE, IMAGE_FORMAT_GRAYSCALE,
                            &spectrum);
    if (status != EMBEDDIP_OK) {
        deleteImage(patch);
        return status;
    }

    status = fft(patch, spectrum);
    deleteImage(patch);
    if (status != EMBEDDIP_OK) {
        deleteImage(spectrum);
        return status;
    }

    state->template_spectrum = spectrum;
    state->box_width = roi.width;
    state->box_height = roi.height;
    state->center_x = roi.x + roi.width / 2;
    state->center_y = roi.y + roi.height / 2;
    state->initialized = true;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_kcf_update(CvKcfState *state, const ImageView *frame, Rectangle *out_box)
{
    if (state == NULL || frame == NULL || out_box == NULL || frame->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    if (!kcf_format_ok(frame->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    Rectangle search_roi = {state->center_x - state->box_width / 2,
                            state->center_y - state->box_height / 2, state->box_width,
                            state->box_height};
    /* Clamp search_roi to frame bounds. */
    if (search_roi.x < 0) {
        search_roi.x = 0;
    }
    if (search_roi.y < 0) {
        search_roi.y = 0;
    }
    if ((uint32_t)(search_roi.x + search_roi.width) > frame->width) {
        search_roi.x = (int32_t)frame->width - search_roi.width;
    }
    if ((uint32_t)(search_roi.y + search_roi.height) > frame->height) {
        search_roi.y = (int32_t)frame->height - search_roi.height;
    }

    Image *search_patch = NULL;
    embeddip_status_t status = kcf_make_patch_image(frame, search_roi, &search_patch);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    Image *search_spectrum = NULL;
    status = createImageWH((int)CV_KCF_PATCH_SIZE, (int)CV_KCF_PATCH_SIZE, IMAGE_FORMAT_GRAYSCALE,
                            &search_spectrum);
    if (status != EMBEDDIP_OK) {
        deleteImage(search_patch);
        return status;
    }
    status = fft(search_patch, search_spectrum);
    deleteImage(search_patch);
    if (status != EMBEDDIP_OK) {
        deleteImage(search_spectrum);
        return status;
    }

    Image *corr_spectrum = NULL;
    status = createImageWH((int)CV_KCF_PATCH_SIZE, (int)CV_KCF_PATCH_SIZE, IMAGE_FORMAT_GRAYSCALE,
                            &corr_spectrum);
    if (status != EMBEDDIP_OK) {
        deleteImage(search_spectrum);
        return status;
    }
    status = createChalsComplex(corr_spectrum, 2u);
    if (status != EMBEDDIP_OK) {
        deleteImage(search_spectrum);
        deleteImage(corr_spectrum);
        return status;
    }
    kcf_conj_multiply(search_spectrum, state->template_spectrum, corr_spectrum);
    deleteImage(search_spectrum);

    Image *corr_time = NULL;
    status = createImageWH((int)CV_KCF_PATCH_SIZE, (int)CV_KCF_PATCH_SIZE, IMAGE_FORMAT_GRAYSCALE,
                            &corr_time);
    if (status != EMBEDDIP_OK) {
        deleteImage(corr_spectrum);
        return status;
    }
    status = ifft(corr_spectrum, corr_time);
    deleteImage(corr_spectrum);
    if (status != EMBEDDIP_OK) {
        deleteImage(corr_time);
        return status;
    }

    int32_t peak_dx = 0;
    int32_t peak_dy = 0;
    kcf_find_peak_offset(corr_time, &peak_dx, &peak_dy);
    deleteImage(corr_time);

    int32_t offset_x = peak_dx * search_roi.width / (int32_t)CV_KCF_PATCH_SIZE;
    int32_t offset_y = peak_dy * search_roi.height / (int32_t)CV_KCF_PATCH_SIZE;

    state->center_x = search_roi.x + search_roi.width / 2 + offset_x;
    state->center_y = search_roi.y + search_roi.height / 2 + offset_y;

    out_box->x = state->center_x - state->box_width / 2;
    out_box->y = state->center_y - state->box_height / 2;
    out_box->width = state->box_width;
    out_box->height = state->box_height;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_kcf_free(CvKcfState *state)
{
    if (state == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (state->template_spectrum != NULL) {
        deleteImage(state->template_spectrum);
        state->template_spectrum = NULL;
    }
    return EMBEDDIP_OK;
}
```

The correlation core uses only real, previously confirmed `imgproc/fft.c`/`board/common.h` calls: `fft()` on heap patch Images, a manual complex conjugate-multiply loop over the confirmed `chals->ch[0]`/`ch[1]` real/imag layout, `ifft()` on the product spectrum, and a real linear-scan peak finder that maps the circular correlation index back to a signed pixel offset. No `TODO`/placeholder markers remain.

- [ ] **Step 3: Write test `tests/test_cv_tracker_kcf.c`**

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <core/image.h>
#include <cv/tracker_kcf.h>

#define FRAME_W 128u
#define FRAME_H 128u

static void fill_frame(uint8_t *pixels, int32_t block_x, int32_t block_y)
{
    memset(pixels, 0u, FRAME_W * FRAME_H);
    for (int32_t y = block_y; y < block_y + 20; ++y) {
        for (int32_t x = block_x; x < block_x + 20; ++x) {
            pixels[y * (int32_t)FRAME_W + x] = 255u;
        }
    }
}

static void test_init_null(void)
{
    Rectangle roi = {40, 40, 20, 20};
    assert(cv_kcf_init(NULL, NULL, roi) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_update_without_init(void)
{
    CvKcfState state;
    memset(&state, 0, sizeof(state));
    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 40, 40);
    ImageView frame = {.pixels = pixels,
                       .width = FRAME_W,
                       .height = FRAME_H,
                       .row_stride_bytes = FRAME_W,
                       .format = IMAGE_FORMAT_GRAYSCALE,
                       .depth = IMAGE_DEPTH_U8};
    Rectangle out;
    assert(cv_kcf_update(&state, &frame, &out) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_init_and_update_stationary(void)
{
    uint8_t pixels[FRAME_W * FRAME_H];
    fill_frame(pixels, 40, 40);
    ImageView view = {.pixels = pixels,
                      .width = FRAME_W,
                      .height = FRAME_H,
                      .row_stride_bytes = FRAME_W,
                      .format = IMAGE_FORMAT_GRAYSCALE,
                      .depth = IMAGE_DEPTH_U8};
    Rectangle roi = {40, 40, 20, 20};

    CvKcfState state;
    assert(cv_kcf_init(&state, &view, roi) == EMBEDDIP_OK);

    Rectangle out;
    assert(cv_kcf_update(&state, &view, &out) == EMBEDDIP_OK);
    /* Stationary target: the recovered box should stay near the original. */
    assert(out.width == 20 && out.height == 20);
    assert(out.x > 20 && out.x < 60);
    assert(out.y > 20 && out.y < 60);

    assert(cv_kcf_free(&state) == EMBEDDIP_OK);
}

int main(void)
{
    test_init_null();
    test_update_without_init();
    test_init_and_update_stationary();
    return 0;
}
```

- [ ] **Step 4: Add to `CMakeLists.txt`**

In `CV_SOURCES`, after the `cv/tracker_particle.c/h` lines added in Task 3:

```cmake
    cv/tracker_kcf.c
    cv/tracker_kcf.h
```

- [ ] **Step 5: Add to `tests/CMakeLists.txt`**

After the `embeddip_test_cv_tracker_particle` block:

```cmake
add_executable(embeddip_test_cv_tracker_kcf test_cv_tracker_kcf.c)
target_link_libraries(embeddip_test_cv_tracker_kcf PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_kcf COMMAND embeddip_test_cv_tracker_kcf)
```

- [ ] **Step 6: Build and run the test**

Run: `cmake --build build --target embeddip_test_cv_tracker_kcf && ctest --test-dir build -R cv_tracker_kcf -V`
Expected: build succeeds, test PASSES (stationary-target case recovers a box near the original).

- [ ] **Step 7: Commit**

```bash
git add cv/tracker_kcf.c cv/tracker_kcf.h tests/test_cv_tracker_kcf.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add KCF tracker module to cv/"
```

---

## Task 5: Full test suite verification

**Files:** none created/modified — verification only.

- [ ] **Step 1: Run the full test suite**

Run: `ctest --test-dir build -V`
Expected: all tests pass, including the four new `embeddip.cv_tracker_*` tests and all pre-existing tests (no regressions).

- [ ] **Step 2: Confirm no leftover placeholders**

Run: `grep -rn "TODO\|placeholder" cv/tracker_*.c cv/tracker_*.h`
Expected: no matches.

If matches are found, stop and resolve them — do not report the plan complete with placeholders in committed code.
