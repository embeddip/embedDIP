/**
 * @file arch.h
 * @brief Common architecture interface for embedDIP library
 *
 * This header defines the common interface that all architecture implementations
 * must provide. Architecture-specific implementations are in arch/<architecture>/
 * directories (e.g., arch/arm_cortex_m7/, arch/xtensa_lx6/).
 *
 * Architecture Layer Responsibilities:
 * - Provide CPU-optimized implementations (FFT, DSP operations)
 * - Implement memory management APIs
 * - CPU initialization and configuration
 * - Can use architecture-specific libraries (CMSIS-DSP, ESP-DSP)
 * - Must NOT contain board-specific code (pins, addresses)
 *
 * @note This interface allows the same application code to work across
 *       different CPU architectures transparently.
 */

#ifndef EMBEDDIP_ARCH_H
#define EMBEDDIP_ARCH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Memory Management Interface
// ============================================================================

/**
 * @brief Initialize architecture-specific memory subsystem
 *
 * Called once during board initialization. May configure:
 * - External RAM (SDRAM, PSRAM, etc.)
 * - Memory pools and allocators
 * - Cache settings
 * - Memory Protection Unit (MPU) if available
 *
 * Memory addresses and sizes come from board configuration.
 *
 * @note Must be called before any memory_alloc() calls
 */
void arch_memory_init(void);

/**
 * @brief Allocate memory from architecture-managed pool
 *
 * Allocates a block of memory of the specified size. The memory is
 * 4-byte aligned. Returns NULL if allocation fails.
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 *
 * @note Thread-safe if architecture supports threading
 * @note Memory pool location determined by board configuration
 */
void* arch_memory_alloc(size_t size);

/**
 * @brief Free previously allocated memory
 *
 * Frees a memory block previously allocated by arch_memory_alloc().
 * Passing NULL is safe (no-op).
 *
 * @param ptr Pointer to memory block to free
 *
 * @warning Freeing the same pointer twice results in undefined behavior
 */
void arch_memory_free(void* ptr);

/**
 * @brief Reallocate memory block to new size
 *
 * Changes the size of the memory block pointed to by ptr to new_size bytes.
 * Contents are preserved up to the minimum of old and new sizes.
 *
 * @param ptr Existing pointer (or NULL to allocate new)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 *
 * @note If ptr is NULL, behaves like arch_memory_alloc()
 * @note If reallocation fails, original block remains unchanged
 */
void* arch_memory_realloc(void* ptr, size_t new_size);

// ============================================================================
// FFT (Fast Fourier Transform) Interface
// ============================================================================

/**
 * @brief Perform 2D Fast Fourier Transform
 *
 * Transforms a 2D real-valued image from spatial domain to frequency domain
 * using CPU-optimized FFT implementation (CMSIS-DSP, ESP-DSP, or software).
 *
 * Input: Real values (spatial domain)
 * Output: Complex values interleaved as [Re0, Im0, Re1, Im1, ...]
 *
 * @param input Input buffer containing real values (width × height elements)
 * @param output Output buffer for complex values (width × height × 2 elements)
 * @param width Image width in pixels (must be power of 2)
 * @param height Image height in pixels (must be power of 2 and equal to width)
 * @return 0 on success, negative error code on failure:
 *         -1: NULL pointer
 *         -2: Invalid size (not power of 2, not square, or out of range)
 *         -3: Unsupported FFT size for this architecture
 *         -4: Memory allocation failed
 *
 * @note Supported sizes: 16, 32, 64, 128, 256, 512, 1024, 2048 (architecture-dependent)
 * @note Output size must be at least width × height × 2 × sizeof(float)
 * @note This function may allocate temporary buffers internally
 *
 * @example
 * float input[256*256];
 * float output[256*256*2];  // Complex: real, imag interleaved
 * int result = arch_fft_2d(input, output, 256, 256);
 */
int arch_fft_2d(const float* input, float* output, int width, int height);

/**
 * @brief Perform 2D Inverse Fast Fourier Transform
 *
 * Transforms a 2D complex image from frequency domain back to spatial domain.
 *
 * Input: Complex values interleaved as [Re0, Im0, Re1, Im1, ...]
 * Output: Real values (spatial domain)
 *
 * @param input Input buffer containing complex values (width × height × 2 elements)
 * @param output Output buffer for real values (width × height elements)
 * @param width Image width in pixels (must be power of 2)
 * @param height Image height in pixels (must be power of 2 and equal to width)
 * @return 0 on success, negative error code on failure (same as arch_fft_2d)
 *
 * @note Result is normalized (divided by N×N where N=width=height)
 * @note Applying FFT then IFFT should recover original input (within floating point precision)
 *
 * @example
 * float fft_data[256*256*2];  // From arch_fft_2d()
 * float output[256*256];
 * int result = arch_ifft_2d(fft_data, output, 256, 256);
 */
int arch_ifft_2d(const float* input, float* output, int width, int height);

// ============================================================================
// Architecture Initialization
// ============================================================================

/**
 * @brief Initialize architecture-specific features
 *
 * Called during system startup to configure CPU-specific features:
 * - Enable FPU (Floating Point Unit) if available
 * - Configure cache policies (instruction/data cache)
 * - Set up DSP extensions
 * - Configure clock speeds (if not handled by board layer)
 * - Enable architecture-specific optimizations
 *
 * @note Must be called before using any architecture-specific features
 * @note Board initialization calls this function automatically
 */
void arch_init(void);

/**
 * @brief Get architecture name string
 *
 * Returns a human-readable string identifying the current architecture.
 * Useful for logging, diagnostics, and runtime architecture detection.
 *
 * @return Constant string with architecture name
 *         Examples: "ARM Cortex-M7", "Xtensa LX6", "Host (x86_64)"
 */
const char* arch_get_name(void);

/**
 * @brief Get architecture capabilities bitmask
 *
 * Returns a bitmask indicating which optional features are available
 * on this architecture. Use ARCH_CAP_* constants to test capabilities.
 *
 * @return Bitmask of architecture capabilities
 *
 * @example
 * uint32_t caps = arch_get_capabilities();
 * if (caps & ARCH_CAP_FPU) {
 *     // Use floating point operations
 * }
 */
uint32_t arch_get_capabilities(void);

// Architecture capability flags
#define ARCH_CAP_FPU        (1 << 0)  ///< Floating Point Unit available
#define ARCH_CAP_DSP        (1 << 1)  ///< DSP instructions/extensions available
#define ARCH_CAP_CACHE      (1 << 2)  ///< Instruction/data cache available
#define ARCH_CAP_DMA        (1 << 3)  ///< DMA controller available
#define ARCH_CAP_SIMD       (1 << 4)  ///< SIMD instructions (NEON, etc.)
#define ARCH_CAP_DUAL_CORE  (1 << 5)  ///< Dual-core/multi-core CPU

// ============================================================================
// Architecture Configuration (from arch/<arch>/arch_config.h)
// ============================================================================

// The following are defined in architecture-specific headers:
// - ARCH_NAME: Architecture name string
// - ARCH_HAS_FPU: 1 if FPU available, 0 otherwise
// - ARCH_HAS_DSP: 1 if DSP available, 0 otherwise
// - ARCH_HAS_CACHE: 1 if cache available, 0 otherwise
// - ARCH_MAX_FFT_SIZE: Maximum supported FFT size
// - ARCH_ALIGNMENT: Memory alignment requirement (bytes)

#ifdef __cplusplus
}
#endif

#endif // EMBEDDIP_ARCH_H
