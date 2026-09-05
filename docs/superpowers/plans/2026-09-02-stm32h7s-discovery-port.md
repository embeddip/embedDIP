# STM32H7S78-DK Board Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the STM32H7S78-DK Discovery kit as a first-class embedDIP board target with memory management, build glue, and display + UART drivers (camera deferred).

**Architecture:** Mirror the existing STM32N6 port for memory (region arena over linker symbols + SCB cache maintenance) and CMake/test wiring; mirror the STM32F7 port for the thin ST-HAL driver wrappers and the `embedDIP_configs.h` feature block. H7S is Cortex-M7, so `arch/arm` needs no changes.

**Tech Stack:** C11, CMake, STM32CubeH7RS HAL/CMSIS (out-of-tree, gitignored), CTest for host unit tests.

## Global Constraints

- License header on every new source file: `// SPDX-License-Identifier: MIT` then `// Copyright (c) 2025 EmbedDIP` (match existing files verbatim).
- Board macro: `EMBED_DIP_BOARD_STM32H7S`. CPU: `CORTEX_M7`. Arch: `ARM`.
- CMSIS device part macro: `STM32H7S7xx` (selects `stm32h7s7xx.h`). There is no `STM32H7RSxx` device macro.
- External PSRAM (APS256XX) is memory-mapped at `0x90000000`, size 32 MB. AXI-SRAM base `0x24000000`. Cache line = 32 bytes.
- Commit messages: **no `Co-Authored-By` trailer.** Author is the user only.
- Drivers never initialize peripherals — they wrap `extern` HAL handles the application owns.
- Do not modify F7/N6/ESP32/HOST behavior. Do not vendor the SDK.

---

### Task 1: Memory allocator + board configs.h (host-testable core)

The one piece with real logic: the per-region bump allocator and cache-span rounding. Ported verbatim from the N6 board (identical Cortex D-cache model), and unit-tested on host exactly like `tests/test_stm32n6_memory.c`.

**Files:**
- Create: `board/stm32h7s/configs.h`
- Create: `board/stm32h7s/board_stm32h7s_memory.c`
- Create: `tests/fakes/stm32h7s/stm32h7s7xx.h`
- Create: `tests/test_stm32h7s_memory.c`
- Modify: `tests/CMakeLists.txt` (register the new host test)

**Interfaces:**
- Consumes: `core/memory_manager.h` (`embeddip_memory_region_t`, `embeddip_status_t`, region enum values), same as N6.
- Produces: `memory_init(uintptr_t)`, `memory_alloc(size_t)`, `memory_free(void*)`, `memory_realloc(void*, size_t)`, `embeddip_board_alloc_region(region,size,alignment)`, `embeddip_board_cache_clean(addr,size)`, `embeddip_board_cache_invalidate(addr,size)`. Internal (exposed for tests): `static embeddip_status_t h7s_allocate_region(region,size,alignment,void**)`.

- [ ] **Step 1: Create the CMSIS fake header for host tests**

Create `tests/fakes/stm32h7s/stm32h7s7xx.h`:

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP
// Minimal host fake: the real header pulls in Cortex-M7 core intrinsics.
// The test provides its own SCB_*DCache_by_Addr definitions.
#ifndef EMBEDDIP_TEST_FAKE_STM32H7S7XX_H
#define EMBEDDIP_TEST_FAKE_STM32H7S7XX_H
#include <stdint.h>
void SCB_CleanDCache_by_Addr(void *address, int32_t size);
void SCB_InvalidateDCache_by_Addr(void *address, int32_t size);
#endif
```

- [ ] **Step 2: Create `board/stm32h7s/configs.h`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_STM32H7S_CONFIGS_H
#define EMBEDDIP_STM32H7S_CONFIGS_H

#include <stdint.h>

#define EMBEDDIP_H7S_CACHE_LINE_BYTES 32u

// APS256XX PSRAM, memory-mapped via XSPI2. Used as the LTDC framebuffer base.
#define FRAME_BUFFER 0x90000000u
#define LCD_FRAME_BUFFER 0x90000000u

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t __embeddip_fast_sram_start__[];
extern uint8_t __embeddip_fast_sram_end__[];
extern uint8_t __embeddip_dma_start__[];
extern uint8_t __embeddip_dma_end__[];
extern uint8_t __embeddip_psram_start__[];
extern uint8_t __embeddip_psram_end__[];
extern uint8_t __embeddip_xspi_flash_start__[];
extern uint8_t __embeddip_xspi_flash_end__[];

#ifdef __cplusplus
}
#endif

#endif
```

- [ ] **Step 3: Create `board/stm32h7s/board_stm32h7s_memory.c`**

Verbatim port of `board/stm32n6/board_stm32n6_memory.c` with prefix `n6_`→`h7s_`, type `n6_memory_range_t`→`h7s_memory_range_t`, `EMBEDDIP_N6_CACHE_LINE_BYTES`→`EMBEDDIP_H7S_CACHE_LINE_BYTES`, and the device include swapped to `stm32h7s7xx.h`:

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <configs.h>
#include <stm32h7s7xx.h>

#include "core/memory_manager.h"

#include <limits.h>
#include <stdalign.h>
#include <stdint.h>

typedef struct {
    uintptr_t start;
    uintptr_t end;
    uintptr_t cursor;
} h7s_memory_range_t;

static h7s_memory_range_t fast_sram;
static h7s_memory_range_t dma_memory;
static h7s_memory_range_t psram;
static int memory_initialized;

static void h7s_reset_range(h7s_memory_range_t *range, uint8_t *start, uint8_t *end)
{
    range->start = (uintptr_t)start;
    range->end = (uintptr_t)end;
    range->cursor = range->start;
}

void memory_init(uintptr_t pool_start_addr)
{
    (void)pool_start_addr;
    h7s_reset_range(&fast_sram, __embeddip_fast_sram_start__, __embeddip_fast_sram_end__);
    h7s_reset_range(&dma_memory, __embeddip_dma_start__, __embeddip_dma_end__);
    h7s_reset_range(&psram, __embeddip_psram_start__, __embeddip_psram_end__);
    memory_initialized = 1;
}

static int h7s_alignment_is_power_of_two(size_t alignment)
{
    return alignment != 0u && (alignment & (alignment - 1u)) == 0u;
}

static h7s_memory_range_t *h7s_range_for_region(embeddip_memory_region_t region)
{
    switch (region) {
    case EMBEDDIP_MEMORY_REGION_DEFAULT:
    case EMBEDDIP_MEMORY_REGION_FAST_SRAM:
        return &fast_sram;
    case EMBEDDIP_MEMORY_REGION_DMA:
        return &dma_memory;
    case EMBEDDIP_MEMORY_REGION_PSRAM:
        return &psram;
    case EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH:
    default:
        return NULL;
    }
}

static embeddip_status_t h7s_allocate_region(embeddip_memory_region_t region, size_t size,
                                             size_t alignment, void **allocation)
{
    h7s_memory_range_t *range;
    uintptr_t aligned_cursor;
    uintptr_t alignment_mask;

    if (allocation == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    *allocation = NULL;

    if (!h7s_alignment_is_power_of_two(alignment)) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (size == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (region == EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    range = h7s_range_for_region(region);
    if (range == NULL) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (!memory_initialized) {
        memory_init(0u);
        range = h7s_range_for_region(region);
    }

    alignment_mask = (uintptr_t)alignment - 1u;
    if (range->end < range->start || range->cursor > UINTPTR_MAX - alignment_mask) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    aligned_cursor = (range->cursor + alignment_mask) & ~alignment_mask;
    if (aligned_cursor > range->end || size > range->end - aligned_cursor) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    range->cursor = aligned_cursor + size;
    *allocation = (void *)aligned_cursor;
    return EMBEDDIP_OK;
}

void *embeddip_board_alloc_region(embeddip_memory_region_t region, size_t size, size_t alignment)
{
    void *allocation;

    if (h7s_allocate_region(region, size, alignment, &allocation) != EMBEDDIP_OK) {
        return NULL;
    }
    return allocation;
}

void *memory_alloc(size_t size)
{
    return embeddip_board_alloc_region(EMBEDDIP_MEMORY_REGION_DEFAULT, size, alignof(max_align_t));
}

void memory_free(void *ptr)
{
    (void)ptr;
}

void *memory_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL) {
        return memory_alloc(new_size);
    }
    return NULL;
}

static embeddip_status_t h7s_cache_span(const void *address, size_t size, uintptr_t *rounded_start,
                                        int32_t *rounded_size)
{
    const uintptr_t line_mask = (uintptr_t)EMBEDDIP_H7S_CACHE_LINE_BYTES - 1u;
    uintptr_t start;
    uintptr_t end;
    uintptr_t rounded_end;
    uintptr_t span;

    if (address == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (size == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    start = (uintptr_t)address;
    if (size > UINTPTR_MAX - start) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }
    end = start + size;
    if (end > UINTPTR_MAX - line_mask) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }

    *rounded_start = start & ~line_mask;
    rounded_end = (end + line_mask) & ~line_mask;
    span = rounded_end - *rounded_start;
    if (span > INT32_MAX) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }

    *rounded_size = (int32_t)span;
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_board_cache_clean(const void *address, size_t size)
{
    uintptr_t rounded_start;
    int32_t rounded_size;
    embeddip_status_t status = h7s_cache_span(address, size, &rounded_start, &rounded_size);

    if (status != EMBEDDIP_OK) {
        return status;
    }
    SCB_CleanDCache_by_Addr((void *)rounded_start, rounded_size);
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_board_cache_invalidate(const void *address, size_t size)
{
    uintptr_t rounded_start;
    int32_t rounded_size;
    embeddip_status_t status = h7s_cache_span(address, size, &rounded_start, &rounded_size);

    if (status != EMBEDDIP_OK) {
        return status;
    }
    SCB_InvalidateDCache_by_Addr((void *)rounded_start, rounded_size);
    return EMBEDDIP_OK;
}
```

- [ ] **Step 4: Write the failing host test**

Create `tests/test_stm32h7s_memory.c` (port of `tests/test_stm32n6_memory.c`; note the include is `board/stm32h7s/board_stm32h7s_memory.c` and the tested internal function is `h7s_allocate_region`):

```c
#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "core/memory_manager.h"

alignas(32) uint8_t fast_storage[96];
alignas(32) uint8_t dma_storage[64];
alignas(32) uint8_t psram_storage[48];
alignas(32) uint8_t flash_storage[32];

__asm__(".globl __embeddip_fast_sram_start__\n"
        ".set __embeddip_fast_sram_start__, fast_storage\n"
        ".globl __embeddip_fast_sram_end__\n"
        ".set __embeddip_fast_sram_end__, fast_storage + 96\n"
        ".globl __embeddip_dma_start__\n"
        ".set __embeddip_dma_start__, dma_storage\n"
        ".globl __embeddip_dma_end__\n"
        ".set __embeddip_dma_end__, dma_storage + 64\n"
        ".globl __embeddip_psram_start__\n"
        ".set __embeddip_psram_start__, psram_storage\n"
        ".globl __embeddip_psram_end__\n"
        ".set __embeddip_psram_end__, psram_storage + 48\n"
        ".globl __embeddip_xspi_flash_start__\n"
        ".set __embeddip_xspi_flash_start__, flash_storage\n"
        ".globl __embeddip_xspi_flash_end__\n"
        ".set __embeddip_xspi_flash_end__, flash_storage + 32\n");

static void *last_clean_address;
static int32_t last_clean_size;
static void *last_invalidate_address;
static int32_t last_invalidate_size;

void SCB_CleanDCache_by_Addr(void *address, int32_t size)
{
    last_clean_address = address;
    last_clean_size = size;
}

void SCB_InvalidateDCache_by_Addr(void *address, int32_t size)
{
    last_invalidate_address = address;
    last_invalidate_size = size;
}

#include "board/stm32h7s/board_stm32h7s_memory.c"

int main(void)
{
    alignas(32) uint8_t cache_span[96];
    void *allocation = (void *)(uintptr_t)1u;

    memory_init(0u);

    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DEFAULT, 40u, 32u, &allocation) == EMBEDDIP_OK);
    assert(allocation == fast_storage);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_FAST_SRAM, 64u, 1u, &allocation) ==
           EMBEDDIP_ERROR_OUT_OF_MEMORY);
    assert(allocation == NULL);

    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DMA, 16u, 3u, &allocation) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(allocation == NULL);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_DMA, sizeof(dma_storage), 8u, &allocation) == EMBEDDIP_OK);
    assert(allocation == dma_storage);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_PSRAM, sizeof(psram_storage), 16u, &allocation) == EMBEDDIP_OK);
    assert(allocation == psram_storage);
    assert(h7s_allocate_region(EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH, 1u, 1u, &allocation) ==
           EMBEDDIP_ERROR_NOT_SUPPORTED);
    assert(allocation == NULL);

    assert(embeddip_board_cache_clean(cache_span + 3u, 33u) == EMBEDDIP_OK);
    assert(last_clean_address == cache_span);
    assert(last_clean_size == 64);
    assert(embeddip_board_cache_invalidate(cache_span + 31u, 2u) == EMBEDDIP_OK);
    assert(last_invalidate_address == cache_span);
    assert(last_invalidate_size == 64);

    return 0;
}
```

- [ ] **Step 5: Register the test in `tests/CMakeLists.txt`**

Find the N6 memory test block (near `embeddip_test_stm32n6_memory`) and add immediately after it:

```cmake
    add_executable(embeddip_test_stm32h7s_memory test_stm32h7s_memory.c)
    target_include_directories(embeddip_test_stm32h7s_memory PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/stm32h7s
        ${CMAKE_SOURCE_DIR}/board/stm32h7s
        ${CMAKE_SOURCE_DIR})
    add_test(NAME embeddip.stm32h7s_memory COMMAND embeddip_test_stm32h7s_memory)
```

Match the exact `target_include_directories` shape used by the N6 test block (copy its include list, swap `stm32n6`→`stm32h7s`). The `${CMAKE_SOURCE_DIR}` entry lets `#include "board/stm32h7s/..."` and `#include "core/memory_manager.h"` resolve.

- [ ] **Step 6: Configure + build + run the test (expect PASS)**

Run:
```bash
cmake -S ~/book-ready/embedDIP -B ~/book-ready/embedDIP/build/host \
  -DEMBEDDIP_TARGET_BOARD=HOST -DEMBEDDIP_ARCH=HOST -DEMBEDDIP_CPU=NATIVE \
  -DEMBEDDIP_BUILD_TESTS=ON
cmake --build ~/book-ready/embedDIP/build/host --target embeddip_test_stm32h7s_memory
ctest --test-dir ~/book-ready/embedDIP/build/host -R embeddip.stm32h7s_memory --output-on-failure
```
Expected: test `embeddip.stm32h7s_memory` PASSES. (Confirm the exact `EMBEDDIP_BUILD_TESTS`/host cache flags against how N6's test is built — reuse whatever the repo's existing host+tests configure line is; check `build/host` or CI workflow if unsure.)

- [ ] **Step 7: Commit**

```bash
git add board/stm32h7s/configs.h board/stm32h7s/board_stm32h7s_memory.c \
        tests/fakes/stm32h7s/stm32h7s7xx.h tests/test_stm32h7s_memory.c tests/CMakeLists.txt
git commit -m "feat: add STM32H7S board memory allocator with host test"
```

---

### Task 2: CMake board glue + board profile + configs feature block

Wire `STM32H7S` into the build system so the board configures and compiles, and add the `embedDIP_configs.h` feature block that defines the board/arch/CPU macros and driver-gating `DEVICE_*` macros.

**Files:**
- Modify: `CMakeLists.txt` (cache STRINGS ~L19, matrix ~L48-72, SDK-root cache var ~L27)
- Create: `board/stm32h7s/board_profile.cmake`
- Modify: `embedDIP_configs.h` (arch/CPU inference, sanity counts, matrix, new feature block)
- Create: `tests/cmake/test_stm32h7s_profile.cmake`
- Modify: `tests/CMakeLists.txt` (register profile configure tests)

**Interfaces:**
- Consumes: `board/stm32h7s/board_stm32h7s_memory.c`, `board/stm32h7s/configs.h` (Task 1); the display/serial source paths it lists are created in Tasks 3–4 — the profile references them by path now, so Task 2's board build is only fully green once Tasks 3–4 land. The **profile configure test** (below) does not compile sources, so it passes at end of Task 2.
- Produces: board id string `STM32H7S`, cache var `EMBEDDIP_STM32CUBE_H7RS_ROOT`, defines `EMBED_DIP_BOARD_STM32H7S`, `DEVICE_RK050HR18`, `DEVICE_STM32H7S_UART`.

- [ ] **Step 1: Add SDK-root cache variable in `CMakeLists.txt`**

After the `EMBEDDIP_STM32CUBE_N6_ROOT` cache line (~L27) add:
```cmake
set(EMBEDDIP_STM32CUBE_H7RS_ROOT "" CACHE PATH "Path to the STM32CubeH7RS SDK root")
```

- [ ] **Step 2: Add STM32H7S to the board cache STRINGS**

At the `EMBEDDIP_TARGET_BOARD` declaration (~L18-19) add `STM32H7S` to both the doc string and the `set_property(... STRINGS ...)` list:
```cmake
set(EMBEDDIP_TARGET_BOARD "" CACHE STRING "Target board (required): STM32F7, STM32H7S, STM32N6, ESP32, or HOST")
set_property(CACHE EMBEDDIP_TARGET_BOARD PROPERTY STRINGS "STM32F7" "STM32H7S" "STM32N6" "ESP32" "HOST")
```

- [ ] **Step 3: Add the compatibility-matrix branch in `CMakeLists.txt`**

In the `if(EMBEDDIP_TARGET_BOARD STREQUAL "STM32F7") ... endif()` chain (~L50-66), add a branch (place after the STM32F7 branch):
```cmake
elseif(EMBEDDIP_TARGET_BOARD STREQUAL "STM32H7S")
    if(EMBEDDIP_ARCH STREQUAL "ARM" AND EMBEDDIP_CPU STREQUAL "CORTEX_M7")
        set(_embeddip_pair_valid TRUE)
    endif()
```
And append to the `FATAL_ERROR` "Supported:" string (~L71): `, STM32H7S+ARM+CORTEX_M7`.

- [ ] **Step 4: Create `board/stm32h7s/board_profile.cmake`**

Modeled on `board/stm32n6/board_profile.cmake` (SDK-root guard) plus F7-style device sources:
```cmake
# Board profile: STM32H7S78-DK

if(NOT IS_DIRECTORY "${EMBEDDIP_STM32CUBE_H7RS_ROOT}")
    message(FATAL_ERROR
        "EMBEDDIP_STM32CUBE_H7RS_ROOT must name an existing STM32CubeH7RS SDK directory: "
        "'${EMBEDDIP_STM32CUBE_H7RS_ROOT}'")
endif()

set(EMBEDDIP_BOARD_SOURCES
    ${BOARD_COMMON_SOURCES}
    board/stm32h7s/board_stm32h7s_memory.c
    board/stm32h7s/configs.h
)

set(EMBEDDIP_DEVICE_SOURCES
    ${DEVICE_COMMON_SOURCES}
    device/display/stm32h7s_rk050hr18.c
    device/serial/stm32h7s_uart.c
)

set(EMBEDDIP_BOARD_DEFINES
    EMBED_DIP_BOARD_STM32H7S=1
    STM32H7S7xx
    USE_HAL_DRIVER
)

set(EMBEDDIP_BOARD_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/board/stm32h7s
    ${EMBEDDIP_STM32CUBE_H7RS_ROOT}/Drivers/CMSIS/Device/ST/STM32H7RSxx/Include
    ${EMBEDDIP_STM32CUBE_H7RS_ROOT}/Drivers/CMSIS/Core/Include
    ${EMBEDDIP_STM32CUBE_H7RS_ROOT}/Drivers/CMSIS/DSP/Include
    ${EMBEDDIP_STM32CUBE_H7RS_ROOT}/Drivers/STM32H7RSxx_HAL_Driver/Inc
)
```

- [ ] **Step 5: Update `embedDIP_configs.h` inference + sanity checks**

Make these exact edits (line numbers approximate — match on the text):

Arch inference (~L65): add H7S to the ARM branch condition:
```c
    #elif defined(EMBED_DIP_BOARD_STM32F7) || defined(EMBED_DIP_BOARD_STM32N6) || defined(EMBED_DIP_BOARD_STM32H7S)
        #define EMBED_DIP_ARCH_ARM 1
```

CPU inference (~L81-85): add an H7S branch mapping to Cortex-M7:
```c
    #elif defined(EMBED_DIP_BOARD_STM32F7) || defined(EMBED_DIP_BOARD_STM32H7S)
        #define EMBED_DIP_CPU_CORTEX_M7 1
    #elif defined(EMBED_DIP_BOARD_STM32N6)
        #define EMBED_DIP_CPU_CORTEX_M55 1
```

Board auto-select (~L52-58): add an `#elif defined(STM32H7S7xx)` branch after the N6 one:
```c
    #elif defined(STM32H7S7xx)
        #define EMBED_DIP_BOARD_STM32H7S 1
```

"No board selected" guard (~L52 top-level `#if`): add `&& !defined(EMBED_DIP_BOARD_STM32H7S)` to that condition.

Both board-count sanity checks (~L89 and ~L92): add `+ (defined(EMBED_DIP_BOARD_STM32H7S) ? 1 : 0)` to each parenthesized sum, and add `EMBED_DIP_BOARD_STM32H7S` to the "No board selected" error text.

- [ ] **Step 6: Add the H7S branch to the `embedDIP_configs.h` matrix**

After the STM32F7 matrix block (~L116-120) add:
```c
#elif defined(EMBED_DIP_BOARD_STM32H7S)
    #if !(defined(EMBED_DIP_ARCH_ARM) && defined(EMBED_DIP_CPU_CORTEX_M7))
        #error \
            "Invalid combination: EMBED_DIP_BOARD_STM32H7S requires EMBED_DIP_ARCH_ARM + EMBED_DIP_CPU_CORTEX_M7."
    #endif
```

- [ ] **Step 7: Add the H7S feature block to `embedDIP_configs.h`**

In the feature-flag chain (`#if defined(EMBED_DIP_BOARD_STM32F7) ... #elif defined(EMBED_DIP_BOARD_ESP32)`), insert an H7S `#elif` block before the ESP32 one (~L173), modeled on the F7 block:
```c
/* ============================== STM32H7S ================================== */
#elif defined(EMBED_DIP_BOARD_STM32H7S)
    #ifndef STM32H7S7xx
        #define STM32H7S7xx 1
    #endif

    #ifndef ENABLE_UART_LOGGING
        #define ENABLE_UART_LOGGING 1
    #endif
    #ifndef ENABLE_IMAGE_PROCESSING
        #define ENABLE_IMAGE_PROCESSING 1
    #endif
    #ifndef ENABLE_CAMERA_INPUT
        #define ENABLE_CAMERA_INPUT 0
    #endif
    #ifndef ENABLE_DISPLAY_OUTPUT
        #define ENABLE_DISPLAY_OUTPUT 1
    #endif

    #ifndef DEVICE_RK050HR18
        #define DEVICE_RK050HR18 1
    #endif
    #ifndef DEVICE_STM32H7S_UART
        #define DEVICE_STM32H7S_UART 1
    #endif
```
(Camera input defaulted OFF: no camera driver in this port.)

- [ ] **Step 8: Create the profile configure test `tests/cmake/test_stm32h7s_profile.cmake`**

Port `tests/cmake/test_stm32h7s_profile.cmake` from `test_stm32n6_profile.cmake`: swap `N6`→`H7S`, `CORTEX_M55`→`CORTEX_M7`, `EMBEDDIP_TARGET_BOARD=STM32N6`→`STM32H7S`, and `EMBEDDIP_STM32CUBE_N6_ROOT`→`EMBEDDIP_STM32CUBE_H7RS_ROOT` (all occurrences, including the missing-SDK diagnostic match string). Copy the file verbatim otherwise.

- [ ] **Step 9: Register the profile tests in `tests/CMakeLists.txt`**

Mirror the N6 profile-test registration block (the `embeddip.stm32n6_profile_missing` and `embeddip.stm32n6_profile` `add_test` entries, ~L80-94). Duplicate it with `n6`→`h7s`, `N6`→`H7S`, passing `-DEMBEDDIP_STM32CUBE_H7RS_ROOT=...` and pointing `-P` at `test_stm32h7s_profile.cmake`. The "profile" (non-missing) test should only be registered when `EMBEDDIP_STM32CUBE_H7RS_ROOT` is set, matching how the N6 one is gated.

- [ ] **Step 10: Verify — missing-SDK diagnostic + host suite unaffected**

Run:
```bash
# H7S with no SDK must FATAL_ERROR naming the SDK var:
cmake -S ~/book-ready/embedDIP -B /tmp/h7s-missing \
  -DEMBEDDIP_TARGET_BOARD=STM32H7S -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7 2>&1 \
  | grep -q "EMBEDDIP_STM32CUBE_H7RS_ROOT" && echo "OK: missing-SDK diagnostic"
# Invalid combo still rejected:
cmake -S ~/book-ready/embedDIP -B /tmp/h7s-bad \
  -DEMBEDDIP_TARGET_BOARD=STM32H7S -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M55 2>&1 \
  | grep -q "Invalid board/arch/cpu" && echo "OK: bad combo rejected"
# Host tests still pass:
ctest --test-dir ~/book-ready/embedDIP/build/host --output-on-failure
```
Expected: both `OK:` lines print; host suite green.

- [ ] **Step 11: Commit**

```bash
git add CMakeLists.txt board/stm32h7s/board_profile.cmake embedDIP_configs.h \
        tests/cmake/test_stm32h7s_profile.cmake tests/CMakeLists.txt
git commit -m "feat: wire STM32H7S board into build system and configs"
```

---

### Task 3: Display driver (RK050HR18 over LTDC)

Thin `display_t` wrapper over `extern LTDC_HandleTypeDef hltdc`, modeled on `device/display/stm32_rk043fn48h.c`, sized for the 800×480 panel.

**Files:**
- Create: `device/display/stm32h7s_rk050hr18.c`
- Modify: `device/display/display.h` (add extern declaration)

**Interfaces:**
- Consumes: `DEVICE_RK050HR18` macro (Task 2), `FRAME_BUFFER` from `board/stm32h7s/configs.h` (Task 1), `display_t` / `Image` / `displayColor` / `IMAGE_FORMAT_*` from `display.h` + `core/image.h`.
- Produces: `display_t stm32h7s_rk050hr18` with `.init/.deinit/.reset/.clear/.show`.

- [ ] **Step 1: Create `device/display/stm32h7s_rk050hr18.c`**

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef DEVICE_RK050HR18

    #include "board/stm32h7s/configs.h"
    #include "core/error.h"
    #include "device/display/display.h"

    #include "stm32h7rsxx_hal.h"

// LTDC handle owned/initialized by the application (STM32CubeMX).
extern LTDC_HandleTypeDef hltdc;

    #define LCD_WIDTH 800
    #define LCD_HEIGHT 480
    #define LCD_FRAMEBUFFER ((uint32_t *)(uintptr_t)FRAME_BUFFER)

static int display_init(void)
{
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)(uintptr_t)LCD_FRAMEBUFFER, LTDC_LAYER_1);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);
    return EMBEDDIP_OK;
}

static int display_deinit(void)
{
    HAL_LTDC_DeInit(&hltdc);
    return EMBEDDIP_OK;
}

static int display_reset(void)
{
    return EMBEDDIP_OK;
}

static int display_clear(displayColor color)
{
    for (uint32_t i = 0; i < (LCD_WIDTH * LCD_HEIGHT); i++) {
        LCD_FRAMEBUFFER[i] = color;
    }
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)(uintptr_t)LCD_FRAMEBUFFER, LTDC_LAYER_1);
    return EMBEDDIP_OK;
}

static int display_show(Image *inImg)
{
    if (!inImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    switch (inImg->format) {
    case IMAGE_FORMAT_RGB888:
        HAL_LTDC_SetPixelFormat(&hltdc, LTDC_PIXEL_FORMAT_RGB888, LTDC_LAYER_1);
        break;
    case IMAGE_FORMAT_RGB565:
        HAL_LTDC_SetPixelFormat(&hltdc, LTDC_PIXEL_FORMAT_RGB565, LTDC_LAYER_1);
        break;
    case IMAGE_FORMAT_GRAYSCALE:
        HAL_LTDC_SetPixelFormat(&hltdc, LTDC_PIXEL_FORMAT_L8, LTDC_LAYER_1);
        break;
    default:
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    HAL_LTDC_SetWindowSize(&hltdc, inImg->width, inImg->height, LTDC_LAYER_1);
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)(uintptr_t)inImg->pixels, LTDC_LAYER_1);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);
    return EMBEDDIP_OK;
}

display_t stm32h7s_rk050hr18 = {
    .init = display_init,
    .deinit = display_deinit,
    .reset = display_reset,
    .clear = display_clear,
    .show = display_show,
};

#endif
```

- [ ] **Step 2: Declare the driver in `device/display/display.h`**

After `extern display_t stm32_ota5180a;` (~L112) add:
```c
extern display_t stm32h7s_rk050hr18;
```

- [ ] **Step 3: Verify it compiles under the board profile**

If the CubeH7RS HAL include tree resolves (LTDC HAL headers present), configure the H7S board and compile just this translation unit:
```bash
cmake -S ~/book-ready/embedDIP -B ~/book-ready/embedDIP/build/h7s \
  -DEMBEDDIP_TARGET_BOARD=STM32H7S -DEMBEDDIP_ARCH=ARM -DEMBEDDIP_CPU=CORTEX_M7 \
  -DEMBEDDIP_STM32CUBE_H7RS_ROOT=$PWD/STM32CubeH7RS \
  -DCMAKE_TOOLCHAIN_FILE=<arm-none-eabi toolchain used for N6/F7>
cmake --build ~/book-ready/embedDIP/build/h7s 2>&1 | grep -i "stm32h7s_rk050hr18"
```
Expected: the display TU compiles with no errors. If the ARM cross-toolchain/HAL config isn't available in this environment, the deliverable is instead: file present, gated on `DEVICE_RK050HR18`, and a clean read-through review confirming every HAL symbol used (`HAL_LTDC_SetAddress/Reload/DeInit/SetPixelFormat/SetWindowSize`, `LTDC_LAYER_1`, `LTDC_PIXEL_FORMAT_*`, `LTDC_RELOAD_IMMEDIATE`) exists in `stm32h7rsxx_hal_ltdc.h`. Verify those symbols with:
```bash
grep -rhoE "HAL_LTDC_(SetAddress|Reload|DeInit|SetPixelFormat|SetWindowSize)|LTDC_PIXEL_FORMAT_(RGB888|RGB565|L8)|LTDC_RELOAD_IMMEDIATE|LTDC_LAYER_1" \
  STM32CubeH7RS/Drivers/STM32H7RSxx_HAL_Driver/Inc/stm32h7rsxx_hal_ltdc.h | sort -u
```

- [ ] **Step 4: Commit**

```bash
git add device/display/stm32h7s_rk050hr18.c device/display/display.h
git commit -m "feat: add STM32H7S RK050HR18 LTDC display driver"
```

---

### Task 4: Serial driver (UART4 VCP)

Thin `serial_t` wrapper over `extern UART_HandleTypeDef huart4`, modeled on `device/serial/stm32_uart.c`. Keep the full serial protocol (STR/STW/STJ/ST<n> framing) identical — only the handle name and HAL header change.

**Files:**
- Create: `device/serial/stm32h7s_uart.c`
- Modify: `device/serial/serial.h` (add extern branch)

**Interfaces:**
- Consumes: `DEVICE_STM32H7S_UART` macro (Task 2), `serial_t` / `Image` / `Serial1DDataType` from `serial.h`, `EMBEDDIP_*` status codes from `core/error.h`.
- Produces: `serial_t stm32h7s_uart` with `.init/.flush/.capture/.send/.sendJPEG/.send1D`.

- [ ] **Step 1: Create `device/serial/stm32h7s_uart.c`**

Port `device/serial/stm32_uart.c` verbatim with exactly two mechanical changes: gate macro `DEVICE_STM32_UART`→`DEVICE_STM32H7S_UART`; HAL include `stm32f7xx_hal.h`→`stm32h7rsxx_hal.h`; handle `huart1`→`huart4` (all occurrences); and the exported object name `stm32_uart`→`stm32h7s_uart`. Header block:
```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef DEVICE_STM32H7S_UART

    #include "core/error.h"
    #include "device/serial/serial.h"

    #include <stdarg.h>
    #include <stdio.h>

    #include "stm32h7rsxx_hal.h"
    ...
extern UART_HandleTypeDef huart4;
    ...
serial_t stm32h7s_uart = {
    .init = serial_init,
    .capture = serial_capture,
    .send = serial_send,
    .sendJPEG = serial_send_jpeg,
    .send1D = serial_send_1d,
    .flush = serial_flush,
};

#endif
```
Keep every function body byte-for-byte identical to `stm32_uart.c` except `huart1`→`huart4`. (The `HAL_UART_*` API is identical across F7 and H7RS HALs.)

- [ ] **Step 2: Declare the driver in `device/serial/serial.h`**

After the existing board-gated extern branches (~L40-46) add:
```c
#ifdef EMBED_DIP_BOARD_STM32H7S
extern serial_t stm32h7s_uart;
#endif
```

- [ ] **Step 3: Verify HAL symbols + gate**

```bash
grep -rhoE "HAL_UART_(Transmit|Receive|Abort)|__HAL_UART_(SEND_REQ|CLEAR_FLAG)|UART_CLEAR_(OREF|NEF|FEF|PEF)|UART_RXDATA_FLUSH_REQUEST" \
  STM32CubeH7RS/Drivers/STM32H7RSxx_HAL_Driver/Inc/stm32h7rsxx_hal_uart.h | sort -u
```
Expected: every symbol the driver uses is present in the H7RS UART HAL header. (If cross-toolchain is available, compile the board profile and confirm the serial TU builds.)

- [ ] **Step 4: Commit**

```bash
git add device/serial/stm32h7s_uart.c device/serial/serial.h
git commit -m "feat: add STM32H7S UART4 serial driver"
```

---

## Self-Review

**Spec coverage:**
- CMake glue (board id, matrix, SDK root, CMSIS includes) → Task 2 ✓
- `board/stm32h7s/` memory.c + configs.h + board_profile.cmake → Tasks 1, 2 ✓
- Region arena (DEFAULT/FAST_SRAM/DMA/PSRAM), SCB cache ops → Task 1 ✓
- Display driver (LTDC/RK050HR18) + header → Task 3 ✓
- Serial driver (UART4) + header → Task 4 ✓
- Camera deferred → not implemented, camera input defaulted OFF in feature block (Task 2 Step 7) ✓
- App-owned bring-up contract → drivers use `extern` handles only (Tasks 3, 4) ✓
- Testing: host memory unit test (Task 1), CMake configure/diagnostic tests (Task 2), driver symbol/compile checks (Tasks 3, 4) ✓
- Global constraint "no Co-Authored-By" → all commit messages omit it ✓

**Placeholder scan:** All code blocks are concrete. Two intentional non-literals: the arm cross-toolchain file path (Task 3 Step 3) — the repo's existing N6/F7 toolchain, not inventable here; and "reuse the repo's existing host+tests configure line" (Task 1 Step 6) — the exact flag is environment-specific and the executor confirms against N6's working invocation. Both are verification-environment details, not implementation gaps.

**Type consistency:** `h7s_allocate_region` used identically in memory.c and test. `stm32h7s_rk050hr18` / `stm32h7s_uart` object names consistent between driver, header extern, and board_profile source list. `DEVICE_RK050HR18` / `DEVICE_STM32H7S_UART` consistent between configs feature block and driver gate. `EMBEDDIP_STM32CUBE_H7RS_ROOT` consistent across CMakeLists, board_profile, and profile test. `FRAME_BUFFER` defined in configs.h (Task 1), consumed by display driver (Task 3).
