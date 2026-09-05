# Chapter 6 Classical Feature Foundation Design

**Status:** Approved implementation direction after the Book STM32 inventory

**Scope:** This slice makes the active Chapter 6 headings executable on host, STM32F7, and STM32N6: spatial image views, summed-area tables, Haar/Viola–Jones feature evaluation, HOG descriptors, and a fixed-size linear classifier. It is deliberately independent of camera drivers, model runtimes, and physical-board evidence.

## Requirements and boundaries

The new Book STM32 headings name spatial filtering, Viola–Jones filters, Haar filters, and Histogram of Oriented Gradients. The commented traditional-CV outline additionally names HOG and Viola–Jones descriptors and classifiers. Existing `imgproc/` functions remain source-compatible; the new bounded, stride-aware algorithms live under `cv/` and use `ImageView` so the same implementation can consume a tightly packed host image or an N6 DMA buffer. This is an additive module in EmbedDIP’s existing `core`/`imgproc`/`wrapper` organization, not a second library architecture.

The implementation will not parse OpenCV XML, allocate hidden full-frame buffers, depend on OpenCV/CMSIS at link time, or embed a trained model. A host-side converter may later emit a compact static cascade/classifier representation. Runtime APIs accept caller-owned storage and report capacity/overflow errors. All numeric behavior is deterministic and defined for 8-bit grayscale input.

## Public contracts

`cv/image_gray.h` provides a read-only grayscale accessor over `ImageView` and validates `IMAGE_FORMAT_GRAYSCALE` or `IMAGE_FORMAT_MASK` with `IMAGE_DEPTH_U8`. It exposes row-stride-safe pixel reads without changing the existing `Image` ABI.

`cv/integral.h` defines:

```c
typedef struct {
    uint32_t *values;
    uint32_t width;
    uint32_t height;
    uint32_t row_stride_values;
} CvIntegralU32;

embeddip_status_t cv_integral_u8_u32(const ImageView *src,
                                     CvIntegralU32 *dst);
embeddip_status_t cv_integral_sum_u32(const CvIntegralU32 *table,
                                      Rectangle roi,
                                      uint64_t *out_sum);
```

The table has one value per source pixel, uses a caller-provided `uint32_t` buffer, and rejects dimensions whose worst-case sum can overflow `uint32_t`. A squared table is deferred until a cascade requires variance normalization; the API must not silently wrap.

`cv/haar.h` defines upright rectangle features as signed weighted rectangles, weak classifiers, stages, and a cascade view. The evaluator consumes an integral table and a window origin, returns a scalar feature response or a binary stage/cascade decision, and never owns the model arrays. The first format supports two- and three-rectangle upright Haar features and signed fixed-point weights; tilted features and XML parsing are explicitly deferred. A separate host converter can produce the static arrays used by the MCU.

`cv/hog.h` defines a caller-configured descriptor with 9 unsigned orientation bins, configurable cell size, 2×2-cell blocks, L2-Hys normalization, and a descriptor-size query. Extraction accepts a grayscale `ImageView`, a rectangular ROI, and caller-provided `float` output. It validates that the ROI is large enough for a whole-cell grid and returns the required length before writing. The scalar reference uses deterministic gradient differences and bilinear orientation-bin interpolation; an MVE/CMSIS optimization can replace the inner loops later without changing the contract.

`cv/linear_classifier.h` defines a fixed-size linear score model and top-1/top-k scoring over a HOG descriptor. Weights and biases are caller-owned `float` arrays; the result reports the selected class and score. This is an inference primitive for Chapter 8’s later recognition pipeline, not a training runtime.

The C++ surface follows the existing wrapper convention rather than bypassing it. `wrapper/CvFeatureWrapper.hpp/.cpp` provides non-owning `ImageView` adapters and status-returning static helpers that call the C contracts; it does not introduce a second ownership model or throw exceptions. The existing `embedDIP::Image` class gains only thin view/feature entry points where they can preserve caller-provided output storage. C++ callers may therefore use the same algorithms without including board headers, while existing `Image` methods and their compatibility behavior remain unchanged.

## Data flow

```text
ImageView (stride + format metadata)
        │
        ├── cv_integral_u8_u32 ──> Haar rectangle/weak/stage/cascade evaluator
        │                                      │
        │                                      └── detection phase later
        └── cv_hog_extract ──> cv_linear_classifier_score
                                           │
                                           └── recognition phase later
```

The book-facing examples can use `image_view_from_image()` on existing F7 images and the same API on N6 camera buffers. No board-specific include may enter `cv/`.

Integration follows the repository’s build and publication structure:

- Add `CV_SOURCES` to the root `CMakeLists.txt`, link it into the existing `embedDIP` target, and keep host/F7/N6 selection in the existing target matrix.
- Include the public `cv/*.h` headers from `embedDIP.h` inside its current C-linkage block; include the C++ wrapper from `embedDIP.hpp` and the existing `ImageWrapper.hpp` surface.
- Install `cv/` headers alongside `core/`, `imgproc/`, `runtime/`, and `wrapper/` through the existing install rule.
- Register feature tests in `tests/CMakeLists.txt` as ordinary `embeddip.*` CTest executables, following the current assert-based test style.
- Preserve SPDX/MIT headers, Doxygen parameter conventions, C11 implementation, C++17 wrappers, and the existing `embeddip_status_t` error vocabulary.

## Error and memory rules

Every public function checks null pointers, dimensions, stride, format/depth, ROI bounds, output capacity, and arithmetic overflow. `EMBEDDIP_ERROR_OVERFLOW` is returned for integral-table range overflow or descriptor-size arithmetic overflow; `EMBEDDIP_ERROR_INVALID_SIZE` is returned for geometrically invalid configurations. No function calls `malloc`, `free`, or a board allocator. The caller owns all tables, descriptors, and models, which makes the maximum working set visible in book listings and suitable for N6 named memory regions.

## Testing strategy

Host CTest adds deterministic tests for:

- padded grayscale views and exact integral sums, including a max-value overflow rejection;
- two- and three-rectangle Haar responses with known tables and cascade thresholds;
- HOG descriptor length, normalization, orientation-bin placement, ROI/stride validation, and insufficient-output rejection;
- linear classifier class selection and invalid model/descriptor dimensions.

The tests use small hand-computed fixtures plus one synthetic edge/gradient image. No test depends on the untracked `Book STM32/` directory or a board SDK. Existing host, F7, and N6 CMake profiles must continue to configure, and the new sources must compile with the same C11 warning policy.

## Upstream implementation guidance

The scalar algorithms may use the structure of OpenMV’s MIT-licensed integral/Haar/HOG implementations and the BSD-3-Clause FAST/ORB code as reference, with attribution retained if code is adapted. OpenCV is a behavioral reference only; its desktop object-detection modules are not a dependency. CMSIS-DSP/MVE acceleration remains an optional later backend.

## Deferred work

Chapter 7 adds Harris/FAST corners, line segments, keypoints, binary descriptors, and matching. Chapter 8 adds recognition databases/evaluation and Chapter 9 adds contours, component statistics, saliency/attention, detector result lists, image pyramids, NMS, and Haar/HOG sliding-window detectors. Their APIs will consume the contracts in this slice but are not hidden inside it.
