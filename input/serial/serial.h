#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>
#include "image.h" // Assume `Image*` is defined here

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct serial_interface
    {
        void (*init)(void);
        void (*capture)(Image *img);
        void (*send)(const Image *img);
    } serial_t;

    int _write(int file, char *ptr, int len);

    // External declaration of STM32 implementation
    extern serial_t stm32_uart;

#ifdef __cplusplus
}
#endif

#endif // SERIAL_H
