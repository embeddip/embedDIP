// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#if defined(EMBED_DIP_ARCH_ARM) && defined(EMBED_DIP_CPU_CORTEX_M7)

    #include "arm_const_structs.h"
    #include "arm_math.h"
    #include <arch/fft_backend.h>

embeddip_status_t embeddip_fft_backend_init(int n)
{
    if (n != 256) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_fft_backend_forward_1d(float *data, int n)
{
    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (n != 256) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    arm_cfft_f32(&arm_cfft_sR_f32_len256, data, 0, 1);
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_fft_backend_inverse_1d(float *data, int n)
{
    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (n != 256) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    arm_cfft_f32(&arm_cfft_sR_f32_len256, data, 1, 1);
    return EMBEDDIP_OK;
}

#endif
