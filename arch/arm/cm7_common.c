// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#if defined(EMBED_DIP_ARCH_ARM) && defined(EMBED_DIP_CPU_CORTEX_M7)

    #include "core/image.h"

    #include <stdlib.h>
    #include <string.h>

    #include "stm32f7xx_hal.h"
    #include <board/common.h>
    #include <core/memory_manager.h>

    #define DWT_LAR_KEY 0xC5ACCE55

void tic()
{
    // Unlock access to DWT
    DWT->LAR = DWT_LAR_KEY;
    CoreDebug->DEMCR |= 0x01000000;
    DWT->CYCCNT = 0;          // reset the counter
    DWT->CTRL |= 0x00000001;  // enable the counter
}

uint32_t toc()
{
    DWT->CTRL &= 0xFFFFFFFE;  // disable the counter
    return DWT->CYCCNT;       // Return elapsed cycles
}

#endif
