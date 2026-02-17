/* ========================================================================== */
/*  File: error.h                                                             */
/*  Brief: Unified error handling for the EmbedDIP library                    */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */
#ifndef EMBEDDIP_ERROR_H
#define EMBEDDIP_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file error.h
 * @brief Unified error handling and status codes for EmbedDIP.
 *
 * This header provides a consistent error reporting mechanism across all
 * EmbedDIP modules. Functions that can fail should return embeddip_status_t
 * to indicate success or the specific error condition.
 *
 * @note This replaces inconsistent patterns like NULL returns, asserts, and
 *       void functions throughout the library.
 */

/**
 * @defgroup embedDIP_error Error Handling
 * @ingroup embedDIP_c_api
 * @brief Unified error codes and status reporting.
 * @{
 */

/**
 * @enum embeddip_status_t
 * @brief Standard error codes returned by EmbedDIP functions.
 *
 * Success is indicated by EMBEDDIP_OK (0). All error codes are negative
 * values to allow functions to return positive values for data when
 * appropriate.
 */
typedef enum {
    EMBEDDIP_OK = 0,                    /**< Operation successful */

    /* Memory-related errors */
    EMBEDDIP_ERROR_OUT_OF_MEMORY = -1,  /**< Memory allocation failed */
    EMBEDDIP_ERROR_NULL_PTR = -2,       /**< Null pointer argument */

    /* Input validation errors */
    EMBEDDIP_ERROR_INVALID_ARG = -3,    /**< Invalid argument value */
    EMBEDDIP_ERROR_INVALID_FORMAT = -4, /**< Invalid image format */
    EMBEDDIP_ERROR_INVALID_SIZE = -5,   /**< Invalid size or dimensions */
    EMBEDDIP_ERROR_INVALID_DEPTH = -6,  /**< Invalid image depth/precision */

    /* Operation errors */
    EMBEDDIP_ERROR_NOT_SUPPORTED = -7,  /**< Operation not supported */
    EMBEDDIP_ERROR_NOT_INITIALIZED = -8,/**< Component not initialized */
    EMBEDDIP_ERROR_BUSY = -9,           /**< Resource is busy */
    EMBEDDIP_ERROR_TIMEOUT = -10,       /**< Operation timed out */

    /* Device errors */
    EMBEDDIP_ERROR_DEVICE_ERROR = -11,  /**< Hardware device error */
    EMBEDDIP_ERROR_IO_ERROR = -12,      /**< I/O operation failed */
    EMBEDDIP_ERROR_COMMUNICATION = -13, /**< Communication error */

    /* Data errors */
    EMBEDDIP_ERROR_OVERFLOW = -14,      /**< Arithmetic overflow */
    EMBEDDIP_ERROR_UNDERFLOW = -15,     /**< Arithmetic underflow */
    EMBEDDIP_ERROR_OUT_OF_RANGE = -16,  /**< Value out of valid range */

    /* Generic errors */
    EMBEDDIP_ERROR_UNKNOWN = -99        /**< Unknown or unspecified error */
} embeddip_status_t;

/**
 * @brief Convert error code to human-readable string.
 *
 * @param status Error code to convert.
 * @return Constant string describing the error, or "Unknown error" if
 *         the code is not recognized.
 *
 * @note The returned string is a constant and should not be freed.
 *
 * Example:
 * @code
 * embeddip_status_t err = some_function();
 * if (err != EMBEDDIP_OK) {
 *     printf("Error: %s\n", embeddip_status_str(err));
 * }
 * @endcode
 */
const char* embeddip_status_str(embeddip_status_t status);

/**
 * @brief Check if status code indicates success.
 *
 * @param status Status code to check.
 * @return 1 if successful (EMBEDDIP_OK), 0 otherwise.
 */
static inline int embeddip_success(embeddip_status_t status) {
    return (status == EMBEDDIP_OK);
}

/**
 * @brief Check if status code indicates failure.
 *
 * @param status Status code to check.
 * @return 1 if failed (not EMBEDDIP_OK), 0 otherwise.
 */
static inline int embeddip_failed(embeddip_status_t status) {
    return (status != EMBEDDIP_OK);
}

/** @} */ /* end of embedDIP_error */

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_ERROR_H */
