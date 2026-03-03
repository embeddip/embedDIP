/**
 * @file arch_fft.c
 * @brief ARM Cortex-M7 optimized FFT implementation
 *
 * This implementation uses ARM CMSIS-DSP library for hardware-accelerated
 * FFT operations. It works on ANY ARM Cortex-M7 board.
 *
 * CMSIS-DSP provides optimized FFT routines that utilize:
 * - ARM DSP instructions
 * - Hardware FPU (Floating Point Unit)
 * - Optimized memory access patterns
 *
 * @note This code has NO board-specific dependencies
 * @note Works on: STM32F7, STM32H7, NXP i.MX RT1060, etc.
 */

#include "arch/arch.h"
#include "arch_config.h"

#ifdef ARCH_ARM_CORTEX_M7

#include "arm_math.h"
#include "arm_const_structs.h"
#include <string.h>
#include <math.h>

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Check if FFT size is valid
 * @param width Image width
 * @param height Image height
 * @return 1 if valid, 0 otherwise
 */
static int is_valid_fft_size(int width, int height) {
    // Must be square
    if (width != height) return 0;

    // Must be power of 2
    if ((width & (width - 1)) != 0) return 0;

    // Must be within supported range
    if (width < ARCH_MIN_FFT_SIZE || width > ARCH_MAX_FFT_SIZE) return 0;

    return 1;
}

/**
 * @brief Get CMSIS-DSP FFT instance for given size
 * @param size FFT size (must be power of 2)
 * @return Pointer to FFT instance, or NULL if unsupported
 */
static const arm_cfft_instance_f32* get_fft_instance(int size) {
    switch (size) {
        case 16:   return &arm_cfft_sR_f32_len16;
        case 32:   return &arm_cfft_sR_f32_len32;
        case 64:   return &arm_cfft_sR_f32_len64;
        case 128:  return &arm_cfft_sR_f32_len128;
        case 256:  return &arm_cfft_sR_f32_len256;
        case 512:  return &arm_cfft_sR_f32_len512;
        case 1024: return &arm_cfft_sR_f32_len1024;
        case 2048: return &arm_cfft_sR_f32_len2048;
        case 4096: return &arm_cfft_sR_f32_len4096;
        default:   return NULL;
    }
}

// ============================================================================
// Public API Implementation
// ============================================================================

int arch_fft_2d(const float* input, float* output, int width, int height) {
    // Validate inputs
    if (!input || !output) {
        return -1;  // NULL pointer
    }

    if (!is_valid_fft_size(width, height)) {
        return -2;  // Invalid size
    }

    // Get CMSIS-DSP FFT instance
    const arm_cfft_instance_f32* fft_inst = get_fft_instance(width);
    if (!fft_inst) {
        return -3;  // Unsupported FFT size
    }

    int N = width;

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
        arm_cfft_f32(fft_inst, output + row * N * 2, 0, 1);
        // Parameters: instance, buffer, ifftFlag=0 (forward), bitReverseFlag=1
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
        arm_cfft_f32(fft_inst, temp + row * N * 2, 0, 1);
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

    // Get CMSIS-DSP FFT instance
    const arm_cfft_instance_f32* fft_inst = get_fft_instance(width);
    if (!fft_inst) {
        return -3;  // Unsupported FFT size
    }

    int N = width;

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
    for (int row = 0; row < N; row++) {
        arm_cfft_f32(fft_inst, temp1 + row * N * 2, 1, 1);
        // ifftFlag=1 for inverse transform
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
        arm_cfft_f32(fft_inst, temp2 + row * N * 2, 1, 1);
    }

    // Step 4: Transpose back and extract real part
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (x * N + y);
            output[y * N + x] = temp2[src];  // Take real part only
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
    // Enable FPU if not already enabled
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  // Set CP10 and CP11 Full Access
    #endif

    // Enable instruction cache
    #if defined(ARCH_HAS_CACHE) && ARCH_HAS_CACHE
        SCB_EnableICache();
    #endif

    // Enable data cache
    #if defined(ARCH_HAS_CACHE) && ARCH_HAS_CACHE
        SCB_EnableDCache();
    #endif

    // CMSIS-DSP library is now ready to use
}

#endif // ARCH_ARM_CORTEX_M7
