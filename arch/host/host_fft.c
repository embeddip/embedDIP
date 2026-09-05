// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#if defined(EMBED_DIP_ARCH_HOST)

#include <math.h>
#include <stddef.h>

#include <arch/fft_backend.h>

/*
 * Portable host FFT backend: naive O(n^2) DFT over interleaved complex floats
 * (data[2*i] = real, data[2*i+1] = imag). Matches the CMSIS-DSP contract,
 * including the 1/n scaling applied on the inverse transform. This exists so
 * the C++ Image wrapper links and runs on host; it is not tuned for speed.
 * ponytail: O(n^2) DFT, swap for an FFT if host FFT throughput ever matters.
 */

static int host_fft_size = -1;

embeddip_status_t embeddip_fft_backend_init(int n)
{
    if (n <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    host_fft_size = n;
    return EMBEDDIP_OK;
}

static embeddip_status_t host_dft(float *data, int n, int inverse)
{
    const double sign = inverse ? 1.0 : -1.0;
    const double two_pi = 6.28318530717958647692;
    int k;

    if (data == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (n != host_fft_size) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    {
        /* O(n) stack scratch, VLA; host test sizes only. */
        double out_re[n];
        double out_im[n];
        int j;

        for (k = 0; k < n; ++k) {
            double sum_re = 0.0;
            double sum_im = 0.0;
            for (j = 0; j < n; ++j) {
                double angle = sign * two_pi * (double)k * (double)j / (double)n;
                double c = cos(angle);
                double s = sin(angle);
                double in_re = (double)data[2 * j];
                double in_im = (double)data[2 * j + 1];
                sum_re += in_re * c - in_im * s;
                sum_im += in_re * s + in_im * c;
            }
            out_re[k] = sum_re;
            out_im[k] = sum_im;
        }
        for (k = 0; k < n; ++k) {
            if (inverse) {
                out_re[k] /= (double)n;
                out_im[k] /= (double)n;
            }
            data[2 * k] = (float)out_re[k];
            data[2 * k + 1] = (float)out_im[k];
        }
    }
    return EMBEDDIP_OK;
}

embeddip_status_t embeddip_fft_backend_forward_1d(float *data, int n)
{
    return host_dft(data, n, 0);
}

embeddip_status_t embeddip_fft_backend_inverse_1d(float *data, int n)
{
    return host_dft(data, n, 1);
}

#endif
