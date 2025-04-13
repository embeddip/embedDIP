#ifndef EMBED_DIP_H
#define EMBED_DIP_H

#ifdef __cplusplus
extern "C"
{
#endif

// =============================
// Project Version
// =============================
#define EMBED_DIP_VERSION_MAJOR 0
#define EMBED_DIP_VERSION_MINOR 0
#define EMBED_DIP_VERSION_PATCH 0

// =============================
// Feature Flags
// =============================
#define ENABLE_UART_LOGGING 1
#define ENABLE_IMAGE_PROCESSING 1
#define ENABLE_CAMERA_INPUT 1
#define ENABLE_DISPLAY_OUTPUT 1

// =============================
// Core Modules
// =============================
#include "dip.h"
#include "common.h"
#include "image.h"
#include "pixel.h"
#include "filter.h"
#include "color.h"
// =============================
// Input Modules
// =============================
#include "serial.h"
#include "camera.h"
#include "ov5640.h"
#include "fonts.h"

// =============================
// Output Modules
// =============================
#include "display.h"
#include "rk043fn48h.h"

    // Generic image operation function pointer type
    typedef void (*ImageOpFunc)(const Image *inImg, Image *outImg, int ch_idx, void *context);

    // Generic per-channel image operation wrapper
    void wrapper(ImageOpFunc func, Image *inImg, Image *outImg, void *context);

    void wrapper(ImageOpFunc func, Image *inImg, Image *outImg, void *context)
    {
        assert(func && inImg && outImg);
        assert(inImg->format == outImg->format);

        if (!inImg->is_chals)
        {
            inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
            inImg->is_chals = 0x00;
            for (int i = 0; i < 4; ++i)
                inImg->chals->ch[i] = NULL;
        }

        if (!outImg->is_chals)
        {
            outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
            outImg->is_chals = 0x00;
            for (int i = 0; i < 4; ++i)
                outImg->chals->ch[i] = NULL;
        }

        if (inImg->format == IMAGE_FORMAT_GRAYSCALE)
        {
            func(inImg, outImg, 0, context);
        }
        else if (inImg->format == IMAGE_FORMAT_RGB888)
        {
            for (int ch = 1; ch <= 3; ++ch)
                func(inImg, outImg, ch, context);
        }
        else
        {
            assert(false && "Unsupported format in wrapper");
        }
    }

    // Static inline function interfaces
    void filter2D(Image *inImg, Image *outImg, int size, float kernel[][size])
    {
        static Filter2DContext ctx;
        ctx.size = size;
        ctx.kernel = &kernel[0];
        wrapper(filter2D_single_channel, inImg, outImg, &ctx);
    }

    // TODO Fix this.
    /*
    static void sepFilter2D_wrapper(Image *inImg, Image *outImg, int ch_idx, void *ctx)
    {
        SepFilter2DContext *context = (SepFilter2DContext *)ctx;
        sepFilter2D_single_channel(inImg, outImg, ch_idx,
                                   context->kernelX, context->sizeX,
                                   context->kernelY, context->sizeY,
                                   context->delta);
    }

    static inline void sepFilter2D(Image *inImg, Image *outImg,
                                   int sizeX, float kernelX[], int sizeY, float kernelY[], float delta)
    {
        static SepFilter2DContext ctx;
        ctx.sizeX = sizeX;
        ctx.kernelX = kernelX;
        ctx.sizeY = sizeY;
        ctx.kernelY = kernelY;
        ctx.delta = delta;
        wrapper(sepFilter2D_wrapper, inImg, outImg, &ctx);
    }

    */

#ifdef __cplusplus
}
#endif

#endif // EMBED_DIP_H
