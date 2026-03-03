/**
 * @file arch_fft.c
 * @brief HOST architecture FFT implementation (software Cooley-Tukey)
 *
 * This file provides a simple software FFT implementation for HOST
 * architecture using the Cooley-Tukey radix-2 algorithm.
 *
 * Note: This is not optimized for performance - it's for testing only.
 * For production use on PC, consider using FFTW or similar libraries.
 */

#include "arch/arch.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Check if n is a power of 2
 */
static int is_power_of_2(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

/**
 * @brief Bit reversal permutation
 */
static void bit_reverse(float* data, int n) {
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (i < j) {
            // Swap real parts
            float temp = data[i * 2];
            data[i * 2] = data[j * 2];
            data[j * 2] = temp;

            // Swap imaginary parts
            temp = data[i * 2 + 1];
            data[i * 2 + 1] = data[j * 2 + 1];
            data[j * 2 + 1] = temp;
        }

        int m = n >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}

/**
 * @brief 1D FFT using Cooley-Tukey radix-2 algorithm
 *
 * @param data Input/output array (interleaved real/imag: [r0,i0,r1,i1,...])
 * @param n Number of complex samples
 * @param inverse 0 for forward FFT, 1 for inverse FFT
 * @return 0 on success, -1 on error
 */
static int fft_1d(float* data, int n, int inverse) {
    if (!is_power_of_2(n)) {
        return -1;  // Only power-of-2 sizes supported
    }

    // Bit reversal permutation
    bit_reverse(data, n);

    // Cooley-Tukey FFT
    int direction = inverse ? 1 : -1;

    for (int s = 1; s <= (int)(log2(n)); s++) {
        int m = 1 << s;  // 2^s
        int m2 = m >> 1;

        // Compute twiddle factor for this stage
        float theta = direction * 2.0f * M_PI / m;
        float wm_real = cosf(theta);
        float wm_imag = sinf(theta);

        for (int k = 0; k < n; k += m) {
            float w_real = 1.0f;
            float w_imag = 0.0f;

            for (int j = 0; j < m2; j++) {
                // Butterfly operation
                int t_idx = (k + j + m2) * 2;
                int u_idx = (k + j) * 2;

                float t_real = w_real * data[t_idx] - w_imag * data[t_idx + 1];
                float t_imag = w_real * data[t_idx + 1] + w_imag * data[t_idx];

                float u_real = data[u_idx];
                float u_imag = data[u_idx + 1];

                // Update
                data[u_idx] = u_real + t_real;
                data[u_idx + 1] = u_imag + t_imag;
                data[t_idx] = u_real - t_real;
                data[t_idx + 1] = u_imag - t_imag;

                // Update twiddle factor
                float w_temp = w_real;
                w_real = w_real * wm_real - w_imag * wm_imag;
                w_imag = w_temp * wm_imag + w_imag * wm_real;
            }
        }
    }

    // Scale for inverse FFT
    if (inverse) {
        float scale = 1.0f / n;
        for (int i = 0; i < n * 2; i++) {
            data[i] *= scale;
        }
    }

    return 0;
}

// ============================================================================
// Architecture Interface Implementation
// ============================================================================

/**
 * @brief Initialize architecture-specific FFT (no-op for software implementation)
 */
void arch_fft_init(void) {
    // Nothing to initialize for software FFT
}

/**
 * @brief Perform 2D FFT
 *
 * @param input Input image (real values)
 * @param output Output spectrum (interleaved real/imag)
 * @param width Image width (must be power of 2)
 * @param height Image height (must be power of 2)
 * @return 0 on success, -1 on error
 */
int arch_fft_2d(const float* input, float* output, int width, int height) {
    if (!input || !output || width <= 0 || height <= 0) {
        return -1;
    }

    if (!is_power_of_2(width) || !is_power_of_2(height)) {
        return -1;  // Only power-of-2 sizes supported
    }

    // Allocate temporary buffer for row FFTs
    float* temp = (float*)malloc(width * height * 2 * sizeof(float));
    if (!temp) {
        return -1;
    }

    // Copy input to temp (convert real to complex)
    for (int i = 0; i < width * height; i++) {
        temp[i * 2] = input[i];      // Real part
        temp[i * 2 + 1] = 0.0f;      // Imaginary part
    }

    // FFT on rows
    for (int y = 0; y < height; y++) {
        if (fft_1d(&temp[y * width * 2], width, 0) != 0) {
            free(temp);
            return -1;
        }
    }

    // Transpose: temp -> output
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int src_idx = (y * width + x) * 2;
            int dst_idx = (x * height + y) * 2;
            output[dst_idx] = temp[src_idx];
            output[dst_idx + 1] = temp[src_idx + 1];
        }
    }

    // FFT on columns (which are now rows in output)
    for (int x = 0; x < width; x++) {
        if (fft_1d(&output[x * height * 2], height, 0) != 0) {
            free(temp);
            return -1;
        }
    }

    // Transpose back: output -> temp
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int src_idx = (x * height + y) * 2;
            int dst_idx = (y * width + x) * 2;
            temp[dst_idx] = output[src_idx];
            temp[dst_idx + 1] = output[src_idx + 1];
        }
    }

    // Copy result back to output
    memcpy(output, temp, width * height * 2 * sizeof(float));

    free(temp);
    return 0;
}

/**
 * @brief Perform 2D inverse FFT
 *
 * @param input Input spectrum (interleaved real/imag)
 * @param output Output image (real values)
 * @param width Image width (must be power of 2)
 * @param height Image height (must be power of 2)
 * @return 0 on success, -1 on error
 */
int arch_ifft_2d(const float* input, float* output, int width, int height) {
    if (!input || !output || width <= 0 || height <= 0) {
        return -1;
    }

    if (!is_power_of_2(width) || !is_power_of_2(height)) {
        return -1;
    }

    // Allocate temporary buffer
    float* temp = (float*)malloc(width * height * 2 * sizeof(float));
    if (!temp) {
        return -1;
    }

    // Copy input
    memcpy(temp, input, width * height * 2 * sizeof(float));

    // IFFT on rows
    for (int y = 0; y < height; y++) {
        if (fft_1d(&temp[y * width * 2], width, 1) != 0) {
            free(temp);
            return -1;
        }
    }

    // Transpose
    float* temp2 = (float*)malloc(width * height * 2 * sizeof(float));
    if (!temp2) {
        free(temp);
        return -1;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int src_idx = (y * width + x) * 2;
            int dst_idx = (x * height + y) * 2;
            temp2[dst_idx] = temp[src_idx];
            temp2[dst_idx + 1] = temp[src_idx + 1];
        }
    }

    // IFFT on columns
    for (int x = 0; x < width; x++) {
        if (fft_1d(&temp2[x * height * 2], height, 1) != 0) {
            free(temp);
            free(temp2);
            return -1;
        }
    }

    // Transpose back and extract real part
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int src_idx = (x * height + y) * 2;
            int dst_idx = y * width + x;
            output[dst_idx] = temp2[src_idx];  // Only take real part
        }
    }

    free(temp);
    free(temp2);
    return 0;
}

/**
 * @brief Get architecture information
 */
const char* arch_get_name(void) {
    return "HOST (x86/x64 software)";
}

/**
 * @brief Initialize architecture
 */
void arch_init(void) {
    // Nothing to initialize for HOST
}

/**
 * @brief Get architecture capabilities
 */
uint32_t arch_get_capabilities(void) {
    return ARCH_CAP_FPU | ARCH_CAP_CACHE;  // HOST has FPU and cache
}
