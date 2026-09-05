#ifndef TESTS_FAKES_STM32N6_STM32N6XX_H
#define TESTS_FAKES_STM32N6_STM32N6XX_H

#include <stdint.h>

void SCB_CleanDCache_by_Addr(void *address, int32_t size);
void SCB_InvalidateDCache_by_Addr(void *address, int32_t size);

#endif
