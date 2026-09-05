// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>
#include <board/common.h>

#if defined(EMBED_DIP_ARCH_ARM)

/* Include the ST device header, not the bare CMSIS core header: the device
 * header defines IRQn_Type, __NVIC_PRIO_BITS, and __DSP_PRESENT before pulling
 * in the matching core_cmXX.h. Including core_cmXX.h directly leaves those
 * undefined and fails to compile. */
#if defined(EMBED_DIP_BOARD_STM32F7)
#include "stm32f7xx.h"
#elif defined(EMBED_DIP_BOARD_STM32N6)
#include "stm32n6xx.h"
#elif defined(EMBED_DIP_CPU_CORTEX_M7)
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

#endif
