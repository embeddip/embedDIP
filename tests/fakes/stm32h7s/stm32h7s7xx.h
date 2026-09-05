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
