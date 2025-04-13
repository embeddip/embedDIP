#include "filter.h"
#include <memory_manager.h>

// Internal helper — filters only one channel
uint8_t channel_mask[] = {
    0xFE,
    0xFD,
    0xFB,
    0xF7,
};

void filter2D_single_channel(Image *inImg, Image *outImg, int ch_idx, void *ctx)
{

#ifdef ARM_CMSIS_DSP // TODO check if it is working or not

    Filter2DContext *context = (Filter2DContext *)ctx;
    const int size = context->size;
    const int half = size / 2;
    const int width = inImg->width;
    const int height = inImg->height;
    const int padded_width = width + 2 * half;
    const int padded_height = height + 2 * half;
    const int padded_size = padded_width * padded_height;

    // Prepare input channel
    if (!inImg->chals)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(inImg->chals, 0, sizeof(channels_t));
    }

    if (!(inImg->is_chals & channel_mask[ch_idx]))
    {
        inImg->chals->ch[ch_idx] = (float *)memory_alloc(sizeof(float) * width * height);
        float *inCh = inImg->chals->ch[ch_idx];
        const uint8_t *raw = (const uint8_t *)inImg->pixels;

        if (inImg->format == IMAGE_FORMAT_GRAYSCALE)
        {
            for (int i = 0; i < width * height; ++i)
                inCh[i] = (float)raw[i];
        }
        else if (inImg->format == IMAGE_FORMAT_RGB888)
        {
            for (int i = 0; i < width * height; ++i)
                inCh[i] = (float)raw[i * 3 + ch_idx - 1];
        }

        inImg->is_chals |= channel_mask[ch_idx];
    }

    float *inCh = inImg->chals->ch[ch_idx];

    // Pad the input image
    float *padded = (float *)memory_alloc(sizeof(float) * padded_size);
    memset(padded, 0, sizeof(float) * padded_size);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            padded[(y + half) * padded_width + (x + half)] = inCh[y * width + x];

    // Prepare output channel
    if (!outImg->chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outImg->chals, 0, sizeof(channels_t));
    }

    if (!(outImg->is_chals & channel_mask[ch_idx]))
    {
        outImg->chals->ch[ch_idx] = (float *)memory_alloc(sizeof(float) * width * height);
        outImg->is_chals |= channel_mask[ch_idx];
    }

    float *outCh = outImg->chals->ch[ch_idx];

    // Prepare CMSIS matrices
    arm_matrix_instance_f32 input_mat, kernel_mat, output_mat;
    arm_mat_init_f32(&input_mat, padded_height, padded_width, padded);
    arm_mat_init_f32(&kernel_mat, size, size, (float *)context->kernel);
    arm_mat_init_f32(&output_mat, height, width, outCh);

    // Perform 2D convolution
    arm_conv2d_f32(&input_mat, &kernel_mat, &output_mat, PADDING_VALID);

    return;

#endif

    Filter2DContext *context = (Filter2DContext *)ctx;
    int size = context->size;
    float (*filter)[size] = (float (*)[size])context->kernel;

    int width = inImg->width;
    int height = inImg->height;
    int half = size / 2;

    float *inCh = NULL;

    if (inImg->chals == NULL)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        inImg->chals->ch[0] = NULL;
        inImg->chals->ch[1] = NULL;
        inImg->chals->ch[2] = NULL;
        inImg->chals->ch[3] = NULL;
    }

    // Allocate float input if necessary
    if (inImg->is_chals & ~(channel_mask[ch_idx]))
    {
        inCh = inImg->chals->ch[ch_idx];
    }
    else
    {
        inImg->chals->ch[ch_idx] = (float *)memory_alloc(height * width * BYTES_PER_PIXEL);
        inCh = inImg->chals->ch[ch_idx];
        inImg->is_chals = inImg->is_chals | ~(channel_mask[ch_idx]);

        assert(inCh);
        const uint8_t *raw = (const uint8_t *)inImg->pixels;

        if (inImg->format == IMAGE_FORMAT_GRAYSCALE)
        {
            for (int i = 0; i < (int)inImg->size; ++i)
                inCh[i] = (float)raw[i];
        }
        else if (inImg->format == IMAGE_FORMAT_RGB888)
        {
            for (int i = 0; i < (int)inImg->size; ++i)
                inCh[i] = (float)raw[i * 3 + ch_idx - 1]; // ch[1]=R, ch[2]=G, ch[3]=B
        }
    }

    float *outCh = NULL;

    if (outImg->chals == NULL)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        outImg->chals->ch[0] = NULL;
        outImg->chals->ch[1] = NULL;
        outImg->chals->ch[2] = NULL;
        outImg->chals->ch[3] = NULL;
    }

    if (outImg->is_chals & ~(channel_mask[ch_idx]))
    {
        outCh = outImg->chals->ch[ch_idx];
    }
    else
    {
        outImg->chals->ch[ch_idx] = (float *)memory_alloc(height * width * BYTES_PER_PIXEL);
        outImg->is_chals = outImg->is_chals | ~(channel_mask[ch_idx]);
        outCh = outImg->chals->ch[ch_idx];
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float sum = 0.0f;

            for (int fy = -half; fy <= half; ++fy)
            {
                for (int fx = -half; fx <= half; ++fx)
                {
                    int iy = y + fy;
                    int ix = x + fx;

                    float val = 0.0f;
                    if (iy >= 0 && iy < height && ix >= 0 && ix < width)
                        val = inCh[iy * width + ix];

                    sum += val * filter[fy + half][fx + half];
                }
            }

            outCh[y * width + x] = sum;
        }
    }
}

// TODO will continue from there

void DIP_sepFilter2D(const Image *inImg, Image *outImg, int kernelSizeX, float kernelX[kernelSizeX],
                     int kernelSizeY, float kernelY[kernelSizeY], double delta)
{

    // Temporary image to store the result after the first pass (horizontal filtering)
    Image *tempImg = (Image *)createImage(IMAGE_RES_WQVGA,
                                          inImg->format);

    // First pass: filter rows with kernelX (horizontal filtering)
    for (int y = 0; y < inImg->height; y++)
    {
        for (int x = 0; x < inImg->width; x++)
        {
            float sum = 0.0;
            for (int fx = -kernelSizeX / 2; fx <= kernelSizeX / 2; ++fx)
            {
                int pixelValue = 0;
                int xIndex = x + fx;

                // Handle borders by ignoring pixels outside bounds (border handling)
                if (xIndex < 0 || xIndex >= inImg->width)
                {
                    pixelValue = 0;
                }
                else
                {
                    // pixelValue = inImg->pixels[y * inImg->width + xIndex];
                }

                sum += ((float)pixelValue * kernelX[fx + kernelSizeX / 2]);
            }

            sum = sum + delta;                           // Add delta after the filter operation
            sum = sum < 0 ? 0 : (sum > 255 ? 255 : sum); // Normalize to [0, 255]
            // tempImg->pixels[y * tempImg->width + x] = (uint8_t)sum;
        }
    }

    // Second pass: filter columns with kernelY (vertical filtering)
    for (int y = 0; y < outImg->height; y++)
    {
        for (int x = 0; x < outImg->width; x++)
        {
            float sum = 0.0;
            for (int fy = -kernelSizeY / 2; fy <= kernelSizeY / 2; ++fy)
            {
                int pixelValue = 0;
                int yIndex = y + fy;

                // Handle borders by ignoring pixels outside bounds (border handling)
                if (yIndex < 0 || yIndex >= tempImg->height)
                {
                    pixelValue = 0;
                }
                else
                {
                    // pixelValue = tempImg->pixels[yIndex * tempImg->width + x];
                }

                sum += ((float)pixelValue * kernelY[fy + kernelSizeY / 2]);
            }

            sum = sum + delta;                           // Add delta after the filter operation
            sum = sum < 0 ? 0 : (sum > 255 ? 255 : sum); // Normalize to [0, 255]
            // outImg->pixels[y * outImg->width + x] = (uint8_t)sum;
        }
    }
}

/*
void wrapper(ImageOpFunc func, Image *inImg, Image *outImg, void *context)
{
    assert(func && inImg && outImg);
    assert(inImg->format == outImg->format);

    // Ensure channels are allocated for input
    if (!inImg->is_chals)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        inImg->is_chals = true;
        for (int i = 0; i < 4; ++i)
            inImg->chals->ch[i] = NULL;
    }

    // Ensure channels are allocated for output
    if (!outImg->is_chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t)F);
        outImg->is_chals = true;
        for (int i = 0; i < 4; ++i)
            outImg->chals->ch[i] = NULL;
    }

    // Dispatch per format
    if (inImg->format == IMAGE_FORMAT_GRAYSCALE)
    {
        func(inImg, outImg, 0, context); // l channel
    }
    else if (inImg->format == IMAGE_FORMAT_RGB888)
    {
        for (int ch = 1; ch <= 3; ++ch) // r=1, g=2, b=3
            func(inImg, outImg, ch, context);
    }
    else
    {
        assert(false && "Unsupported format in wrapper");
    }
}
    */
