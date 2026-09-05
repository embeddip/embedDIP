# CV Book (STM32N6) Library Coverage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close every *portable* algorithm gap the "Book STM32" computer-vision curriculum (20 chapters, 4 parts, 285 pp.) requires of the `embedDIP` library, so each chapter's on-device techniques have a host-testable library backing, mirroring how the STM32F7 predecessor book (`examples-stm32`) had one app per listing.

**Architecture:** Add C11 algorithms under the existing `cv/` and `imgproc/` modules, consuming non-owning `ImageView`/`cv_tensor_t` inputs and caller-owned output buffers. No heap, no board headers, no SDK dependency in portable code. Register through `embedDIP.h`, the C++ facade (`wrapper/CvFeatureWrapper.hpp`), and CTest, exactly as the existing `cv/` modules do. Board-layer and SDK-backend work is captured as a phased roadmap (Phases 6–8), not as TDD tasks, because it requires N6 hardware and vendor SDKs and lives largely in the example project.

**Tech Stack:** C11 portable scalar, C++17 non-owning facade, CMake/CTest, `embeddip_status_t`, `ImageView`, `cv_tensor_t`, host assert-based golden tests. No OpenCV, CMSIS-DSP, camera SDK, model runtime, or heap in portable modules.

## Global Constraints

- Existing `core/`, `imgproc/`, `cv/`, `runtime/`, and `wrapper/` APIs remain source-compatible.
- All new public functions return `embeddip_status_t` and validate null pointers, dimensions, row stride, format/depth, ROI/bounds, capacities, and arithmetic overflow before writing output.
- New algorithms accept `ImageView`/`cv_tensor_t` and caller-owned buffers; no `malloc`, `free`, `memory_alloc`, hidden allocation, or hard-coded address in `cv/` or `imgproc/`.
- Grayscale input is `IMAGE_FORMAT_GRAYSCALE` or `IMAGE_FORMAT_MASK` with `IMAGE_DEPTH_U8`; padded rows valid when `row_stride_bytes >= width`. Reuse `cv_gray_view_validate()`.
- Every new source/header carries `// SPDX-License-Identifier: MIT` / `// Copyright (c) 2025 EmbedDIP` and Doxygen comments, and is registered in `CV_SOURCES` (or the imgproc list), `embedDIP.h`, and `tests/CMakeLists.txt` as `embeddip.<name>`.
- Tests are deterministic C executables; no test reads or edits the untracked `Book STM32/` directory or requires hardware.
- Host build/verify commands:
  - `cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON`
  - `cmake --build build/host --parallel`
  - `ctest --test-dir build/host --output-on-failure`

---

## Audit Basis (why each task exists)

Five parallel audits cross-referenced every chapter's `.tex` prose and `codes/` listings against the library inventory. Findings:

- **Ch4/5** (acquisition, color, JPEG): zero portable gaps — `cvtColor` covers all RGB888/565/GRAY/YUV/HSI permutations; `compress`, wrappers complete. Remaining work is **N6 device-layer port** (Phase 6).
- **Ch6** (features): `cv/{integral,haar,hog,detect,linear_classifier}` cover Viola-Jones eval + HOG. Gap: multi-scale is manual and `resize` is nearest-neighbor only → **Task 1 (bilinear resize)**.
- **Ch7** (primitives): edges (Canny/LoG/DoG/gradients) and lines (Hough) present. **Corner extraction, keypoint descriptors, and line-*segment* extraction are entirely absent** → **Tasks 2–4**.
- **Ch8/9**: prose empty; listings (FFT filtering, segmentation) fully covered. No portable gap.
- **Ch10/11**: on-device = float MNIST. Portable gaps: **output dequantize** and **raw-buffer normalize** → **Task 5**. Backends (X-CUBE-AI `ai_*`, TFLM) are **Phase 7**.
- **Ch12/13/14**: classification + segmentation on device. Portable gaps: **RGB/multichannel input quantize** (Task 5), **segmentation overlay/blend** (Task 6, shared with Ch9), **detection box+label drawing** (Task 7), **YOLO/FOMO output decoder** (Task 8). Ch14 prose is a stub — the decoder is built against the standard grid/anchor contract so it is ready when the chapter is written.
- **Ch15–20** (ViT, tracking, scene analysis, image understanding, 3-D, VLM): all `.tex` stubs, no listings → **Phase 8 roadmap** only.

Structural note recorded for the book owner: prose `.tex` files are **new and mostly unwritten** (Ch6/7/8/9/14/15–20 are section-header stubs), and each written NN chapter's `\lstinputlisting` points at `codes/` numbered **one higher** (Ch10.tex → `codes/Chapter11`), a legacy of the F7 numbering. This does not change the library gaps.

---

### Task 1: Bilinear resize option for multi-scale pyramids

**Files:**
- Modify: `imgproc/imgwarp.h`
- Modify: `imgproc/imgwarp.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_imgwarp_bilinear.c`

**Interfaces:**
- Consumes: existing `resize(Image *src, Image *dst, int width, int height)` (nearest-neighbor).
- Produces: `embeddip_status_t resizeBilinear(Image *src, Image *dst, int width, int height);` — grayscale U8 bilinear scaler, used by callers building image pyramids for multi-scale Viola-Jones/HOG (Ch6).

- [ ] **Step 1: Write the failing test.** Create `tests/test_imgwarp_bilinear.c`. Build a 2×2 grayscale `Image` with pixels `[0,100; 200,255]` via `createImageWH`, resize to 3×3 with `resizeBilinear`, and assert: corners equal the source corners (`out(0,0)=0`, `out(2,0)=100`, `out(0,2)=200`, `out(2,2)=255`); the true center `out(1,1)` equals the 4-neighbor average rounded (`round((0+100+200+255)/4)=139`); null src/dst → `EMBEDDIP_ERROR_NULL_PTR`; zero target dims → `EMBEDDIP_ERROR_INVALID_SIZE`. Use the existing image lifecycle test (`tests/test_image_lifecycle.c`) as the pattern for constructing and freeing an `Image`.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON
cmake --build build/host --target embeddip_test_imgwarp_bilinear --parallel
```

Expected: configuration or link fails because `resizeBilinear` and the test target do not exist.

- [ ] **Step 3: Implement `resizeBilinear`.** Add the declaration to `imgwarp.h` (Doxygen, grouped with `resize`). In `imgwarp.c`, validate both images non-null, grayscale U8, and target dims > 0. Map each destination pixel to source coordinate `sx = (dx + 0.5) * src_w / dst_w - 0.5` (and `sy` likewise), clamp to `[0, src_dim-1]`, take the 4 integer neighbors, and interpolate with the fractional weights; round to `uint8_t`. Write into `dst->pixels` using `dst`'s stride. Keep `resize` (nearest) unchanged.

- [ ] **Step 4: Register and run.** Add `embeddip_test_imgwarp_bilinear` to `tests/CMakeLists.txt` as `embeddip.imgwarp_bilinear`, then:

```bash
cmake --build build/host --target embeddip_test_imgwarp_bilinear --parallel
ctest --test-dir build/host -R 'embeddip\.imgwarp_bilinear' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: new test and full suite pass.

- [ ] **Step 5: Commit.**

```bash
git add imgproc/imgwarp.h imgproc/imgwarp.c tests/CMakeLists.txt tests/test_imgwarp_bilinear.c
git commit -m "feat: add bilinear resize for image pyramids"
```

### Task 2: Harris / Shi-Tomasi corner response (Ch7 §Corner Extraction)

**Files:**
- Create: `cv/corner.h`
- Create: `cv/corner.c`
- Modify: `CMakeLists.txt` (add to `CV_SOURCES`)
- Modify: `embedDIP.h`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_corner.c`

**Interfaces:**
- Consumes: validated grayscale `ImageView`, `cv_gray_view_validate()`.
- Produces:
  - `typedef enum { CV_CORNER_HARRIS, CV_CORNER_SHI_TOMASI } CvCornerMethod;`
  - `typedef struct { uint16_t x; uint16_t y; float score; } CvCorner;`
  - `embeddip_status_t cv_corner_response(const ImageView *src, Rectangle roi, CvCornerMethod method, float harris_k, float *response, size_t response_capacity, size_t *out_length);` — writes one float response per ROI pixel (row-major, `roi.width*roi.height`).
  - `embeddip_status_t cv_corner_detect(const ImageView *src, Rectangle roi, CvCornerMethod method, float harris_k, float threshold, uint16_t nms_radius, CvCorner *out, size_t out_capacity, size_t *out_count);` — thresholds the response and applies local-max NMS within `nms_radius`.

- [ ] **Step 1: Write the failing test.** Create `tests/test_cv_corner.c`. Build a 9×9 grayscale `ImageView` (stack buffer) that is all 0 with a single bright 3×3 block whose corner sits near the center — a step corner. Assert: `cv_corner_response` on the full ROI returns length `81`, all values finite (`isfinite`), and the maximum response is at the block corner, higher than any flat-region or straight-edge pixel. Assert `cv_corner_detect` with a threshold below that max and `nms_radius=2` returns exactly one corner at the expected `(x,y)`. Assert rejections: `harris_k` used only for HARRIS (Shi-Tomasi ignores it — no error), response capacity `80` → `EMBEDDIP_ERROR_INVALID_SIZE`, null output → `EMBEDDIP_ERROR_NULL_PTR`, ROI smaller than 3×3 → `EMBEDDIP_ERROR_INVALID_SIZE`, ROI outside image → `EMBEDDIP_ERROR_OUT_OF_RANGE`, RGB888 format → `EMBEDDIP_ERROR_INVALID_FORMAT`.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_corner --parallel
```

Expected: target/API missing.

- [ ] **Step 3: Implement the corner module.** In `cv/corner.c`: validate source with `cv_gray_view_validate`, ROI in-bounds and ≥3×3, capacities, and format. For each ROI pixel compute central-difference gradients `Ix, Iy` (replicate-border like `cv/hog.c`), accumulate the structure tensor `[Sxx Sxy; Sxy Syy]` over a 3×3 Gaussian-ish (box is acceptable) window. HARRIS response `= det - k*trace^2` where `det = Sxx*Syy - Sxy^2`, `trace = Sxx+Syy`. SHI_TOMASI response `= min eigenvalue = (trace - sqrt(trace^2 - 4*det))/2`. Write responses; `cv_corner_detect` then scans, keeps pixels above `threshold` that are the strict local maximum within `nms_radius` (Chebyshev), emitting `CvCorner` until capacity. Deterministic scan order (row-major) breaks ties toward lower `y`, then lower `x`.

- [ ] **Step 4: Register and verify.** Add `cv/corner.c`/`cv/corner.h` to `CV_SOURCES`, include `cv/corner.h` in `embedDIP.h`, register `embeddip.cv_corner`, then:

```bash
cmake --build build/host --target embeddip_test_cv_corner --parallel
ctest --test-dir build/host -R 'embeddip\.cv_corner' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: all pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/corner.h cv/corner.c CMakeLists.txt embedDIP.h tests/CMakeLists.txt tests/test_cv_corner.c
git commit -m "feat: add harris and shi-tomasi corner detection"
```

### Task 3: FAST keypoints + BRIEF-style binary descriptor (Ch7 §Keypoint Extraction)

**Files:**
- Create: `cv/keypoint.h`
- Create: `cv/keypoint.c`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP.h`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_keypoint.c`

**Interfaces:**
- Consumes: validated grayscale `ImageView`; `cv_gray_pixel_u8()`.
- Produces:
  - `typedef struct { uint16_t x; uint16_t y; uint16_t score; } CvKeypoint;`
  - `#define CV_BRIEF_BYTES 32` (256-bit descriptor)
  - `embeddip_status_t cv_fast_detect(const ImageView *src, Rectangle roi, uint8_t threshold, uint8_t min_arc, CvKeypoint *out, size_t out_capacity, size_t *out_count);` — FAST-9/16 corner detector (contiguous arc of `min_arc` of 16 ring pixels all brighter-than or darker-than center by `threshold`); score = sum of |ring−center| over the arc.
  - `embeddip_status_t cv_brief_describe(const ImageView *src, const CvKeypoint *keypoints, size_t count, const int8_t *pattern, size_t pattern_len, uint8_t *descriptors, size_t descriptors_capacity);` — for each keypoint writes `CV_BRIEF_BYTES` bytes; `pattern` is `pattern_len` (=512) signed offsets giving 256 (dx0,dy0,dx1,dy1) test pairs; bit set when `sample(p0) < sample(p1)`.
  - `embeddip_status_t cv_hamming_distance(const uint8_t *a, const uint8_t *b, size_t bytes, uint32_t *out_distance);` — popcount of XOR, for descriptor matching.

- [ ] **Step 1: Write the failing test.** Create `tests/test_cv_keypoint.c`. (a) FAST: build a grayscale `ImageView` with a uniform mid-gray field and one bright corner feature at least 3 px from the ROI edge; assert `cv_fast_detect` with a suitable `threshold`/`min_arc=9` returns a keypoint at that location and none in the flat region; assert ROI-edge margin is respected (no keypoint where the 16-ring would read outside the image); assert capacity and null rejections. (b) BRIEF: define a small deterministic `pattern` (e.g. 256 pairs from a fixed table in the test), describe one keypoint twice → identical descriptors; assert `cv_hamming_distance` of a descriptor with itself is 0 and with its bitwise-NOT is `CV_BRIEF_BYTES*8`. (c) `cv_hamming_distance` null/lenght rejections.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_keypoint --parallel
```

Expected: target/API missing.

- [ ] **Step 3: Implement the keypoint module.** In `cv/keypoint.c`: FAST reads the 16-pixel Bresenham ring (offsets hard-coded), requires all pixels of some contiguous arc ≥ `min_arc` to be > center+threshold or < center−threshold; skip pixels whose ring would leave the image (respect ROI + 3 px). `cv_brief_describe` validates `pattern_len == 2 * (CV_BRIEF_BYTES*8) ... ` (512 offsets → 256 pairs → 256 bits → 32 bytes), reads paired samples with border clamp, packs bits MSB-first per byte. `cv_hamming_distance` XORs and popcounts (`__builtin_popcount` on 32-bit chunks with a scalar fallback). No allocation; all outputs caller-owned.

- [ ] **Step 4: Register and verify.** Add sources to `CV_SOURCES`, include in `embedDIP.h`, register `embeddip.cv_keypoint`, then:

```bash
cmake --build build/host --target embeddip_test_cv_keypoint --parallel
ctest --test-dir build/host -R 'embeddip\.cv_keypoint' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: all pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/keypoint.h cv/keypoint.c CMakeLists.txt embedDIP.h tests/CMakeLists.txt tests/test_cv_keypoint.c
git commit -m "feat: add fast keypoints and brief descriptors"
```

### Task 4: Line-segment extraction (Ch7 §Line Segment Extraction)

**Files:**
- Create: `cv/line_segment.h`
- Create: `cv/line_segment.c`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP.h`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_line_segment.c`

**Interfaces:**
- Consumes: validated grayscale/mask `ImageView` (binary edge map, nonzero = edge).
- Produces:
  - `typedef struct { uint16_t x0; uint16_t y0; uint16_t x1; uint16_t y1; uint16_t length; } CvLineSegment;`
  - `embeddip_status_t cv_line_segments_from_edges(const ImageView *edges, Rectangle roi, uint16_t min_length, uint8_t max_gap, CvLineSegment *out, size_t out_capacity, size_t *out_count);` — traces connected runs of edge pixels along the 8 principal directions, bridging gaps up to `max_gap`, emitting segments with pixel `length >= min_length` (endpoints, not the infinite `rho/theta` lines that `houghTransform` yields).

- [ ] **Step 1: Write the failing test.** Create `tests/test_cv_line_segment.c`. Build a binary edge `ImageView` (mask, U8) with one horizontal run of 10 edge pixels on an otherwise blank field. Assert `cv_line_segments_from_edges` with `min_length=5`, `max_gap=0` returns exactly one segment whose endpoints span the run and `length==10`. Add a one-pixel gap in the run: with `max_gap=0` it yields two shorter segments (or one below `min_length`), with `max_gap=1` it yields one bridged segment. Assert a run shorter than `min_length` yields zero. Assert capacity, null, ROI-bounds, and format rejections.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_line_segment --parallel
```

Expected: target/API missing.

- [ ] **Step 3: Implement segment tracing.** In `cv/line_segment.c`: validate source and ROI. Scan ROI row-major; at each unvisited edge pixel, for each of the 4 orientations (0°, 45°, 90°, 135°) walk forward while pixels are edges or within `max_gap` blank pixels of the next edge, marking visited; record start/end and pixel count; emit when `length >= min_length`. Use a caller-independent visited scheme that does not allocate: since no heap is allowed and the edge map is caller-owned (const), track visited via a bounded stack bitset sized to the ROI only if it fits a fixed cap, otherwise walk without a visited buffer by only starting a segment when the backward neighbor in that orientation is not an edge (prevents re-tracing). Prefer the backward-neighbor approach — it needs no scratch and is deterministic. Emit until capacity, then stop and still return `EMBEDDIP_OK` with the truncated count.

- [ ] **Step 4: Register and verify.** Add sources to `CV_SOURCES`, include in `embedDIP.h`, register `embeddip.cv_line_segment`, then:

```bash
cmake --build build/host --target embeddip_test_cv_line_segment --parallel
ctest --test-dir build/host -R 'embeddip\.cv_line_segment' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: all pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/line_segment.h cv/line_segment.c CMakeLists.txt embedDIP.h tests/CMakeLists.txt tests/test_cv_line_segment.c
git commit -m "feat: add line segment extraction from edge maps"
```

### Task 5: Tensor dequantize and multichannel/raw input quantize (Ch11–14)

**Files:**
- Modify: `cv/nn.h`
- Modify: `cv/nn.c`
- Modify: `tests/test_cv_nn.c`

**Interfaces:**
- Consumes: existing `cv_tensor_t`, `cv_nn_image_to_tensor`, `cv_gray_view_validate`.
- Produces:
  - `embeddip_status_t cv_nn_dequantize(const cv_tensor_t *src, float *out, size_t out_capacity, size_t *out_count);` — for I8/U8 tensors writes `(value - zero_point) * scale` per element; for F32 copies through. Element count = `width*height*channels`.
  - `embeddip_status_t cv_nn_image_to_tensor_rgb(const ImageView *src, cv_tensor_t *dst);` — RGB888 (3-channel) input; per channel normalize `px/255` then encode to the tensor type (f32 store, or `round(normalized/scale)+zero_point` clamped for i8/u8), honoring HWC vs CHW layout. Complements the existing grayscale-only `cv_nn_image_to_tensor`.

- [ ] **Step 1: Write the failing test additions.** In `tests/test_cv_nn.c` add: (a) dequantize an I8 tensor `data={-128,0,127}`, `scale=1/255`, `zero_point=-128` → floats `{0, 0.5019.., 1.0}` within `1e-4`; F32 pass-through copies exactly; capacity `count-1` → `EMBEDDIP_ERROR_INVALID_SIZE`; null → `EMBEDDIP_ERROR_NULL_PTR`. (b) `cv_nn_image_to_tensor_rgb` on a 2×1 RGB888 view `[(0,128,255),(255,0,64)]`: F32 HWC output equals `px/255` per channel in interleaved order; CHW output places all R, then all G, then all B; wrong channel count (grayscale view or `dst->channels!=3`) → `EMBEDDIP_ERROR_NOT_SUPPORTED`.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_nn --parallel
```

Expected: link fails — `cv_nn_dequantize` / `cv_nn_image_to_tensor_rgb` undefined.

- [ ] **Step 3: Implement both helpers in `cv/nn.c`.** `cv_nn_dequantize`: validate src/data/out non-null, compute `count = width*height*channels`, capacity check, switch on type (I8 `((int8_t*)data)[i]`, U8, F32 copy). `cv_nn_image_to_tensor_rgb`: validate `src` is RGB888 (add an `ImageFormat` check — accept `IMAGE_FORMAT_RGB888`), `dst->channels==3`, dims match; loop pixels reading 3 bytes, normalize, and write per layout (HWC index `((y*w+x)*3)+c`, CHW index `c*w*h + y*w + x`) with the same type encoding/clamping as `cv_nn_image_to_tensor`. Declare both in `cv/nn.h` with Doxygen.

- [ ] **Step 4: Run and verify full suite.**

```bash
cmake --build build/host --target embeddip_test_cv_nn --parallel
ctest --test-dir build/host -R 'embeddip\.cv_nn' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: all pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/nn.h cv/nn.c tests/test_cv_nn.c
git commit -m "feat: add tensor dequantize and rgb input quantization"
```

### Task 6: Segmentation overlay / alpha blend (Ch9 + Ch13/14, shared gap)

**Files:**
- Modify: `cv/nn.h`
- Modify: `cv/nn.c`
- Modify: `tests/test_cv_nn.c`

**Interfaces:**
- Consumes: RGB888 buffers (e.g. `cv_nn_colorize` output and a camera frame).
- Produces:
  - `embeddip_status_t cv_nn_overlay_rgb(uint8_t *base_rgb, const uint8_t *overlay_rgb, uint32_t width, uint32_t height, uint8_t overlay_percent, const uint8_t *skip_color);` — blends `overlay` onto `base` in place as `base = (base*(100-p) + overlay*p)/100`; if `skip_color` (an RGB triple, may be NULL) is non-NULL, pixels whose overlay equals `skip_color` are left unblended (lets a segmentation background class pass through). This is the 55/45 blend Ch14 listings hand-roll and the `overlay` mask Ch9 `Listing9_27` builds by hand.

- [ ] **Step 1: Write the failing test additions.** In `tests/test_cv_nn.c`: base `[(100,100,100)]`, overlay `[(0,0,200)]`, `overlay_percent=50`, no skip → `(50,50,150)`. With `skip_color=(0,0,200)` (equals overlay) → base unchanged `(100,100,100)`. `overlay_percent=101` → `EMBEDDIP_ERROR_INVALID_ARG`; null base/overlay → `EMBEDDIP_ERROR_NULL_PTR`; zero dims → `EMBEDDIP_ERROR_INVALID_SIZE`.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_nn --parallel
```

Expected: link fails — `cv_nn_overlay_rgb` undefined.

- [ ] **Step 3: Implement `cv_nn_overlay_rgb` in `cv/nn.c`.** Validate pointers, `overlay_percent <= 100`, dims > 0. For each pixel: if `skip_color` set and the 3 overlay bytes equal it, continue; else write `base[k] = (uint8_t)(((uint32_t)base[k]*(100-p) + (uint32_t)overlay[k]*p)/100)` for the 3 channels. Declare in `cv/nn.h` with Doxygen.

- [ ] **Step 4: Run and verify full suite.**

```bash
cmake --build build/host --target embeddip_test_cv_nn --parallel
ctest --test-dir build/host --output-on-failure
```

Expected: all pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/nn.h cv/nn.c tests/test_cv_nn.c
git commit -m "feat: add segmentation overlay alpha blend"
```

### Task 7: Rectangle drawing for detection boxes (Ch14)

**Files:**
- Modify: `imgproc/drawing.h`
- Modify: `imgproc/drawing.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_drawing_rect.c`

**Interfaces:**
- Consumes: existing `Image`, `drawLine`.
- Produces:
  - `embeddip_status_t drawRect(Image *dst, Rectangle rect, uint8_t color, uint8_t thickness);` — draws a rectangle outline (thickness ≥ 1) clipped to the image bounds, for detection/box visualization.

- [ ] **Step 1: Write the failing test.** Create `tests/test_drawing_rect.c`. Make a 10×10 grayscale `Image` initialized to 0, `drawRect` a `{x=2,y=2,width=5,height=5}` with `color=255`, `thickness=1`; assert the four border pixels are 255 (e.g. `(2,2)`, `(6,2)`, `(2,6)`, `(6,6)` corners and a mid-edge point) and an interior pixel `(4,4)` is still 0. Assert a rectangle partially off the top-left is clipped (no out-of-bounds write, in-bounds border still drawn). Null dst → `EMBEDDIP_ERROR_NULL_PTR`; zero width/height → `EMBEDDIP_ERROR_INVALID_SIZE`.

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_drawing_rect --parallel
```

Expected: target/API missing.

- [ ] **Step 3: Implement `drawRect`.** In `drawing.c`, validate dst non-null and `rect.width>0 && rect.height>0`. Clamp the four edges to `[0, width-1]/[0, height-1]`. Draw top, bottom, left, right edges by writing pixels directly (or via `drawLine`) for each of `thickness` offsets, skipping any coordinate outside the image. Declare in `drawing.h` with Doxygen.

- [ ] **Step 4: Register and verify.** Add `embeddip.drawing_rect` to `tests/CMakeLists.txt`, then:

```bash
cmake --build build/host --target embeddip_test_drawing_rect --parallel
ctest --test-dir build/host -R 'embeddip\.drawing_rect' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: all pass.

- [ ] **Step 5: Commit.**

```bash
git add imgproc/drawing.h imgproc/drawing.c tests/CMakeLists.txt tests/test_drawing_rect.c
git commit -m "feat: add rectangle drawing for detection boxes"
```

### Task 8: YOLO/FOMO output decoder feeding NMS (Ch14)

**Files:**
- Modify: `cv/detect.h`
- Modify: `cv/detect.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/test_cv_detect_decode.c`

**Interfaces:**
- Consumes: `cv_tensor_t` (F32 model output), existing `CvDetection`, `cv_detect_nms`.
- Produces:
  - `typedef struct { uint16_t grid_w; uint16_t grid_h; uint16_t num_classes; uint16_t cell_stride; float confidence_threshold; } CvGridDecodeConfig;`
  - `embeddip_status_t cv_detect_decode_fomo(const cv_tensor_t *output, const CvGridDecodeConfig *config, CvDetection *out, size_t out_capacity, size_t *out_count);` — FOMO-style decode: each grid cell holds `num_classes` scores (HWC: channel = class); a cell fires for the argmax class when its score exceeds `confidence_threshold`, emitting a `CvDetection` box centered on the cell (`box = {cell_x*cell_stride, cell_y*cell_stride, cell_stride, cell_stride}`, `score = round(class_score * 1000)`). Caller then runs `cv_detect_nms`.

- [ ] **Step 1: Write the failing test.** Create `tests/test_cv_detect_decode.c`. Build a 2×2-grid, 3-class F32 HWC `cv_tensor_t` where exactly one cell has a class-1 score of `0.9` and the rest are `0.1`. With `cell_stride=8`, `confidence_threshold=0.5`, assert `cv_detect_decode_fomo` returns one detection: `box={8,0,8,8}` (or the cell you set), `score==900`. Lower all scores below threshold → zero detections. Assert capacity truncation returns `EMBEDDIP_OK` with the partial count; null/`num_classes==0`/non-F32 tensor rejections. Then feed two overlapping decoded boxes into `cv_detect_nms` and assert it dedups to one (integration sanity).

- [ ] **Step 2: Run the focused test to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_detect_decode --parallel
```

Expected: target/API missing.

- [ ] **Step 3: Implement `cv_detect_decode_fomo` in `cv/detect.c`.** Validate `output` F32 with `data`, `config` non-null, `num_classes>0`, `grid_w*grid_h` matches `output->width*output->height`, `output->channels==num_classes`, capacity. For each cell compute the argmax class and its score across channels (HWC stride = channels), and if `score > confidence_threshold` and capacity remains, emit the cell box and `score=(int32_t)(class_score*1000.0f)`. Stop at capacity, return `EMBEDDIP_OK`. Add the config struct and declaration to `cv/detect.h` with Doxygen. (YOLO anchor-box decode is deferred — FOMO is what the STM32 model zoo / ST tooling in Ch14 targets; note this in the header comment.)

- [ ] **Step 4: Register and verify.** Add `embeddip.cv_detect_decode` to `tests/CMakeLists.txt`, then:

```bash
cmake --build build/host --target embeddip_test_cv_detect_decode --parallel
ctest --test-dir build/host -R 'embeddip\.cv_detect' --output-on-failure
ctest --test-dir build/host --output-on-failure
```

Expected: all pass.

- [ ] **Step 5: Commit.**

```bash
git add cv/detect.h cv/detect.c tests/CMakeLists.txt tests/test_cv_detect_decode.c
git commit -m "feat: add fomo grid output decoder for detection"
```

### Task 9: C++ facade + docs for the new portable APIs

**Files:**
- Modify: `wrapper/CvFeatureWrapper.hpp`
- Modify: `tests/test_cv_cpp_wrapper.cpp`
- Modify: `README.md`
- Modify: `CMakeLists.txt` (verify new `cv/*.h` in install list)

**Interfaces:**
- Consumes: Tasks 1–8 C APIs.
- Produces: non-throwing static methods on `embedDIP::CvFeatures` — `cornerDetect`, `fastDetect`, `briefDescribe`, `hammingDistance`, `lineSegments`, `dequantize`, `imageToTensorRgb`, `overlayRgb`, `decodeFomo` — each a thin call returning the C status, plus `embedDIP::` free wrappers are not required.

- [ ] **Step 1: Write the failing C++ test additions.** In `tests/test_cv_cpp_wrapper.cpp`, construct a small grayscale `embedDIP::Image`, obtain its `ImageView` via `img.view(&view)`, and call `CvFeatures::cornerDetect(...)` and `CvFeatures::lineSegments(...)` asserting status parity with the C API on the same inputs (status `EMBEDDIP_OK` and equal counts). Add a `CvFeatures::dequantize` call matching the C `cv_nn_dequantize` result.

- [ ] **Step 2: Run to verify it fails.**

```bash
cmake --build build/host --target embeddip_test_cv_cpp_wrapper --parallel
```

Expected: compile fails — new `CvFeatures` methods undefined.

- [ ] **Step 3: Add the facade methods.** In `wrapper/CvFeatureWrapper.hpp`, add `#include` for the new C headers inside the existing `extern "C"` block, then add one `static` non-throwing method per new C function, each forwarding pointers/refs exactly like the existing `hog`/`linearTopK` methods.

- [ ] **Step 4: Update docs and verify.** Extend the `cv/` bullet in `README.md` to name corner/keypoint/line-segment/decoder/overlay additions. Confirm all new `cv/*.h` appear in the install `PUBLIC_HEADER`/directory list in `CMakeLists.txt`. Then:

```bash
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
git diff --check
```

Expected: full suite passes, no whitespace errors.

- [ ] **Step 5: Commit.**

```bash
git add wrapper/CvFeatureWrapper.hpp tests/test_cv_cpp_wrapper.cpp README.md CMakeLists.txt
git commit -m "feat: expose new cv primitives through cpp facade and docs"
```

### Task 10: Cross-target compile verification (HOST + N6)

**Files:**
- Test only: all `tests/test_cv_*.c`, `tests/test_imgwarp_bilinear.c`, `tests/test_drawing_rect.c`, `tests/test_cv_cpp_wrapper.cpp`

**Interfaces:**
- Consumes: all Tasks 1–9 outputs.
- Produces: reproducible host test pass and a clean N6 cross-compile of the new portable sources.

- [ ] **Step 1: Full host verification.**

```bash
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
```

Expected: all `embeddip.*` tests pass.

- [ ] **Step 2: N6 cross-compile the library (portable sources must build for Cortex-M55).** Using the installed SDK (`~/sdk/STM32CubeN6`) and STM32CubeCLT toolchain:

```bash
GCC=/opt/st/stm32cubeclt_1.21.0/GNU-tools-for-STM32/bin
cmake -S . -B build/n6 -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M55 -DEMBEDDIP_BUILD_TESTS=OFF \
  -DEMBEDDIP_STM32CUBE_N6_ROOT=$HOME/sdk/STM32CubeN6 \
  -DCMAKE_C_COMPILER=$GCC/arm-none-eabi-gcc -DCMAKE_CXX_COMPILER=$GCC/arm-none-eabi-g++ \
  -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_C_FLAGS="-mcpu=cortex-m55 --specs=nosys.specs" -DCMAKE_CXX_FLAGS="-mcpu=cortex-m55 --specs=nosys.specs"
cmake --build build/n6 --parallel
```

Expected: `libembedDIP.a` builds; all new `cv/`, `imgproc/` objects compile. Any failure must be a missing-SDK/board boundary, never a portable-source error.

- [ ] **Step 3: Commit any fixes surfaced by the cross-compile** (e.g. a stray non-portable include), then stop — the portable coverage phase is complete.

```bash
git add -A
git commit -m "test: verify cv book portable coverage builds host and n6"
```

---

## Phased Roadmap (not TDD tasks — require hardware, SDKs, or unwritten chapters)

These are deliberately **out of the executable plan above** because they cannot be host-tested and/or depend on content the book has not written yet. They are recorded so the coverage picture is complete.

### Phase 6 — N6 device-layer port (Ch4/5, board-bound)
The portable DIP/color/JPEG surface is complete; the deliverable is device drivers, which currently exist for STM32F7 only (`device/serial/stm32_uart.c` and `device/camera/stm32_ov5640.c` hard-`#include "stm32f7xx_hal.h"`).
- N6 UART `serial_t stm32_uart` (init/send/sendJPEG/send1D/capture) over `stm32n6xx_hal.h`, bound under `#ifdef EMBED_DIP_BOARD_STM32N6` in `serial.h`.
- N6 DCMI/DCMIPP + OV5640 `camera_t stm32_ov5640` (init/capture/stop/setRes) over N6 DMA + SCCB/I²C; define `DEVICE_OV5640` in `board/stm32n6/configs.h`.
- FMC/SDRAM bring-up sequence for the N6 project; libjpeg middleware build config (`EMBEDDIP_HAVE_LIBJPEG=1`).
- Verification requires the STM32N6570-DK board. Mirror the F7 per-listing apps into `examples-stm32n6/examples/`.

### Phase 7 — Model-runtime backends (Ch11–14, SDK-bound)
The `runtime/` tensor + backend + manifest seam exists; concrete backends do not.
- **X-CUBE-AI / ST Edge AI adapter**: a `cv_runtime_invoke_fn` wrapping the generated `ai_<model>_create_and_init`/`_run`/`_inputs_get`/`_outputs_get`, marshalling `ai_buffer` ↔ `cv_tensor_t`. Model-specific codegen from ST Edge AI Core against a real `.tflite`/`.onnx`.
- **TFLite Micro backend**: `MicroInterpreter` + op resolver + arena wrapping, plus a C-array model loader and an aligned tensor-arena helper.
- Both sit behind the existing seam and compile only with their SDK present; verify against the `examples-stm32n6/models/` artifacts.

### Phase 8 — Unwritten advanced chapters (Ch8, 9, 14–20)
These chapters are `.tex` stubs (section headers, no prose, no listings). Library work should follow the prose when written; anticipated portable pieces:
- **Ch8 Image Recognition** (if classical): template matching / NCC / SSD, distance metrics, k-NN — none exist today.
- **Ch9 Object Detection** (if classical): sliding-window + classifier is already assembled from `cv/{hog,haar,detect,linear_classifier}`; wire an example when prose lands.
- **Ch14 Object Detection by NN**: YOLO anchor-box decode (FOMO decode delivered in Task 8); IoU exposed as a public metric.
- **Ch15 Vision Transformers, Ch16 Object Tracking, Ch17 Scene Analysis, Ch18 Image Understanding, Ch19 3-D Vision, Ch20 Vision-Language Models**: no library primitives specified yet. Re-audit each when its prose exists. Tracking (Ch16) likely wants a portable IoU/centroid tracker; 3-D (Ch19) likely wants stereo disparity / rectification — both host-testable when specified.

---

## Self-Review

**Spec coverage:** Every portable gap from the five chapter audits maps to a task — bilinear resize (T1, Ch6), corners (T2, Ch7), keypoints+descriptors (T3, Ch7), line segments (T4, Ch7), dequantize + RGB quant (T5, Ch11–14), overlay/blend (T6, Ch9+13/14), box drawing (T7, Ch14), FOMO decode (T8, Ch14), facade+docs (T9), cross-target verify (T10). Board, SDK-backend, and unwritten-chapter gaps are captured in Phases 6–8 with rationale for exclusion from TDD.

**Placeholder scan:** No TBD/TODO; each code step names concrete files, signatures, formulas, and test assertions.

**Type consistency:** `CvCorner`, `CvKeypoint`, `CvLineSegment`, `CvGridDecodeConfig`, and reused `CvDetection`/`cv_tensor_t`/`Rectangle`/`Image`/`ImageView` names are consistent across tasks and match the current headers (`resize` signature, `cv_nn_*` signatures, `CvDetection{box,score}` verified against source).
