// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_ARCH_FFT_BACKEND_H
#define EMBEDDIP_ARCH_FFT_BACKEND_H

#include <core/error.h>

#ifdef __cplusplus
extern "C" {
#endif

embeddip_status_t embeddip_fft_backend_init(int n);
embeddip_status_t embeddip_fft_backend_forward_1d(float *data, int n);
embeddip_status_t embeddip_fft_backend_inverse_1d(float *data, int n);

#ifdef __cplusplus
}
#endif

#endif
