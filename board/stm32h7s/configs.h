// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_STM32H7S_CONFIGS_H
#define EMBEDDIP_STM32H7S_CONFIGS_H

#include <stdint.h>

#define EMBEDDIP_H7S_CACHE_LINE_BYTES 32u

// APS256XX PSRAM, memory-mapped via XSPI2. Used as the LTDC framebuffer base.
#define FRAME_BUFFER 0x90000000u

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
