// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef EMBED_DIP_ARCH_XTENSA

    #include <arch/fft_backend.h>

    #include "esp_dsp.h"

embeddip_status_t embeddip_fft_backend_init(int n)
{
    if (n <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    esp_err_t err = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    return (err == ESP_OK) ? EMBEDDIP_OK : EMBEDDIP_ERROR_INTERNAL;
}

embeddip_status_t embeddip_fft_backend_forward_1d(float *data, int n)
{
    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    dsps_fft2r_fc32(data, n);
    dsps_bit_rev_fc32(data, n);
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_fft_backend_inverse_1d(float *data, int n)
{
    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    for (int i = 0; i < n; ++i) {
        data[2 * i + 1] = -data[2 * i + 1];
    }

    dsps_fft2r_fc32(data, n);
    dsps_bit_rev_fc32(data, n);

    for (int i = 0; i < n; ++i) {
        data[2 * i + 1] = -data[2 * i + 1];
    }

    float inv_n = 1.0f / (float)n;
    for (int i = 0; i < n; ++i) {
        data[2 * i] *= inv_n;
        data[2 * i + 1] *= inv_n;
    }

    return EMBEDDIP_OK;
}

#endif
