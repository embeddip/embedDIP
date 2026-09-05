// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#if defined(EMBED_DIP_ARCH_ARM)

#include <arch/fft_backend.h>
#include "arm_math.h"

static arm_cfft_instance_f32 fft_instance;
static int fft_size = -1;

embeddip_status_t embeddip_fft_backend_init(int n)
{
    if (fft_size == n) {
        return EMBEDDIP_OK;
    }

    if (arm_cfft_init_f32(&fft_instance, (uint16_t)n) != ARM_MATH_SUCCESS) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    fft_size = n;
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_fft_backend_forward_1d(float *data, int n)
{
    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (n != fft_size) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    arm_cfft_f32(&fft_instance, data, 0, 1);
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_fft_backend_inverse_1d(float *data, int n)
{
    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (n != fft_size) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    arm_cfft_f32(&fft_instance, data, 1, 1);
    return EMBEDDIP_OK;
}

#endif
