#ifndef DIP_H
#define DIP_H

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*negative)(const Image* in, Image* out);
    // Extend with more: threshold, blur, edge detection, etc.
} dip_t;

extern dip_t dip_v0;

#ifdef __cplusplus
}
#endif

#endif // DIP_H
