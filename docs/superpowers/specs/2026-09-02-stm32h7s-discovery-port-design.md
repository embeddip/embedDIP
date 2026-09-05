# Design: Port STM32H7S78-DK to embedDIP

Date: 2026-09-02
Status: Approved (pending spec review)

## Goal

Add the **STM32H7S78-DK** Discovery kit as a first-class board target in
embedDIP, alongside the existing STM32F7, STM32N6, ESP32, and HOST ports.
Ship memory management, build glue, and display + UART drivers. Camera is
explicitly **deferred** to a later task.

## Hardware facts (STM32H7S78-DK)

Sourced from the STM32CubeH7RS BSP, `um3289` board manual, and the example
linker scripts.

| Item | Value |
|------|-------|
| MCU | STM32H7S7L8H6H, single Cortex-M7 @600MHz, D-cache (32-byte line) |
| On-chip SRAM | AXI-SRAM base `0x24000000`; DTCM `0x20000000` (64K); ITCM `0x00000000` (64K) |
| External RAM | APS256XX Hexadeca-SPI PSRAM, 256 Mbit = 32 MB, memory-mapped at `0x90000000` (XSPI2) |
| LCD | Rocktech RK050HR18 800×480 RGB, driven over LTDC; framebuffer in PSRAM `0x90000000` |
| Camera | DCMIPP + OV5640 daughterboard on CN6 — optional, not bundled, no board BSP (DEFERRED) |
| VCP UART | UART4, PD1 (TX) / PD0 (RX) |
| SDK | `STM32CubeH7RS/` (submodules initialized) |

The Cortex-M7 core means **no new arch work**: `arch/arm/arch_profile.cmake`
already supports `CORTEX_M7` (used by STM32F7).

## Design principles (follow the existing codebase)

1. **Drivers are thin ST-HAL wrappers over `extern` handles the application
   owns.** The F7 drivers do exactly this: `extern LTDC_HandleTypeDef hltdc;`
   / `extern UART_HandleTypeDef huart1;`, initialized by the parent app, never
   by the library. H7S follows the same contract.
2. **Memory follows the N6 model, not the F7 model.** N6 (also a cached
   Cortex core with external XSPI RAM) uses a region arena over linker-defined
   symbols plus `SCB` cache maintenance. F7's absolute-SDRAM free-list model
   does not fit H7S. Reuse the N6 allocator shape.
3. **The library does not bring up peripherals.** XSPI PSRAM must already be
   in memory-mapped mode (app calls `BSP_XSPI_RAM_Init`), and LTDC/UART must
   be initialized, before the corresponding embedDIP facilities are used.
   Same precondition contract as the F7 port.

## Components

### 1. CMake glue (`CMakeLists.txt`, `arch` unchanged)

- Add `STM32H7S` to `EMBEDDIP_TARGET_BOARD` cache STRINGS.
- Add matrix entry: `STM32H7S + ARM + CORTEX_M7` → valid. Update the
  `FATAL_ERROR` "Supported:" string.
- Add cache var `EMBEDDIP_STM32CUBE_H7RS_ROOT` (PATH), mirroring
  `EMBEDDIP_STM32CUBE_N6_ROOT`.
- Board profile supplies CMSIS include dirs from the SDK root.

### 2. `board/stm32h7s/`

Copy the N6 board directory shape.

- **`board_profile.cmake`** — validate `EMBEDDIP_STM32CUBE_H7RS_ROOT` exists;
  set `EMBEDDIP_BOARD_SOURCES` (memory.c + configs.h), `EMBEDDIP_DEVICE_SOURCES`
  (display + serial), `EMBEDDIP_BOARD_DEFINES`
  (`EMBED_DIP_BOARD_STM32H7S=1`, `STM32H7S7xx` — the part macro that selects
  the CMSIS device header `stm32h7s7xx.h`; add `USE_HAL_DRIVER` if the app
  relies on the profile for it), and
  `EMBEDDIP_BOARD_INCLUDE_DIRS` (board dir + CMSIS Device/Core/DSP under the
  SDK root).
- **`board_stm32h7s_memory.c`** — port of `board_stm32n6_memory.c`:
  - Regions: `DEFAULT`/`FAST_SRAM` → AXI-SRAM; `DMA` → AXI-SRAM;
    `PSRAM` → APS256 (`0x90000000`, 32 MB); `EXTERNAL_FLASH` → not supported.
  - Bump-pointer arena per region over linker symbols.
  - `embeddip_board_cache_clean` / `_invalidate` via `SCB_CleanDCache_by_Addr`
    / `SCB_InvalidateDCache_by_Addr`, 32-byte line rounding (identical to N6).
  - `memory_free` is a no-op; `memory_realloc` only grows via fresh alloc
    (matches N6).
- **`configs.h`** — `EMBEDDIP_H7S_CACHE_LINE_BYTES 32u`; extern region symbols
  (`__embeddip_fast_sram_start__/_end__`, `__embeddip_dma_*`,
  `__embeddip_psram_*`); framebuffer address macro `= 0x90000000`.
  The application's linker script defines the region symbols (as on N6).

### 3. Drivers (`device/`, gated on `EMBED_DIP_BOARD_STM32H7S`)

- **`device/display/stm32h7s_rk050hr18.c`** → `display_t stm32h7s_rk050hr18`.
  Wraps `extern LTDC_HandleTypeDef hltdc;` with `HAL_LTDC_*` (SetAddress,
  SetPixelFormat, SetWindowSize, Reload) for the 800×480 panel. Model on
  `device/display/stm32_rk043fn48h.c`.
- **`device/serial/stm32h7s_uart.c`** → `serial_t stm32h7s_uart`. Wraps
  `extern UART_HandleTypeDef huart4;` with `HAL_UART_*`; provides `_write`
  retarget. Model on `device/serial/stm32_uart.c`.
- Header updates: add `extern display_t stm32h7s_rk050hr18;` to `display.h`;
  add an `EMBED_DIP_BOARD_STM32H7S` `extern serial_t stm32h7s_uart;` branch to
  `serial.h`.

### 4. Camera (DEFERRED — not in this port)

No camera source. Documented as a follow-up task: DCMIPP capture pipeline +
OV5640 I2C configuration, requiring the optional CN6 daughterboard. The N6
port already ships with no camera, so an H7S board with display + serial only
is consistent.

## Testing

- **Host build unaffected** — no change to HOST/host paths.
- **CMake configure test:** `STM32H7S + ARM + CORTEX_M7` configures with a
  valid `EMBEDDIP_STM32CUBE_H7RS_ROOT`; invalid combos still `FATAL_ERROR`.
- **Memory allocator unit test** (host, mirrors `tests/test_stm32n6_memory.c`):
  fake linker symbols back each region; assert region arena bump/alignment,
  out-of-memory boundary, and cache-span rounding. This is the one non-trivial
  logic path and gets a runnable check.
- Drivers are HAL passthroughs (no testable logic on host); verified by cross
  build only.

## Out of scope

- Camera capture (deferred, own task).
- XSPI/LTDC/UART bring-up code (owned by the application, per house contract).
- Any change to F7/N6/ESP32/HOST behavior.
