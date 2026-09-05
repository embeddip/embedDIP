# Chapter 6 Classical Feature Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first portable EmbedDIP `cv/` module so the Book STM32 Chapter 6 topics—spatial grayscale views, summed-area tables, Haar/Viola–Jones primitives, HOG descriptors, and linear classification—compile and run on host, STM32F7, and STM32N6.

**Architecture:** Keep existing `core/`, `imgproc/`, and `wrapper/` APIs source-compatible. Add C11 algorithms under `cv/` that consume non-owning `ImageView` buffers and caller-owned output/model storage; expose them through the existing `embedDIP.h` and `embedDIP.hpp` umbrellas, with a thin non-owning C++ facade. Register sources, headers, install paths, and assert-based CTest executables through the existing CMake structure.

**Tech Stack:** C11 portable scalar implementation, C++17 wrapper, CMake/CTest, `embeddip_status_t`, `ImageView`, host golden fixtures. No OpenCV, CMSIS-DSP, camera SDK, model runtime, heap allocation, or board-specific include in `cv/`.

## Global Constraints

- Existing `imgproc/` and `wrapper/Image` APIs remain source-compatible.
- All new public functions return `embeddip_status_t` and validate null pointers, dimensions, row stride, grayscale format/depth, ROI bounds, capacities, and arithmetic overflow.
- New algorithms accept `ImageView` and caller-owned buffers; no `malloc`, `free`, `memory_alloc`, hidden full-frame allocation, or hard-coded address is permitted in `cv/`.
- Grayscale input is `IMAGE_FORMAT_GRAYSCALE` or `IMAGE_FORMAT_MASK` with `IMAGE_DEPTH_U8`; padded rows are valid when `row_stride_bytes >= width`.
- Integral values must not wrap `uint32_t`; return `EMBEDDIP_ERROR_OVERFLOW` when the worst-case source sum cannot fit.
- HOG uses 9 unsigned orientation bins, configurable cell size, 2×2-cell blocks, deterministic central differences, bilinear bin interpolation, and L2-Hys normalization.
- C++ additions are non-owning, status-returning, non-throwing helpers under `wrapper/`; they do not change existing `Image` ownership semantics.
- Every new source/header carries the repository’s SPDX/MIT and Doxygen conventions and is registered in CMake, umbrella headers, and installation rules.
- Tests are deterministic C executables registered as `embeddip.*`; no test reads or edits the untracked `Book STM32/` directory or requires hardware.

---

### Task 1: Add the `cv/` module and grayscale view contract

**Files:**
- Create: `cv/image_gray.h`
- Create: `cv/image_gray.c`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP.h`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_image_gray.c`

**Interfaces:**
- Consumes: existing `ImageView`, `ImageFormat`, `ImageDepth`, and `embeddip_status_t` from `core/image.h` and `core/error.h`.
- Produces: `embeddip_status_t cv_gray_view_validate(const ImageView *view);` and `embeddip_status_t cv_gray_pixel_u8(const ImageView *view, uint32_t x, uint32_t y, uint8_t *out_pixel);`.

- [ ] **Step 1: Write failing validation and padded-row tests.** Add assertions that a 3×2 grayscale view with a 5-byte row stride validates, reads `(0,0)=1` and `(2,1)=6`, rejects `x == width`, `y == height`, null output, null pixels, zero dimensions, `row_stride_bytes < width`, RGB888 format, and F32 depth.

- [ ] **Step 2: Run the focused test to verify it fails.**

Run:

```bash
cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON
cmake --build build/host --target embeddip_test_cv_image_gray --parallel
```

Expected: configuration or compilation fails because the `cv` target/test and functions do not exist.

- [ ] **Step 3: Implement the contract and register it.** Add `CV_SOURCES` containing `cv/image_gray.c` to the existing `embedDIP` target, add `cv/` to the existing public include path and install directory, include `cv/image_gray.h` from the current C-linkage section in `embedDIP.h`, and register `embeddip_test_cv_image_gray` as `embeddip.cv_image_gray` in `tests/CMakeLists.txt`. The validator must use checked `row_stride_bytes >= width` arithmetic and the accessor must address `row_stride_bytes * y + x` only after bounds validation.

- [ ] **Step 4: Run the focused test and all current tests.**

Run:

```bash
cmake --build build/host --target embeddip_test_cv_image_gray --parallel
ctest --test-dir build/host -R 'embeddip\.cv_image_gray' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: the new test and all pre-existing tests pass.

- [ ] **Step 5: Commit.**

```bash
git add cv CMakeLists.txt embedDIP.h tests/CMakeLists.txt tests/test_cv_image_gray.c
git commit -m "feat: add portable grayscale cv view contract"
```

### Task 2: Implement stride-safe integral images

**Files:**
- Create: `cv/integral.h`
- Create: `cv/integral.c`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP.h`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_integral.c`

**Interfaces:**
- Consumes: `ImageView` and `cv_gray_view_validate()` from Task 1.
- Produces:
  - `typedef struct { uint32_t *values; uint32_t width; uint32_t height; uint32_t row_stride_values; } CvIntegralU32;`
  - `embeddip_status_t cv_integral_u8_u32(const ImageView *src, CvIntegralU32 *dst);`
  - `embeddip_status_t cv_integral_sum_u32(const CvIntegralU32 *table, Rectangle roi, uint64_t *out_sum);`

- [ ] **Step 1: Write failing exact-sum, padding, ROI, and overflow tests.** Use a 3×2 source with rows `[1,2,3,99,99]` and `[4,5,6,99,99]`, a 4-value output stride, and assert the table values `[1,3,6]` / `[5,12,21]`, full sum `21`, ROI `(1,0,2,2)` sum `16`, and rejection of an out-of-bounds/negative ROI. Add a synthetic 65536×65536 view that must return `EMBEDDIP_ERROR_OVERFLOW` before reading pixels because `255 * width * height` exceeds `UINT32_MAX`; use a one-byte dummy pointer because no loop is allowed after rejection.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_integral --parallel
```

Expected: target is not yet defined or the API is missing.

- [ ] **Step 3: Implement the table and ROI sum.** Validate source with Task 1, require `dst->values`, nonzero dimensions, `row_stride_values >= width`, and checked `width * height`/maximum-sum arithmetic. Compute each row with a 64-bit running sum and add the preceding table row, storing only after proving the result fits `uint32_t`. Implement the four-corner ROI sum with signed-safe edge handling and `uint64_t` output.

- [ ] **Step 4: Register and run focused/full tests.** Add `cv/integral.c` to `CV_SOURCES`, include `cv/integral.h` in `embedDIP.h`, register `embeddip_test_cv_integral`, then run:

```bash
cmake --build build/host --target embeddip_test_cv_integral --parallel
ctest --test-dir build/host -R 'embeddip\.cv_(image_gray|integral)' --output-on-failure
```

Expected: both new tests pass and the prior suite remains green.

- [ ] **Step 5: Commit.**

```bash
git add cv/integral.* CMakeLists.txt embedDIP.h tests/CMakeLists.txt tests/test_cv_integral.c
git commit -m "feat: add bounded integral image tables"
```

### Task 3: Add upright Haar and Viola–Jones evaluation

**Files:**
- Create: `cv/haar.h`
- Create: `cv/haar.c`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP.h`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_haar.c`

**Interfaces:**
- Consumes: `CvIntegralU32` and `cv_integral_sum_u32()` from Task 2.
- Produces:
  - `typedef struct { int16_t x; int16_t y; uint16_t width; uint16_t height; int16_t weight_q8; } CvHaarRect;`
  - `typedef struct { uint8_t rectangle_count; CvHaarRect rectangles[3]; int32_t threshold_q8; int32_t left_value; int32_t right_value; } CvHaarWeakClassifier;`
  - `typedef struct { const CvHaarWeakClassifier *weak; size_t weak_count; int32_t threshold; } CvHaarStage;`
  - `typedef struct { const CvHaarStage *stages; size_t stage_count; Rectangle window; } CvHaarCascade;`
  - `embeddip_status_t cv_haar_feature_response(const CvIntegralU32 *table, int32_t origin_x, int32_t origin_y, const CvHaarRect *rectangles, uint8_t rectangle_count, int32_t *out_response_q8);`
  - `embeddip_status_t cv_haar_cascade_eval(const CvIntegralU32 *table, int32_t origin_x, int32_t origin_y, const CvHaarCascade *cascade, bool *out_detected);`

- [ ] **Step 1: Write failing response/stage/cascade tests.** Build a 4×4 integral table from values 1..16. Test one positive rectangle with `weight_q8=256`, a two-rectangle left-minus-right feature, rejection of rectangle count 0 or greater than 3 and out-of-table coordinates, a weak classifier that selects `left_value` below threshold and `right_value` above threshold, one passing stage, and one cascade that fails at its stage threshold.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_haar --parallel
```

Expected: target/API is missing.

- [ ] **Step 3: Implement rectangle and cascade evaluation.** Check every rectangle against table dimensions using signed origin arithmetic before summing. Use `cv_integral_sum_u32()` for each rectangle, accumulate `uint64_t * int16_t`, round/shift by 8 into a checked `int32_t`, and compare weak/stage thresholds without modifying model arrays. A stage passes only when its weak-value sum is at least the stage threshold; a cascade passes only when every stage passes.

- [ ] **Step 4: Register and verify.** Add `cv/haar.c` and its header to the existing target/umbrella/test registration, then run:

```bash
cmake --build build/host --target embeddip_test_cv_haar --parallel
ctest --test-dir build/host -R 'embeddip\.cv_(image_gray|integral|haar)' --output-on-failure
```

Expected: all three feature tests pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/haar.* CMakeLists.txt embedDIP.h tests/CMakeLists.txt tests/test_cv_haar.c
git commit -m "feat: add upright haar and viola jones evaluation"
```

### Task 4: Implement deterministic HOG extraction

**Files:**
- Create: `cv/hog.h`
- Create: `cv/hog.c`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP.h`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_hog.c`

**Interfaces:**
- Consumes: validated grayscale `ImageView` and Task 1 pixel access.
- Produces:
  - `typedef struct { uint16_t cell_size; float l2_hys_clip; } CvHogConfig;`
  - `embeddip_status_t cv_hog_descriptor_size(Rectangle roi, const CvHogConfig *config, size_t *out_length);`
  - `embeddip_status_t cv_hog_extract(const ImageView *src, Rectangle roi, const CvHogConfig *config, float *descriptor, size_t descriptor_capacity, size_t *out_length);`

- [ ] **Step 1: Write failing descriptor-size, orientation, normalization, stride, and capacity tests.** For a 16×16 horizontal-ramp source and `cell_size=4`, assert descriptor length `(4-1)*(4-1)*36 = 324`, all output values are finite and each 36-value block has L2 norm at most 1 after clipping, and the dominant bins are the horizontal-gradient bins. Repeat using a padded row stride. Assert rejection for cell size 0, ROI outside the image, ROI smaller than 2×2 cells, null output, and capacity 323.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_hog --parallel
```

Expected: target/API is missing.

- [ ] **Step 3: Implement size query and extraction.** Validate config/ROI/source, calculate cell/block counts with checked `size_t` arithmetic, and return `EMBEDDIP_ERROR_INVALID_SIZE` or `EMBEDDIP_ERROR_OVERFLOW` before writing. Accumulate 9 unsigned bins per cell using central differences and linear interpolation between adjacent orientation bins, normalize each 2×2-cell block with epsilon `1e-6f`, clip at `l2_hys_clip` (default test value `0.2f`), renormalize, and write exactly the queried number of floats.

- [ ] **Step 4: Register and verify.** Add `cv/hog.c`, include `cv/hog.h`, register the test, and run:

```bash
cmake --build build/host --target embeddip_test_cv_hog --parallel
ctest --test-dir build/host -R 'embeddip\.cv_(image_gray|integral|haar|hog)' --output-on-failure
```

Expected: all four feature tests pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/hog.* CMakeLists.txt embedDIP.h tests/CMakeLists.txt tests/test_cv_hog.c
git commit -m "feat: add stride safe hog descriptors"
```

### Task 5: Add linear classifier and EmbedDIP C++ facade

**Files:**
- Create: `cv/linear_classifier.h`
- Create: `cv/linear_classifier.c`
- Create: `wrapper/CvFeatureWrapper.hpp`
- Create: `wrapper/CvFeatureWrapper.cpp`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP.h`
- Modify: `embedDIP.hpp`
- Modify: `wrapper/ImageWrapper.hpp`
- Modify: `wrapper/ImageWrapper.cpp`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_linear_classifier.c`
- Create: `tests/test_cv_cpp_wrapper.cpp`

**Interfaces:**
- Consumes: `cv_hog_extract()` and the Task 1–4 C types.
- Produces:
  - `typedef struct { const float *weights; const float *bias; uint16_t class_count; uint16_t descriptor_length; } CvLinearClassifier;`
  - `typedef struct { uint16_t class_index; float score; } CvClassScore;`
  - `embeddip_status_t cv_linear_classifier_topk(const CvLinearClassifier *model, const float *descriptor, size_t descriptor_length, size_t top_k, CvClassScore *scores, size_t score_capacity, size_t *out_count);`
  - `class embedDIP::CvFeatures` with non-throwing static methods `validateGray`, `integral`, `haarFeature`, `haarCascade`, `hogSize`, `hog`, and `linearTopK`, each returning the corresponding C status and taking the same caller-owned buffers/models.

- [ ] **Step 1: Write failing classifier and C++ facade tests.** In C, score a two-class, three-element descriptor with known weights and assert descending top-2 output, top-1 behavior, rejection of descriptor length mismatch, null model arrays, `top_k == 0`, and insufficient score capacity. In C++, create an existing `embedDIP::Image`, call `CvFeatures::validateGray` and `CvFeatures::hogSize`, and assert status/length parity with the C APIs without allocating hidden output storage.

- [ ] **Step 2: Run focused tests to verify they fail.**

```bash
cmake --build build/host --target embeddip_test_cv_linear_classifier embeddip_test_cv_cpp_wrapper --parallel
```

Expected: targets/API are missing.

- [ ] **Step 3: Implement scoring and wrappers.** Validate model dimensions and all pointers, calculate each class score in `float`, maintain a bounded descending top-k list with deterministic class-index tie-breaking, and never mutate caller data. Implement `CvFeatures` as thin non-owning calls; add it to `embedDIP.hpp`, add the C headers to `ImageWrapper.hpp` only where needed for type visibility, and expose an `Image::view(ImageView *) const` helper that delegates to existing `image_view_from_image()` without changing image ownership.

- [ ] **Step 4: Register and verify C/C++ integration.** Add C and C++ sources to the existing `CV_SOURCES`/`WRAPPER_SOURCES`, add the C++ test target with the same include/link settings as existing wrapper code, then run:

```bash
cmake --build build/host --target embeddip_test_cv_linear_classifier embeddip_test_cv_cpp_wrapper --parallel
ctest --test-dir build/host -R 'embeddip\.cv_' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: all new C/C++ tests and all existing tests pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/linear_classifier.* wrapper/CvFeatureWrapper.* wrapper/ImageWrapper.* embedDIP.h embedDIP.hpp CMakeLists.txt tests/CMakeLists.txt tests/test_cv_linear_classifier.c tests/test_cv_cpp_wrapper.cpp
git commit -m "feat: add classical feature classifier and cpp facade"
```

### Task 6: Cross-target publication and regression verification

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `README.md`
- Modify: `docs/README.md`
- Modify: `embedDIP.h`
- Modify: `embedDIP.hpp`
- Test: all `tests/test_cv_*.c` and `tests/test_cv_cpp_wrapper.cpp`

**Interfaces:**
- Consumes: all Chapter 6 C/C++ APIs from Tasks 1–5.
- Produces: reproducible host verification and compile-only F7/N6 target coverage for the new `cv/` module.

- [ ] **Step 1: Add concise module documentation.** Document the new `cv/` headers, caller-owned memory rule, grayscale/stride contract, and the Chapter 6 mapping in the existing README/Doxygen style. Correct only documentation claims touched by this module; do not rewrite unrelated legacy chapter references.

- [ ] **Step 2: Configure all supported target profiles.** Run:

```bash
cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON
cmake -S . -B build/f7 -DEMBEDDIP_TARGET_BOARD=STM32F7 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7 -DEMBEDDIP_BUILD_TESTS=OFF
cmake -S . -B build/n6 -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M55 -DEMBEDDIP_BUILD_TESTS=OFF
```

Expected: host config succeeds; F7/N6 configs either compile with their available SDKs or fail only at the pre-existing missing-SDK/profile boundary, never because `cv/` includes a board header.

- [ ] **Step 3: Run final host verification and static checks.**

```bash
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
git diff --check HEAD~5..HEAD
```

Expected: all tests pass and the diff has no whitespace errors.

- [ ] **Step 4: Commit documentation/verification updates.**

```bash
git add README.md docs/README.md CMakeLists.txt embedDIP.h embedDIP.hpp
git commit -m "docs: publish chapter 6 cv module"
```

