#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stddef.h>
#include "image.h" // Assume `Image*` is defined here

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct display_interface
    {
        int (*init)(void);
        int (*show)(Image *inImg);
    } display_t;

    // External declaration of STM32 implementation
    extern display_t stm32_rk043fn48h;

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
