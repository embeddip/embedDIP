/* ========================================================================== */
/*  File: error.c                                                             */
/*  Brief: Error handling implementation for EmbedDIP                         */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "error.h"

/**
 * @brief Convert error code to human-readable string.
 */
const char* embeddip_status_str(embeddip_status_t status) {
    switch (status) {
        case EMBEDDIP_OK:
            return "Success";

        /* Memory-related errors */
        case EMBEDDIP_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case EMBEDDIP_ERROR_NULL_PTR:
            return "Null pointer";

        /* Input validation errors */
        case EMBEDDIP_ERROR_INVALID_ARG:
            return "Invalid argument";
        case EMBEDDIP_ERROR_INVALID_FORMAT:
            return "Invalid format";
        case EMBEDDIP_ERROR_INVALID_SIZE:
            return "Invalid size";
        case EMBEDDIP_ERROR_INVALID_DEPTH:
            return "Invalid depth";

        /* Operation errors */
        case EMBEDDIP_ERROR_NOT_SUPPORTED:
            return "Operation not supported";
        case EMBEDDIP_ERROR_NOT_INITIALIZED:
            return "Not initialized";
        case EMBEDDIP_ERROR_BUSY:
            return "Resource busy";
        case EMBEDDIP_ERROR_TIMEOUT:
            return "Timeout";

        /* Device errors */
        case EMBEDDIP_ERROR_DEVICE_ERROR:
            return "Device error";
        case EMBEDDIP_ERROR_IO_ERROR:
            return "I/O error";
        case EMBEDDIP_ERROR_COMMUNICATION:
            return "Communication error";

        /* Data errors */
        case EMBEDDIP_ERROR_OVERFLOW:
            return "Overflow";
        case EMBEDDIP_ERROR_UNDERFLOW:
            return "Underflow";
        case EMBEDDIP_ERROR_OUT_OF_RANGE:
            return "Out of range";

        /* Generic errors */
        case EMBEDDIP_ERROR_UNKNOWN:
            return "Unknown error";

        default:
            return "Unrecognized error code";
    }
}
