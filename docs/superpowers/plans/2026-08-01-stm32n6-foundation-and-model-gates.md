# STM32N6 Foundation and On-Device Model Gates Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish a tested host and STM32N6570-DK foundation, then prove one camera-to-Neural-ART classifier performs all inference on the MCU and records reproducible evidence.

**Architecture:** Keep EmbedDIP portable by adding a host test profile, non-owning image views, named allocation/cache regions, and a model-agnostic runtime boundary. Add the N6 board and Cortex-M55 profiles without embedding ST-generated model names or addresses in the library. A separate `examples-stm32n6` repository owns CubeN6 startup, camera middleware, generated ST Edge AI artifacts, flashing, and the first application, connected through the public EmbedDIP runtime interface.

**Tech Stack:** C11, C++17, CMake 3.15+, CTest, GNU Arm Embedded toolchain, STM32CubeN6, CMSIS-DSP, ST Edge AI Core 4.0.0 reference workflow, STM32CubeIDE 1.17.0, STM32CubeProgrammer 2.18.0, STM32N6570-DK, MB1854B/IMX335, DCMIPP, Neural-ART NPU.

## Global Constraints

- The only book-promoted hardware target is **STM32N6570-DK**; STM32F7 remains a legacy target and STM32H7 is not a reference target.
- Every neural example must execute inference fully on the MCU; the host may train, convert, flash, or visualize but must never provide deployed inference.
- The default deployed data path is IMX335 camera → DCMIPP/ISP → local NPU → local postprocessing → LCD. UART/file input is a labelled regression path only.
- Do not copy ST CubeN6, camera middleware, ST Edge AI runtime, or generated model sources into `embedDIP`; take them from their licensed upstream packages in `examples-stm32n6`.
- Generated model code and NPU binary weights are model artifacts. The generic library may not contain a model-specific tensor name, shape, raw address, label set, or generated function name.
- Use named regions for DMA/display, fast internal SRAM, PSRAM, and external flash. Algorithms and examples must not contain F7-style hard-coded SDRAM addresses.
- Cache maintenance must round the start down and the end up to the N6 D-cache line size before a DMA/NPU hand-off.
- Record the source-model hash, dataset license, label map, preprocessing, quantization, ST Edge AI version, generated-artifact hash, memory locations, NPU/CPU allocation, accuracy, latency, FPS, and peak workspace for each model.
- The initial pinned reference workflow is ST Edge AI Core 4.0.0, STM32CubeIDE 1.17.0, and STM32CubeProgrammer 2.18.0. A future upgrade must first regenerate and re-benchmark every committed model.
- Preserve the public `createImage*`, `deleteImage`, `memory_init`, `memory_alloc`, `memory_free`, and `memory_realloc` APIs while adding the new interfaces below.
- Keep `Book STM32/` unmodified and untracked; it is source material rather than a deliverable of this work.

---

## Scope boundary and file structure

This is the first of five executable plans derived from the approved feasibility design. It supplies Phase 0 and Phase 1 platform evidence and the Chapter 11/12 vertical slice. It deliberately does not select a final transformer, depth, stereo, or vision-language model: those are decisions that require the measured compiler and memory envelope produced here. The later plans are: (1) Chapter 6–9 classical CV; (2) Chapter 10–13 model applications; (3) Chapter 14–18 advanced local vision; and (4) the separately gated Chapter 19–20 3-D and all-local image-language commitments.

### EmbedDIP repository

| File | Responsibility |
| --- | --- |
| `CMakeLists.txt` | Accept the host and N6 target triples, expose test and N6 SDK options, compile runtime sources, and register CTest. |
| `embedDIP_configs.h` | Enforce exactly one valid board/architecture/CPU selection including HOST/NATIVE and STM32N6/CORTEX_M55. |
| `board/host/board_profile.cmake` | Define a native host profile that has no hardware device implementation. |
| `board/host/board_host_memory.c` | Provide the host allocator and no-op cache operations for deterministic C tests. |
| `board/stm32n6/board_profile.cmake` | Describe the N6570-DK profile and require a CubeN6 installation explicitly. |
| `board/stm32n6/configs.h` | Hold N6-only configuration constants, including the cache-line size and linker-section names. |
| `board/stm32n6/board_stm32n6_memory.c` | Configure named N6 allocator regions from linker symbols and perform line-aligned cache maintenance. |
| `arch/host/arch_profile.cmake` | Declare native compilation without ARM options. |
| `arch/host/host_timer.c` | Provide `tic`/`toc` from a monotonic host clock so runtime tests link without an MCU dependency. |
| `arch/arm/arch_profile.cmake` | Select CM7 or CM55 source files and compiler options from `EMBEDDIP_CPU`. |
| `arch/arm/dwt_timer.c` | Provide the portable ARM DWT `tic`/`toc` implementation without an STM32F7 HAL include. |
| `arch/arm/cmsis_fft.c` | Provide the shared CMSIS-DSP FFT backend for the selected ARM core. |
| `core/image.h` | Define an explicit, non-owning `ImageView` without changing the layout of legacy `Image`. |
| `core/memory_manager.h` | Publish allocation-region, ownership, and cache-maintenance APIs. |
| `core/memory_regions.c` | Route region-aware allocation to the selected board implementation and validate cache-range arguments. |
| `runtime/runtime.h`, `runtime/runtime.c` | Define and enforce the model-independent tensor and runtime-backend contract. |
| `runtime/model_manifest.h`, `runtime/model_manifest.c` | Define the compiled manifest record required by every deployed model. |
| `runtime/stedgeai_n6/backend.h`, `runtime/stedgeai_n6/backend.c` | Adapt a model-specific STAI callback set to `cv_runtime_backend_t` without including generated headers. |
| `tools/model_manifest.py` | Validate a canonical JSON model manifest and render its compiled C record. |
| `tests/*.c`, `tests/*.py`, `tests/cmake/*.cmake` | Cover host allocation, target matrix, image views, cache alignment, runtime shape checks, and model-manifest validation. |

### Companion `examples-stm32n6` repository

Create this sibling repository next to `embedDIP`; it owns all board startup and vendor-generated content.

| File | Responsibility |
| --- | --- |
| `CMakeLists.txt`, `CMakePresets.json`, `cmake/arm-none-eabi-gcc.cmake` | Build one named target per book listing with the N6 compiler and pinned SDK paths. |
| `cmake/stedgeai_generate.cmake` | Run the pinned ST Edge AI generation command and turn the external-flash NPU blob into the linked flash image. |
| `platform/stm32n6570_dk/n6_boot.c` | Enable NPU RAM, XSPI RAM/NOR mapped mode, security configuration, and clocks needed during NPU execution. |
| `platform/stm32n6570_dk/n6_camera_pipeline.[ch]` | Configure DCMIPP pipe 1 for RGB565 LCD output and pipe 2 for the current model's input tensor. |
| `platform/stm32n6570_dk/n6_display.[ch]` | Draw camera frames, labels, and measured timing on the local LCD. |
| `examples/ch05_live_preview/main.c` | Prove the IMX335-to-LCD path before a model is introduced. |
| `examples/ch12_local_classifier/main.c` | Run the first classifier from a live N6 camera frame and show its local result. |
| `models/ch12_local_classifier/manifest.json` | Canonical classifier deployment record. |
| `models/ch12_local_classifier/generate.sh` | Generate the model code and external-flash NPU blob from the committed ONNX source. |
| `models/ch12_local_classifier/generated/model_binding.c` | Bind one STAI-generated `stai_network` instance to the EmbedDIP N6 callback adapter. |
| `scripts/flash_n6.sh` | Program the boot image and model-weight image to external flash, reset the board, and report success or failure. |
| `docs/benchmarks/ch12_local_classifier.md` | Publish the exact board, model, memory, operator-allocation, latency, FPS, and accuracy result. |

### Public interfaces locked by this plan

```c
typedef enum {
    EMBEDDIP_MEMORY_REGION_DEFAULT = 0,
    EMBEDDIP_MEMORY_REGION_FAST_SRAM,
    EMBEDDIP_MEMORY_REGION_DMA,
    EMBEDDIP_MEMORY_REGION_PSRAM,
    EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH
} embeddip_memory_region_t;

typedef enum {
    EMBEDDIP_BUFFER_CPU_READ = 1u << 0,
    EMBEDDIP_BUFFER_CPU_WRITE = 1u << 1,
    EMBEDDIP_BUFFER_DMA_READ = 1u << 2,
    EMBEDDIP_BUFFER_DMA_WRITE = 1u << 3,
    EMBEDDIP_BUFFER_NPU_READ = 1u << 4,
    EMBEDDIP_BUFFER_NPU_WRITE = 1u << 5,
    EMBEDDIP_BUFFER_READ_ONLY = 1u << 6
} embeddip_buffer_flags_t;

typedef struct {
    uint8_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t row_stride_bytes;
    ImageFormat format;
    ImageDepth depth;
    embeddip_memory_region_t region;
    uint32_t flags;
} ImageView;

embeddip_status_t image_view_from_buffer(uint8_t *pixels, uint32_t width,
                                          uint32_t height, uint32_t row_stride_bytes,
                                          ImageFormat format, ImageDepth depth,
                                          embeddip_memory_region_t region,
                                          uint32_t flags, ImageView *out_view);
embeddip_status_t image_view_from_image(const Image *image, ImageView *out_view);
uint8_t *image_view_row(const ImageView *view, uint32_t y);

void *memory_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment);
embeddip_status_t memory_cache_clean(const void *address, size_t size);
embeddip_status_t memory_cache_invalidate(const void *address, size_t size);

typedef enum { CV_TENSOR_U8, CV_TENSOR_I8, CV_TENSOR_F32 } cv_tensor_type_t;
typedef enum { CV_TENSOR_HWC, CV_TENSOR_CHW } cv_tensor_layout_t;

typedef struct {
    void *data;
    uint32_t bytes;
    uint16_t width;
    uint16_t height;
    uint16_t channels;
    cv_tensor_type_t type;
    cv_tensor_layout_t layout;
    float scale;
    int32_t zero_point;
    embeddip_memory_region_t region;
    uint32_t flags;
} cv_tensor_t;

typedef embeddip_status_t (*cv_runtime_invoke_fn)(void *context,
                                                    const cv_tensor_t *input,
                                                    cv_tensor_t *output);
typedef struct {
    void *context;
    cv_tensor_t input_contract;
    cv_tensor_t output_contract;
    cv_runtime_invoke_fn invoke;
} cv_runtime_backend_t;

typedef enum {
    CV_DEPLOYMENT_MCU = 0,
    CV_DEPLOYMENT_HOST
} cv_deployment_location_t;

typedef struct {
    const char *model_id;
    const char *source_sha256;
    const char *generated_sha256;
    const char *stedgeai_version;
    const char *cube_n6_version;
    const char *license;
    const char *dataset_license;
    const char *label_map_id;
    const char *training_recipe;
    const char *quantization_recipe;
    cv_tensor_t input;
    cv_tensor_t output;
    uint32_t weights_bytes;
    uint32_t activations_bytes;
    embeddip_memory_region_t weights_region;
    embeddip_memory_region_t activations_region;
    cv_deployment_location_t deployment_location;
} cv_model_manifest_t;

embeddip_status_t cv_runtime_init(const cv_runtime_backend_t *backend);
embeddip_status_t cv_runtime_infer(const cv_tensor_t *input, cv_tensor_t *output,
                                   uint32_t *elapsed_cycles);
embeddip_status_t cv_model_manifest_validate(const cv_model_manifest_t *manifest);
```

## Task 1: Add a native host profile and CTest safety net

**Files:**
- Create: `board/host/board_profile.cmake`
- Create: `board/host/board_host_memory.c`
- Create: `arch/host/arch_profile.cmake`
- Create: `arch/host/host_timer.c`
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_image_lifecycle.c`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP_configs.h`

**Interfaces:**
- Consumes: existing `createImageWH`, `deleteImage`, `memory_init`, and `memory_alloc` declarations.
- Produces: the configure triple `HOST + HOST + NATIVE`, a host-only static library, and the CTest executable `embeddip_test_image_lifecycle`.

- [ ] **Step 1: Write the failing lifecycle test and its CTest registration.**

```c
/* tests/test_image_lifecycle.c */
#include <assert.h>
#include <stdint.h>
#include <board/common.h>
#include <core/memory_manager.h>

int main(void)
{
    Image *image = 0;
    memory_init(0);
    assert(createImageWH(3, 2, IMAGE_FORMAT_RGB888, &image) == EMBEDDIP_OK);
    assert(image != 0);
    assert(image->width == 3u && image->height == 2u);
    assert(image->size == 6u && image->depth == IMAGE_DEPTH_U24);
    assert(image->pixels != 0);
    ((uint8_t *)image->pixels)[17] = 0xA5u;
    assert(((uint8_t *)image->pixels)[17] == 0xA5u);
    deleteImage(image);
    return 0;
}
```

```cmake
# tests/CMakeLists.txt
add_executable(embeddip_test_image_lifecycle test_image_lifecycle.c)
target_link_libraries(embeddip_test_image_lifecycle PRIVATE embedDIP)
add_test(NAME embeddip.image_lifecycle COMMAND embeddip_test_image_lifecycle)
```

- [ ] **Step 2: Run the host configuration to verify it fails before the profile exists.**

Run: `cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON`

Expected: configuration fails because `HOST` is absent from the target matrix or `board/host/board_profile.cmake` is absent.

- [ ] **Step 3: Add only the host build path needed by this test.**

Add `HOST` and `NATIVE` to the CMake cache-string lists and accept only `HOST + HOST + NATIVE` in the compatibility matrix. Add `EMBEDDIP_BUILD_TESTS` as an `OFF` option; after `add_library`, call `enable_testing()` and `add_subdirectory(tests)` only when it is `ON`. The new board profile must list `${BOARD_COMMON_SOURCES}`, `board/host/board_host_memory.c`, `EMBED_DIP_BOARD_HOST=1`, and no device source. The architecture profile must list only `arch/host/host_timer.c`, define `EMBED_DIP_ARCH_HOST=1` and `EMBED_DIP_CPU_NATIVE=1`, and set no ARM compiler options.

Implement the host allocator with the standard heap, preserving the legacy names:

```c
/* board/host/board_host_memory.c */
void memory_init(uintptr_t ignored) { (void)ignored; }
void *memory_alloc(size_t size) { return size == 0u ? NULL : malloc(size); }
void memory_free(void *ptr) { free(ptr); }
void *memory_realloc(void *ptr, size_t size) {
    if (size == 0u) { free(ptr); return NULL; }
    return realloc(ptr, size);
}
```

Implement the host timing shim with `clock_gettime(CLOCK_MONOTONIC, ...)`. `tic` stores the start time and `toc` returns elapsed nanoseconds as an unsigned 32-bit count; it returns zero if `tic` has not been called. This is a test-only implementation of the existing timing API, not a replacement for ARM DWT cycle timing.

Extend `embedDIP_configs.h` so its exactly-one checks include `EMBED_DIP_BOARD_HOST`, `EMBED_DIP_ARCH_HOST`, and `EMBED_DIP_CPU_NATIVE`; its compatibility branch must accept only the host triple. Do not enable hardware camera or display macros for HOST.

- [ ] **Step 4: Build and run the host test.**

Run: `cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON && cmake --build build/host --parallel && ctest --test-dir build/host --output-on-failure`

Expected: the build succeeds and `embeddip.image_lifecycle` passes.

- [ ] **Step 5: Commit the independently usable host test foundation.**

```bash
git add CMakeLists.txt embedDIP_configs.h board/host arch/host tests
git commit -m "test: add native host profile and image lifecycle test"
```

## Task 2: Generalize target selection and ARM common code for Cortex-M55

**Files:**
- Create: `arch/arm/dwt_timer.c`
- Create: `arch/arm/cmsis_fft.c`
- Create: `tests/cmake/test_target_matrix.cmake`
- Modify: `CMakeLists.txt`
- Modify: `embedDIP_configs.h`
- Modify: `arch/arm/arch_profile.cmake`
- Modify: `arch/arm/cm7_common.c`
- Modify: `arch/arm/cm7_fft.c`

**Interfaces:**
- Consumes: Task 1's CTest setup and the legacy public `void tic(void); uint32_t toc(void);` declarations.
- Produces: valid `STM32N6 + ARM + CORTEX_M55` selection; `tic`/`toc` compiled from `arch/arm/dwt_timer.c`; CMSIS FFT support without a fixed F7-only source selection.

- [ ] **Step 1: Add a configuration-matrix CMake test that is red for an invalid N6 CPU.**

```cmake
# tests/cmake/test_target_matrix.cmake
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${EMBEDDIP_SOURCE_DIR}" -B "${EMBEDDIP_BINARY_DIR}/bad-n6"
          -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7
  RESULT_VARIABLE bad_result OUTPUT_VARIABLE bad_out ERROR_VARIABLE bad_err)
if(bad_result EQUAL 0 OR NOT "${bad_out}${bad_err}" MATCHES "Invalid board/arch/cpu combination")
  message(FATAL_ERROR "STM32N6+CORTEX_M7 must be rejected")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${EMBEDDIP_SOURCE_DIR}" -B "${EMBEDDIP_BINARY_DIR}/good-n6"
          -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M55
  RESULT_VARIABLE good_result OUTPUT_VARIABLE good_out ERROR_VARIABLE good_err)
if(good_result EQUAL 0 OR NOT "${good_out}${good_err}" MATCHES "Board profile not found")
  message(FATAL_ERROR "The N6+CM55 target matrix must reach board-profile selection")
endif()
```

Register it with `add_test(NAME embeddip.target_matrix COMMAND ${CMAKE_COMMAND} -DEMBEDDIP_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DEMBEDDIP_BINARY_DIR=${CMAKE_BINARY_DIR}/matrix -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_target_matrix.cmake)`.

- [ ] **Step 2: Run the CMake test before changing the target matrix.**

Run: `ctest --test-dir build/host -R embeddip.target_matrix --output-on-failure`

Expected: FAIL because the current N6+CM55 configuration is rejected by the target matrix rather than reaching board-profile selection.

- [ ] **Step 3: Add the N6/CM55 matrix entries and remove the F7 HAL dependency from generic ARM timing.**

Change CMake and `embedDIP_configs.h` to recognise `STM32N6`, `CORTEX_M55`, and the sole valid triple `STM32N6 + ARM + CORTEX_M55`; retain the two existing legacy triples. In `arch/arm/arch_profile.cmake`, select `EMBED_DIP_CPU_CORTEX_M7=1` plus `ARM_MATH_CM7` for M7, and `EMBED_DIP_CPU_CORTEX_M55=1` plus `ARM_MATH_MVEI` for M55. Select the matching `-mcpu` and floating-point options there, not in portable code.

Move the DWT implementation into `arch/arm/dwt_timer.c` and compile it for both ARM CPUs:

```c
#if defined(EMBED_DIP_CPU_CORTEX_M7)
#include "core_cm7.h"
#elif defined(EMBED_DIP_CPU_CORTEX_M55)
#include "core_cm55.h"
#endif

void tic(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t toc(void) {
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    return DWT->CYCCNT;
}
```

Change the public `toc` documentation from milliseconds to CPU cycles. Remove the `stm32f7xx_hal.h` include and timer definitions from `cm7_common.c`; leave that file out of the ARM profile source list.

Replace the 256-only `cm7_fft.c` implementation with `cmsis_fft.c` using a cached `arm_cfft_instance_f32` and `arm_cfft_init_f32(&instance, (uint16_t)n)`. Return `EMBEDDIP_ERROR_INVALID_SIZE` when CMSIS rejects `n`; use `arm_cfft_f32(&instance, data, 0, 1)` and `arm_cfft_f32(&instance, data, 1, 1)` for forward and inverse execution. Remove `cm7_fft.c` from the profile source list.

- [ ] **Step 4: Re-run the host tests and validate the diagnostics.**

Run: `cmake --build build/host --parallel && ctest --test-dir build/host --output-on-failure && cmake -S . -B build/invalid-n6 -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7`

Expected: host tests pass; the final command fails with `Invalid board/arch/cpu combination` and lists `STM32N6+ARM+CORTEX_M55` as the valid N6 triple.

- [ ] **Step 5: Commit the target-independent ARM refactor.**

```bash
git add CMakeLists.txt embedDIP_configs.h arch/arm tests
git commit -m "feat: prepare ARM target selection for Cortex-M55"
```

## Task 3: Add image views and named, cache-aware buffer regions

**Files:**
- Create: `core/memory_regions.c`
- Create: `tests/memory_test_hooks.h`
- Create: `tests/test_image_view.c`
- Create: `tests/test_memory_regions.c`
- Modify: `core/image.h`
- Modify: `core/memory_manager.h`
- Modify: `board/common.c`
- Modify: `board/host/board_host_memory.c`
- Modify: `board/stm32f7/board_stm32f7_memory.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Task 1 host allocator and `Image` layout.
- Produces: `ImageView`, `image_view_from_buffer`, `image_view_from_image`, `image_view_row`, `memory_alloc_region`, `memory_cache_clean`, and `memory_cache_invalidate` exactly as declared above.

- [ ] **Step 1: Write failing tests for padded rows, invalid views, region allocation, and cache alignment.**

```c
/* tests/test_image_view.c */
uint8_t pixels[16] = {0};
ImageView view;
assert(image_view_from_buffer(pixels, 3u, 2u, 9u, IMAGE_FORMAT_RGB888,
                              IMAGE_DEPTH_U24, EMBEDDIP_MEMORY_REGION_DMA,
                              EMBEDDIP_BUFFER_DMA_WRITE, &view) == EMBEDDIP_OK);
assert(image_view_row(&view, 1u) == pixels + 9u);
assert(image_view_from_buffer(pixels, 3u, 2u, 8u, IMAGE_FORMAT_RGB888,
                              IMAGE_DEPTH_U24, EMBEDDIP_MEMORY_REGION_DMA,
                              EMBEDDIP_BUFFER_DMA_WRITE, &view) == EMBEDDIP_ERROR_INVALID_SIZE);
```

```c
/* tests/test_memory_regions.c */
void *fast = memory_alloc_region(EMBEDDIP_MEMORY_REGION_FAST_SRAM, 64u, 32u);
assert(fast != NULL && ((uintptr_t)fast % 32u) == 0u);
assert(memory_cache_clean((const void *)0x1003u, 61u) == EMBEDDIP_OK);
assert(memory_test_last_cache_range_start() == (uintptr_t)0x1000u);
assert(memory_test_last_cache_range_size() == 64u);
memory_free(fast);
```

The host-only `memory_test_last_cache_range_start` and `memory_test_last_cache_range_size` declarations belong in a guarded `tests/memory_test_hooks.h`, not in the public library API.

- [ ] **Step 2: Build and run the two tests to verify the new APIs are absent.**

Run: `cmake --build build/host --parallel && ctest --test-dir build/host -R "embeddip.(image_view|memory_regions)" --output-on-failure`

Expected: compilation fails with missing `ImageView`, `memory_alloc_region`, and cache-operation declarations.

- [ ] **Step 3: Implement the portable descriptors and board hooks.**

Add `ImageView` and its functions to `core/image.h` and `board/common.c`. `image_view_from_buffer` must reject a null output pointer, a null pixel pointer, zero dimensions, an unknown format/depth pair, and a `row_stride_bytes` smaller than `width * image_pixel_size_bytes(format, depth)`. `image_view_from_image` must derive a tightly packed stride and use `EMBEDDIP_MEMORY_REGION_DEFAULT` plus CPU read/write flags. `image_view_row` returns `NULL` for a null view or `y >= height`.

Add the region enum, buffer flags, and public prototypes to `core/memory_manager.h`. In `core/memory_regions.c`, dispatch `memory_alloc_region` to the board-local symbol `embeddip_board_alloc_region(region, size, alignment)` and validate a nonzero size, power-of-two alignment, and a writable region. Round `memory_cache_clean` and `memory_cache_invalidate` through `embeddip_board_cache_*`; the board hook receives the rounded address and byte count.

The board-hook surface is internal and exact:

```c
void *embeddip_board_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment);
embeddip_status_t embeddip_board_cache_clean(const void *address, size_t size);
embeddip_status_t embeddip_board_cache_invalidate(const void *address, size_t size);
```

The host implementation must use `posix_memalign` for alignments greater than `alignof(max_align_t)`, map every writable region except `EXTERNAL_FLASH` to the native heap, reject `EXTERNAL_FLASH`, and round cache spans to 32 bytes while only recording the span in the guarded test hooks. The F7 implementation must map `DEFAULT`, `FAST_SRAM`, and `PSRAM` to its existing SDRAM allocator; it returns `EMBEDDIP_ERROR_NOT_SUPPORTED` for cache APIs when the board integration cannot provide a cache operation, and rejects `DMA` and `EXTERNAL_FLASH` instead of inventing an address.

- [ ] **Step 4: Run all host tests with warnings enabled.**

Run: `cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON -DCMAKE_C_FLAGS=-Werror && cmake --build build/host --parallel && ctest --test-dir build/host --output-on-failure`

Expected: lifecycle, target-matrix, image-view, and memory-region tests all pass.

- [ ] **Step 5: Commit the compatibility-preserving buffer contract.**

```bash
git add CMakeLists.txt core/image.h core/memory_manager.h core/memory_regions.c board/common.c board/host board/stm32f7 tests
git commit -m "feat: add image views and named memory regions"
```

## Task 4: Add the STM32N6570-DK and Cortex-M55 profiles

**Files:**
- Create: `board/stm32n6/board_profile.cmake`
- Create: `board/stm32n6/configs.h`
- Create: `board/stm32n6/board_stm32n6_memory.c`
- Create: `tests/cmake/test_stm32n6_profile.cmake`
- Modify: `CMakeLists.txt`
- Modify: `arch/arm/arch_profile.cmake`
- Modify: `tests/cmake/test_target_matrix.cmake`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 2–3 target-matrix and internal board-hook APIs.
- Produces: a CMake profile for `STM32N6 + ARM + CORTEX_M55` and named N6 `FAST_SRAM`, `DMA`, `PSRAM`, and `EXTERNAL_FLASH` policies based on linker symbols.

- [ ] **Step 1: Write the failing N6 profile configuration test.**

```cmake
# tests/cmake/test_stm32n6_profile.cmake
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${EMBEDDIP_SOURCE_DIR}" -B "${EMBEDDIP_BINARY_DIR}/n6"
          -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M55
          -DEMBEDDIP_STM32CUBE_N6_ROOT=/opt/STM32CubeN6
  RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "N6 profile did not configure: ${out}${err}")
endif()
```

Register it only when the `EMBEDDIP_STM32CUBE_N6_ROOT` path exists on the developer machine. Add a second unconditional configure test using `/path/that/does/not/exist` and assert the diagnostic contains `EMBEDDIP_STM32CUBE_N6_ROOT`.

- [ ] **Step 2: Run the missing-SDK assertion before adding the profile.**

Run: `ctest --test-dir build/host -R embeddip.stm32n6_profile --output-on-failure`

Expected: FAIL because no N6 profile gives the required explicit SDK-path diagnostic.

- [ ] **Step 3: Create the N6 profile and linker-symbol region mapping.**

Add `EMBEDDIP_STM32CUBE_N6_ROOT` as a CMake `PATH` cache variable. The N6 board profile must fail unless that directory exists, define `EMBED_DIP_BOARD_STM32N6=1`, `STM32N6xx`, and `STM32N657xx`, include the CubeN6 CMSIS device/core and CMSIS-DSP headers from that root, and include only `${BOARD_COMMON_SOURCES}` plus `board/stm32n6/board_stm32n6_memory.c`. It must not assume `../Drivers`, F7 DCMI, or an internal-flash linker script.

In the ARM profile, choose these CM55 options exactly:

```cmake
set(EMBEDDIP_ARCH_COMPILE_OPTIONS
    -mcpu=cortex-m55 -mcmse -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
list(APPEND EMBEDDIP_ARCH_DEFINES ARM_MATH_MVEI EMBED_DIP_CPU_CORTEX_M55=1)
```

`configs.h` must define `EMBEDDIP_N6_CACHE_LINE_BYTES 32u` and declare the eight linker-owned symbols `__embeddip_fast_sram_start__`, `__embeddip_fast_sram_end__`, `__embeddip_dma_start__`, `__embeddip_dma_end__`, `__embeddip_psram_start__`, `__embeddip_psram_end__`, `__embeddip_xspi_flash_start__`, and `__embeddip_xspi_flash_end__`. Do not replace these symbols with literal addresses; NPU-generated artifacts have their own placement and the example linker script owns final addresses.

Implement `board_stm32n6_memory.c` so `DEFAULT` and `FAST_SRAM` use the fast-SRAM symbol range, `DMA` uses the DMA range, `PSRAM` uses the PSRAM range, and `EXTERNAL_FLASH` is read-only and cannot be allocated. The implementation must report `EMBEDDIP_ERROR_OUT_OF_MEMORY` when a range is exhausted and `EMBEDDIP_ERROR_INVALID_ARG` for a non-power-of-two alignment. Its clean/invalidate functions must use `SCB_CleanDCache_by_Addr` and `SCB_InvalidateDCache_by_Addr` with `EMBEDDIP_N6_CACHE_LINE_BYTES`-rounded spans.

Replace the N6+CM55 section in `tests/cmake/test_target_matrix.cmake` with the existing invalid-M7 assertion only; the valid-N6 configure assertion belongs to `test_stm32n6_profile.cmake` because the board profile now requires an actual CubeN6 directory.

- [ ] **Step 4: Verify profiles without claiming a hardware build from an absent SDK.**

Run: `cmake -S . -B build/n6-missing -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M55 -DEMBEDDIP_STM32CUBE_N6_ROOT=/path/that/does/not/exist`

Expected: fails before compilation and names `EMBEDDIP_STM32CUBE_N6_ROOT`.

On a workstation with CubeN6 installed, run: `cmake -S . -B build/n6 -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M55 -DEMBEDDIP_STM32CUBE_N6_ROOT="$STM32CUBE_N6_ROOT" && cmake --build build/n6 --parallel`

Expected: the static library builds with Cortex-M55 options and no STM32F7 header in its compiler command.

- [ ] **Step 5: Commit the board profile separately from applications.**

```bash
git add CMakeLists.txt arch/arm board/stm32n6 tests
git commit -m "feat: add STM32N6570-DK library profile"
```

## Task 5: Implement the model-independent local runtime and deployment manifest

**Files:**
- Create: `runtime/runtime.h`
- Create: `runtime/runtime.c`
- Create: `runtime/model_manifest.h`
- Create: `runtime/model_manifest.c`
- Create: `runtime/stedgeai_n6/backend.h`
- Create: `runtime/stedgeai_n6/backend.c`
- Create: `tools/model_manifest.py`
- Create: `tests/test_runtime.c`
- Create: `tests/test_model_manifest.py`
- Create: `tests/fixtures/valid_model_manifest.json`
- Create: `tests/fixtures/invalid_model_manifest.json`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 2–4 timing, memory-region, and N6 target definitions.
- Produces: the `cv_tensor_t`, `cv_runtime_backend_t`, `cv_model_manifest_t`, `cv_runtime_init`, `cv_runtime_infer`, and `cv_model_manifest_validate` declarations locked in the public-interface section.

- [ ] **Step 1: Write failing C and Python contract tests.**

```c
/* tests/test_runtime.c */
static int calls;
static embeddip_status_t mock_invoke(void *ctx, const cv_tensor_t *in, cv_tensor_t *out) {
    (void)ctx; ++calls; ((uint8_t *)out->data)[0] = ((const uint8_t *)in->data)[0]; return EMBEDDIP_OK;
}

int main(void) {
    uint8_t in_data[12] = {7}; uint8_t out_data[1] = {0}; uint32_t cycles = 0;
    cv_runtime_backend_t backend = {0};
    backend.input_contract = (cv_tensor_t){.bytes=12,.width=2,.height=2,.channels=3,.type=CV_TENSOR_U8,.layout=CV_TENSOR_HWC};
    backend.output_contract = (cv_tensor_t){.bytes=1,.width=1,.height=1,.channels=1,.type=CV_TENSOR_U8,.layout=CV_TENSOR_HWC};
    backend.invoke = mock_invoke;
    cv_tensor_t in = backend.input_contract; in.data = in_data;
    cv_tensor_t out = backend.output_contract; out.data = out_data;
    assert(cv_runtime_init(&backend) == EMBEDDIP_OK);
    assert(cv_runtime_infer(&in, &out, &cycles) == EMBEDDIP_OK);
    assert(calls == 1 && out_data[0] == 7);
    in.width = 3;
    assert(cv_runtime_infer(&in, &out, &cycles) == EMBEDDIP_ERROR_INVALID_SIZE);
    return 0;
}
```

```json
// tests/fixtures/valid_model_manifest.json
{
  "model": {"id": "unit_classifier", "onnx_file": "unit.onnx", "source_sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
  "generated": {"artifact_sha256": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "weights_blob": "generated/unit.xSPI2.bin"},
  "deployment": {"inference_location": "mcu", "stedgeai_version": "4.0.0", "cube_n6_version": "1.0.0"},
  "legal": {"license": "MIT", "label_map_id": "unit-labels", "dataset_license": "CC0-1.0"},
  "provenance": {"training_recipe": "unit-train-v1", "quantization_recipe": "unit-int8-v1"},
  "io": {"input": {"width": 2, "height": 2, "channels": 3, "type": "u8", "layout": "hwc", "scale": 1.0, "zero_point": 0}, "output": {"width": 1, "height": 1, "channels": 1, "type": "f32", "layout": "hwc", "scale": 1.0, "zero_point": 0}},
  "memory": {"weights_bytes": 64, "activations_bytes": 128, "weights_region": "external_flash", "activations_region": "fast_sram"}
}
```

```python
# tests/test_model_manifest.py
import pathlib, subprocess, sys
root = pathlib.Path(__file__).resolve().parents[1]
tool = root / "tools" / "model_manifest.py"
assert subprocess.run([sys.executable, tool, "validate", root / "tests/fixtures/valid_model_manifest.json"]).returncode == 0
assert subprocess.run([sys.executable, tool, "validate", root / "tests/fixtures/invalid_model_manifest.json"]).returncode != 0
```

The invalid fixture is byte-for-byte the valid fixture except `deployment.inference_location` is `host`.

- [ ] **Step 2: Run tests and confirm both contracts are not yet implemented.**

Run: `cmake --build build/host --parallel && ctest --test-dir build/host -R embeddip.runtime --output-on-failure && python3 tests/test_model_manifest.py`

Expected: the C test fails to compile because `runtime/runtime.h` is absent; the Python command fails because the validator is absent.

- [ ] **Step 3: Implement strict tensor validation, cycle timing, and manifest conversion.**

`cv_runtime_init` must reject a null backend, null invoke callback, zero input/output bytes, zero dimensions, or unsupported tensor type/layout. It stores one copy of the backend. `cv_runtime_infer` must reject calls before initialisation, null tensor/data pointers, a shape/type/layout/byte-count mismatch, and an output marked read-only. It must call `memory_cache_clean(input->data, input->bytes)` when `input->flags` includes `EMBEDDIP_BUFFER_NPU_READ`, then call `tic`, `backend.invoke`, `toc`, and `memory_cache_invalidate(output->data, output->bytes)` when `output->flags` includes `EMBEDDIP_BUFFER_NPU_WRITE`. It writes `elapsed_cycles` only when non-null.

`cv_model_manifest_validate` must reject a null record; empty id, hashes, ST Edge AI version, CubeN6 version, license, dataset license, label-map id, training recipe, or quantization recipe; a non-MCU deployment record; zero weights/activations; an external-flash activation region; and an invalid input/output contract. It accepts external-flash weights and fast-SRAM or PSRAM activations.

The N6 backend must depend only on an explicit callback table; generated STAI headers stay outside the library:

```c
typedef embeddip_status_t (*stedgeai_n6_init_fn)(void *context);
typedef embeddip_status_t (*stedgeai_n6_run_fn)(void *context,
                                                 const cv_tensor_t *input,
                                                 cv_tensor_t *output);
typedef struct { void *context; stedgeai_n6_init_fn init; stedgeai_n6_run_fn run; } stedgeai_n6_binding_t;
embeddip_status_t stedgeai_n6_backend_create(const stedgeai_n6_binding_t *binding,
                                              const cv_tensor_t *input_contract,
                                              const cv_tensor_t *output_contract,
                                              cv_runtime_backend_t *out_backend);
```

`tools/model_manifest.py` uses only Python's standard `argparse`, `json`, `hashlib`, and `pathlib` modules. Its `validate` command requires precisely the seven top-level records in the fixture (`model`, `generated`, `deployment`, `legal`, `provenance`, `io`, and `memory`), requires the named child fields and types shown in the fixture, checks that both hashes are 64 hexadecimal characters, accepts only `mcu` deployment, permits only `u8`, `i8`, and `f32` tensor types and `hwc` or `chw` layouts, and rejects nonpositive dimensions or byte counts. Its `render-c` command emits one `const cv_model_manifest_t` declaration named from `model.id` after replacing every non-alphanumeric character with `_`; it sets `deployment_location = CV_DEPLOYMENT_MCU` and never embeds a model binary.

- [ ] **Step 4: Build and run the runtime and manifest checks.**

Run: `cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON && cmake --build build/host --parallel && ctest --test-dir build/host --output-on-failure && python3 tests/test_model_manifest.py && python3 tools/model_manifest.py render-c tests/fixtures/valid_model_manifest.json --output /tmp/valid_model_manifest.c`

Expected: all CTests and the Python test pass; `/tmp/valid_model_manifest.c` contains one `const cv_model_manifest_t` with an MCU-only manifest.

- [ ] **Step 5: Commit the model contract without a generated network.**

```bash
git add CMakeLists.txt runtime tools tests
git commit -m "feat: add local model runtime and deployment manifest"
```

## Task 6: Create the N6 companion examples and first camera-to-LCD proof

**Files:**
- Create: `../examples-stm32n6/CMakeLists.txt`
- Create: `../examples-stm32n6/CMakePresets.json`
- Create: `../examples-stm32n6/cmake/arm-none-eabi-gcc.cmake`
- Create: `../examples-stm32n6/platform/stm32n6570_dk/n6_boot.c`
- Create: `../examples-stm32n6/platform/stm32n6570_dk/n6_camera_pipeline.h`
- Create: `../examples-stm32n6/platform/stm32n6570_dk/n6_camera_pipeline.c`
- Create: `../examples-stm32n6/platform/stm32n6570_dk/n6_display.h`
- Create: `../examples-stm32n6/platform/stm32n6570_dk/n6_display.c`
- Create: `../examples-stm32n6/examples/ch05_live_preview/main.c`
- Create: `../examples-stm32n6/scripts/flash_n6.sh`
- Create: `../examples-stm32n6/tests/test_listing_targets.cmake`

**Interfaces:**
- Consumes: Task 4's EmbedDIP N6 build and `ImageView`/DMA APIs.
- Produces: independent targets `ch05_live_preview` and, in Task 7, `ch12_local_classifier`; `n6_camera_pipeline_*` functions below.

- [ ] **Step 1: Write a failing listing-target test.**

```cmake
# ../examples-stm32n6/tests/test_listing_targets.cmake
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${EXAMPLES_BINARY_DIR}" --target help
                OUTPUT_VARIABLE targets RESULT_VARIABLE result)
if(NOT result EQUAL 0 OR NOT targets MATCHES "ch05_live_preview")
  message(FATAL_ERROR "Each listing must be a named CMake target")
endif()
```

- [ ] **Step 2: Run the target-discovery test before creating the example project.**

Run: `cmake -S ../examples-stm32n6 -B /tmp/examples-stm32n6-build`

Expected: fails because the companion repository and its root `CMakeLists.txt` do not exist.

- [ ] **Step 3: Implement the reproducible preview application around the two DCMIPP pipes.**

The companion CMake root must accept `EMBEDDIP_SOURCE_DIR`, `STM32CUBE_N6_ROOT`, and `STEDGEAI_ROOT` as required paths, include `cmake/arm-none-eabi-gcc.cmake`, import EmbedDIP as a subdirectory or installed package, and declare a separate executable target per listing. It must not implement an app-selection script that replaces a shared `main.c`.

Use this exact camera facade:

```c
embeddip_status_t n6_camera_pipeline_init(uint32_t nn_width, uint32_t nn_height,
                                          uint32_t nn_channels, ImageView *display_view,
                                          ImageView *inference_view);
embeddip_status_t n6_camera_pipeline_start(void);
embeddip_status_t n6_camera_pipeline_wait_frame(uint32_t timeout_ms);
void n6_camera_pipeline_stop(void);
```

`n6_camera_pipeline_init` must call `CMW_CAMERA_Init`, configure `DCMIPP_PIPE1` as RGB565 sized for the LCD, configure `DCMIPP_PIPE2` as RGB888/YUV444 at the supplied inference tensor dimensions, allocate pipe-1 and pipe-2 buffers from `EMBEDDIP_MEMORY_REGION_DMA`, and expose both as `ImageView`s. The pipe-2 callback increments a volatile frame counter. `n6_camera_pipeline_wait_frame` returns `EMBEDDIP_ERROR_TIMEOUT` when that counter does not advance by the requested deadline. `n6_boot.c` must enable NPU RAM, configure XSPI RAM and NOR in memory-mapped mode, and enable DCMIPP, NPU, XSPI, LTDC, and DMA2D clocks during NPU sleep intervals.

`ch05_live_preview/main.c` starts only pipe 1, presents the current RGB565 `ImageView` through `n6_display_present`, and writes the first successful frame count to the local UART log. It contains no network client, socket, or host-inference code.

`scripts/flash_n6.sh` accepts one listing target, locates the DK external loader at `$STM32CUBEPROGRAMMER_ROOT/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr`, signs `build/$1/$1.bin` as an SSBL, and programs the required persistent image from the board's development boot mode:

```bash
STM32_SigningTool_CLI -bin "build/$1/$1.bin" -nk -t ssbl -hv 2.3 -o "build/$1/$1_sign.bin"
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -el "$DK_EXTERNAL_LOADER" -hardRst -w FSBL/ai_fsbl.hex
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -el "$DK_EXTERNAL_LOADER" -hardRst -w "build/$1/$1_sign.bin" 0x70100000
```

When `models/$1/generated/network_data.hex` exists, the script must also issue `STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -el "$DK_EXTERNAL_LOADER" -hardRst -w "models/$1/generated/network_data.hex"`. The script prints the required switch to boot-from-flash mode and exits nonzero when the loader, FSBL, signed application, or optional weight file is missing.

- [ ] **Step 4: Build the preview target and validate the board data path.**

Run: `cmake --preset n6570-dk-debug -S ../examples-stm32n6 && cmake --build --preset n6570-dk-debug --target ch05_live_preview --parallel`

Expected: one `.elf` and one bootable N6 image are produced without replacing a shared source file.

Flash with `../examples-stm32n6/scripts/flash_n6.sh ch05_live_preview`; reset the board. Expected hardware evidence: the IMX335 image appears on the LCD for 60 seconds without a DCMIPP error callback or a stale frame count.

- [ ] **Step 5: Commit the preview proof in the companion repository.**

```bash
cd ../examples-stm32n6
git add CMakeLists.txt CMakePresets.json cmake platform examples tests scripts
git commit -m "feat: add independently buildable N6 live preview listing"
```

## Task 7: Generate, flash, and measure a local NPU classifier

**Files:**
- Create: `../examples-stm32n6/models/ch12_local_classifier/manifest.json`
- Create: `../examples-stm32n6/models/ch12_local_classifier/generate.sh`
- Create: `../examples-stm32n6/models/ch12_local_classifier/generated/model_binding.c`
- Create: `../examples-stm32n6/examples/ch12_local_classifier/main.c`
- Create: `../examples-stm32n6/tests/test_classifier_manifest.py`
- Create: `../examples-stm32n6/docs/benchmarks/ch12_local_classifier.md`
- Modify: `../examples-stm32n6/CMakeLists.txt`
- Modify: `../examples-stm32n6/scripts/flash_n6.sh`

**Interfaces:**
- Consumes: Tasks 5–6 `cv_runtime_*`, `stedgeai_n6_backend_create`, `n6_camera_pipeline_*`, and the validated local camera pipeline.
- Produces: an independently buildable `ch12_local_classifier` that invokes a generated STAI N6 network on a live camera frame and a benchmark record proving the deployed inference location is the MCU.

- [ ] **Step 1: Write failing manifest and generated-artifact tests.**

```python
# ../examples-stm32n6/tests/test_classifier_manifest.py
import json, pathlib, subprocess, sys
root = pathlib.Path(__file__).resolve().parents[1]
manifest = root / "models/ch12_local_classifier/manifest.json"
data = json.loads(manifest.read_text())
assert data["deployment"]["inference_location"] == "mcu"
assert data["model"]["onnx_file"] == "efficientnet_v2B1_240_fft_qdq_int8.onnx"
assert data["generated"]["weights_blob"] == "generated/network_data.xSPI2.bin"
assert subprocess.run([sys.executable, root / "../embedDIP/tools/model_manifest.py", "validate", manifest]).returncode == 0
assert (root / "models/ch12_local_classifier/generated/network_data.xSPI2.bin").is_file()
```

The build must also fail if `generated/stai_network.h`, `generated/stai_network.c`, `generated/network.c`, or `generated/network_data.xSPI2.bin` is absent.

- [ ] **Step 2: Run the manifest test before committing the model record or generated artifacts.**

Run: `python3 ../examples-stm32n6/tests/test_classifier_manifest.py`

Expected: fails because the manifest and NPU binary do not yet exist.

- [ ] **Step 3: Generate the exact N6 artifact and bind it through the generic adapter.**

Set the initial model source to `efficientnet_v2B1_240_fft_qdq_int8.onnx`, the N6 reference model reported at 44 ms. `generate.sh` must run this command with the pinned `stedgeai` executable and must record its tool version in `manifest.json`:

```bash
stedgeai generate \
  --model efficientnet_v2B1_240_fft_qdq_int8.onnx \
  --target stm32n6 \
  --st-neural-art default@user_neuralart_STM32N6570-DK.json \
  --input-data-type uint8 --output-data-type float32 --inputs-ch-position chlast
```

Copy the generated `network.c`, `stai_network.c`, `stai_network.h`, and `network_atonbuf.xSPI2.raw` into the model's `generated/` directory; rename the raw buffer to `network_data.xSPI2.bin`. Convert it to an external-flash Intel HEX image at `0x70380000` exactly:

```bash
arm-none-eabi-objcopy -I binary generated/network_data.xSPI2.bin \
  --change-addresses 0x70380000 -O ihex generated/network_data.hex
```

`model_binding.c` must include only the generated headers and use the generated STAI calls `stai_runtime_init`, `stai_network_init`, `stai_network_get_info`, `stai_network_get_inputs`, `stai_network_get_outputs`, and `stai_network_run(network_context, STAI_MODE_SYNC)`. It supplies `stedgeai_n6_binding_t.run`, checks one input and output against the manifest's byte counts, cleans the input cache, runs synchronously, invalidates the output cache, and maps non-`STAI_SUCCESS` status to `EMBEDDIP_ERROR_DEVICE_ERROR`.

The canonical manifest must state: inference location `mcu`; IMX335/DCMIPP camera input; RGB888 HWC uint8 preprocessing at the generated dimensions; output type/layout/scale; model and generated SHA-256 values; weights in `EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH`; activations in `EMBEDDIP_MEMORY_REGION_FAST_SRAM` or `PSRAM` as reported by ST Edge AI; NPU and CPU operator counts; dataset license; labels; and every pinned tool version.

- [ ] **Step 4: Implement the live-camera application and build it as its own target.**

`ch12_local_classifier/main.c` must call N6 boot, initialise the generated binding, use `stedgeai_n6_backend_create`, and set up pipe 2 at the manifest tensor dimensions. Its loop must wait for a new camera frame, populate `cv_tensor_t.data` from the pipe-2 `ImageView`, call `cv_runtime_infer`, compute top-1 from the local output tensor, and draw label, inference cycles, end-to-end frame cycles, and FPS through `n6_display`. It may write the same values to UART, but it may not transmit the frame or invoke a remote endpoint.

Run: `cmake --build --preset n6570-dk-debug --target ch12_local_classifier --parallel && python3 ../examples-stm32n6/tests/test_classifier_manifest.py`

Expected: the target builds; the manifest validator passes; and all generated artifact prerequisites are present.

- [ ] **Step 5: Flash, measure, publish, and commit the hardware gate.**

Run: `../examples-stm32n6/scripts/flash_n6.sh ch12_local_classifier`

On the N6570-DK, run the app for 100 live IMX335 frames. Record in `docs/benchmarks/ch12_local_classifier.md`: board revision; model/source/generated hashes; exact tool versions; input/output tensor shapes; weights and activations bytes plus regions; NPU and CPU operator allocation; top-1 accuracy on the committed fixed-image set; median/p95 NPU cycles; median/p95 end-to-end cycles; FPS; and the local-LCD/UART evidence. The hardware gate passes only when all 100 inferences are initiated by pipe-2 camera frames, the result is shown locally, no network client is linked, and the application has no stale-frame or cache-coherency error.

```bash
cd ../examples-stm32n6
git add CMakeLists.txt examples/ch12_local_classifier models/ch12_local_classifier tests scripts docs/benchmarks
git commit -m "feat: add measured local N6 classifier listing"
```

## Task 8: Lock the evidence and define the gates for the remaining commitments

**Files:**
- Create: `docs/benchmarks/stm32n6-foundation-gate.md`
- Create: `docs/benchmarks/stm32n6-model-gate-template.json`
- Create: `../examples-stm32n6/tests/test_model_gate_record.py`
- Create: `../examples-stm32n6/docs/benchmarks/ch12_local_classifier.json`
- Modify: `docs/superpowers/specs/2026-08-01-stm32n6-computer-vision-feasibility-design.md`

**Interfaces:**
- Consumes: Tasks 1–7 test output, N6 build output, classifier manifest, and on-device benchmark record.
- Produces: an auditable pass/fail record that later chapter/model plans must satisfy before making a hardware claim.

- [ ] **Step 1: Write the failing evidence-completeness checker.**

```python
required = {
  "board_revision", "stedgeai_version", "cube_n6_version", "model_sha256",
  "generated_sha256", "camera_frames", "npu_cycles_median", "npu_cycles_p95",
  "frame_cycles_median", "frame_cycles_p95", "fps", "weights_bytes",
  "activations_bytes", "npu_operator_count", "cpu_operator_count", "local_display"
}
record = json.loads(pathlib.Path(sys.argv[1]).read_text())
missing = required - record.keys()
assert not missing, f"missing: {sorted(missing)}"
assert record["camera_frames"] >= 100
assert record["local_display"] is True
assert record["inference_location"] == "mcu"
```

Place it at `tests/test_model_gate_record.py` in the companion repository and invoke it against the classifier gate record.

- [ ] **Step 2: Run the checker before creating the gate record.**

Run: `python3 ../examples-stm32n6/tests/test_model_gate_record.py ../examples-stm32n6/docs/benchmarks/ch12_local_classifier.json`

Expected: fails because no machine-readable classifier evidence record exists.

- [ ] **Step 3: Publish the foundation gate and machine-readable model record.**

Create `ch12_local_classifier.json` beside the Markdown benchmark. It contains all required checker fields and the source/generation command hashes. `stm32n6-foundation-gate.md` must report a pass only after these facts are linked to the exact commands and output files: host CTests pass; F7 configure still succeeds when its SDK is available; N6 configure/build succeeds; live IMX335 preview lasts 60 seconds; the classifier completes 100 camera-triggered local inferences; no remote inference path exists; and its manifest, model artifact, memory placement, and benchmark record are committed.

The reusable template must require `candidate_name`, `chapter`, `compile_result`, `cpu_fallback_operators`, `weights_bytes`, `activations_bytes`, `inference_location`, `camera_frames`, `accuracy_metric`, `npu_cycles_median`, `npu_cycles_p95`, `frame_cycles_median`, `frame_cycles_p95`, and `decision`. It permits exactly `accepted` and `rejected` decisions. A chapter may cite only an `accepted` record; a rejected result stays recorded so that the book never silently claims unsupported models.

Update the feasibility design status from `Approved direction — detailed implementation plan pending review` to `Approved direction — foundation and on-device model-gate plan accepted` only after all evidence in this task is present.

- [ ] **Step 4: Run all software verification and review the hardware evidence.**

Run: `cmake --build build/host --parallel && ctest --test-dir build/host --output-on-failure && python3 ../examples-stm32n6/tests/test_classifier_manifest.py && python3 ../examples-stm32n6/tests/test_model_gate_record.py ../examples-stm32n6/docs/benchmarks/ch12_local_classifier.json`

Expected: every software check passes. Manually compare the record's 100-frame camera count, local display confirmation, operator allocation, and timing values against the saved board UART capture and LCD recording before marking the foundation gate as passed.

- [ ] **Step 5: Commit the evidence in each repository.**

```bash
git add docs/benchmarks docs/superpowers/specs/2026-08-01-stm32n6-computer-vision-feasibility-design.md
git commit -m "docs: record STM32N6 foundation model gate"

cd ../examples-stm32n6
git add docs/benchmarks tests/test_model_gate_record.py
git commit -m "docs: publish local classifier gate evidence"
```

## Final verification checklist

- [ ] Configure, build, and test the native profile: `cmake -S . -B build/host -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE -DEMBEDDIP_BUILD_TESTS=ON && cmake --build build/host --parallel && ctest --test-dir build/host --output-on-failure`.
- [ ] Confirm an invalid N6 CPU is rejected: `cmake -S . -B build/invalid-n6 -DEMBEDDIP_TARGET_BOARD=STM32N6 -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7`; it must fail.
- [ ] With a real CubeN6 installation, build `embedDIP` for `STM32N6 + ARM + CORTEX_M55` and confirm no F7 header occurs in the compiler command line.
- [ ] Build each companion listing by its own CMake target; no target may copy or replace a shared `main.c`.
- [ ] Flash and observe the live preview for 60 seconds, then run 100 classifier frames from the IMX335; preserve the UART capture and LCD recording alongside the JSON/Markdown benchmark.
- [ ] Check each classifier manifest with `tools/model_manifest.py validate` and confirm its deployment location is `mcu`.
- [ ] Do not approve Chapter 15, 19, or 20 wording until their own candidate records pass the same all-local compile, fit, camera, operator-allocation, accuracy, and latency gates.
