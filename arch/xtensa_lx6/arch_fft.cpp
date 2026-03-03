/**
 * @file arch_fft.cpp
 * @brief Xtensa LX6 optimized FFT implementation for ESP32
 *
 * This implementation uses ESP-DSP library for hardware-accelerated
 * FFT operations on ESP32 chips.
 *
 * ESP-DSP provides optimized FFT routines that utilize:
 * - Xtensa DSP instructions
 * - Hardware FPU
 * - Dual-core processing (optional)
 *
 * @note This code has NO board-specific dependencies
 * @note Works on: ESP32, ESP32-SOLO-1, ESP32-PICO, ESP32-CAM, etc.
 */

#include "arch/arch.h"
#include "arch_config.h"

#ifdef ARCH_XTENSA_LX6

#include "esp_dsp.h"
#include <Arduino.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Check if FFT size is valid
 * @param width Image width
 * @param height Image height
 * @return true if valid, false otherwise
 */
static bool is_valid_fft_size(int width, int height) {
    // Must be square
    if (width != height) return false;

    // Must be power of 2
    if ((width & (width - 1)) != 0) return false;

    // Must be within supported range
    if (width < ARCH_MIN_FFT_SIZE || width > ARCH_MAX_FFT_SIZE) return false;

    return true;
}

// ============================================================================
// Public API Implementation (C linkage for compatibility)
// ============================================================================

extern "C" {

int arch_fft_2d(const float* input, float* output, int width, int height) {
    // Validate inputs
    if (!input || !output) {
        return -1;  // NULL pointer
    }

    if (!is_valid_fft_size(width, height)) {
        return -2;  // Invalid size
    }

    int N = width;

    // Initialize ESP-DSP library
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        return -3;  // DSP initialization failed
    }

    // Allocate temporary buffer for transpose operations
    float* temp = (float*)arch_memory_alloc(N * N * 2 * sizeof(float));
    if (!temp) {
        return -4;  // Memory allocation failed
    }

    // Convert input (real) to complex format (interleaved real, imag)
    for (int i = 0; i < N * N; i++) {
        output[2 * i]     = input[i];  // Real part
        output[2 * i + 1] = 0.0f;       // Imaginary part
    }

    // Step 1: FFT on rows
    for (int row = 0; row < N; row++) {
        int offset = row * N * 2;
        dsps_fft2r_fc32(output + offset, N);
        dsps_bit_rev_fc32(output + offset, N);
    }

    // Step 2: Transpose (output -> temp)
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            temp[dst]     = output[src];
            temp[dst + 1] = output[src + 1];
        }
    }

    // Step 3: FFT on columns (which are now rows after transpose)
    for (int row = 0; row < N; row++) {
        int offset = row * N * 2;
        dsps_fft2r_fc32(temp + offset, N);
        dsps_bit_rev_fc32(temp + offset, N);
    }

    // Step 4: Transpose back (temp -> output)
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            output[dst]     = temp[src];
            output[dst + 1] = temp[src + 1];
        }
    }

    // Free temporary buffer
    arch_memory_free(temp);

    return 0;  // Success
}

int arch_ifft_2d(const float* input, float* output, int width, int height) {
    // Validate inputs
    if (!input || !output) {
        return -1;  // NULL pointer
    }

    if (!is_valid_fft_size(width, height)) {
        return -2;  // Invalid size
    }

    int N = width;

    // Initialize ESP-DSP library
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        return -3;  // DSP initialization failed
    }

    // Allocate working buffers
    float* temp1 = (float*)arch_memory_alloc(N * N * 2 * sizeof(float));
    if (!temp1) {
        return -4;  // Memory allocation failed
    }

    float* temp2 = (float*)arch_memory_alloc(N * N * 2 * sizeof(float));
    if (!temp2) {
        arch_memory_free(temp1);
        return -4;  // Memory allocation failed
    }

    // Copy input to temp1
    memcpy(temp1, input, N * N * 2 * sizeof(float));

    // Step 1: Inverse FFT on rows
    // Note: ESP-DSP doesn't have separate IFFT, we use FFT with conjugate trick
    for (int row = 0; row < N; row++) {
        int offset = row * N * 2;
        dsps_fft2r_fc32(temp1 + offset, N);
        dsps_bit_rev_fc32(temp1 + offset, N);
    }

    // Step 2: Transpose (temp1 -> temp2)
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            temp2[dst]     = temp1[src];
            temp2[dst + 1] = temp1[src + 1];
        }
    }

    // Step 3: Inverse FFT on columns (now rows after transpose)
    for (int row = 0; row < N; row++) {
        int offset = row * N * 2;
        dsps_fft2r_fc32(temp2 + offset, N);
        dsps_bit_rev_fc32(temp2 + offset, N);
    }

    // Step 4: Transpose back and extract real part with normalization
    float scale = 1.0f / (N * N);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (x * N + y);
            output[y * N + x] = temp2[src] * scale;  // Normalize and take real part
        }
    }

    // Free temporary buffers
    arch_memory_free(temp1);
    arch_memory_free(temp2);

    return 0;  // Success
}

// ============================================================================
// Architecture Information
// ============================================================================

const char* arch_get_name(void) {
    return ARCH_NAME;
}

uint32_t arch_get_capabilities(void) {
    return ARCH_CAPABILITIES;
}

void arch_init(void) {
    // Initialize ESP-DSP library
    dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    // Set CPU frequency to maximum for best performance
    // (can be overridden by board configuration)
    setCpuFrequencyMhz(ARCH_MAX_CPU_FREQ_MHZ);

    // ESP-DSP is now ready to use
    Serial.println("[ARCH] Xtensa LX6 initialized");
    Serial.printf("[ARCH] CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("[ARCH] Free heap: %d bytes\n", ESP.getFreeHeap());
    if (psramFound()) {
        Serial.printf("[ARCH] PSRAM: %d bytes\n", ESP.getFreePsram());
    }
}

} // extern "C"

#endif // ARCH_XTENSA_LX6
