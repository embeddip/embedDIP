/* ========================================================================== */
/*  File: host_uart.c                                                         */
/*  Brief: HOST platform serial implementation (stdin/stdout)                 */
/*  SPDX-License-Identifier: MIT                                              */
/*  Copyright (c) 2024–2025                                                   */
/* ========================================================================== */

#include "device/serial/serial.h"
#include "core/error.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef TARGET_BOARD_HOST

/**
 * @brief HOST UART state
 */
static struct
{
    bool initialized;
    uint32_t baud_rate;
} host_uart = {.initialized = false, .baud_rate = 115200};

/**
 * @brief Initialize HOST UART (use stdin/stdout)
 * @param baud_rate Baud rate (ignored for HOST, informational only)
 * @return EMBEDDIP_OK on success
 */
embeddip_status_t UART_Init(uint32_t baud_rate)
{
    host_uart.baud_rate = baud_rate;
    host_uart.initialized = true;

    printf("[HOST UART] Initialized (using stdin/stdout, baud=%u)\n", baud_rate);
    return EMBEDDIP_OK;
}

/**
 * @brief Transmit data over UART
 * @param data Pointer to data buffer
 * @param size Number of bytes to transmit
 * @return EMBEDDIP_OK on success, error code otherwise
 */
embeddip_status_t UART_Transmit(const uint8_t *data, uint16_t size)
{
    if (!host_uart.initialized) {
        fprintf(stderr, "[HOST UART] Error: Not initialized\n");
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    // Write to stdout
    size_t written = fwrite(data, 1, size, stdout);
    fflush(stdout);

    if (written != size) {
        fprintf(stderr, "[HOST UART] Error: Wrote %zu bytes, expected %u\n", written, size);
        return EMBEDDIP_ERROR_DEVICE_ERROR;
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Receive data from UART
 * @param data Pointer to data buffer
 * @param size Number of bytes to receive
 * @return EMBEDDIP_OK on success, error code otherwise
 */
embeddip_status_t UART_Receive(uint8_t *data, uint16_t size)
{
    if (!host_uart.initialized) {
        fprintf(stderr, "[HOST UART] Error: Not initialized\n");
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    if (!data) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    // Read from stdin
    size_t bytes_read = fread(data, 1, size, stdin);

    if (bytes_read != size) {
        fprintf(stderr, "[HOST UART] Error: Read %zu bytes, expected %u\n", bytes_read, size);
        return EMBEDDIP_ERROR_DEVICE_ERROR;
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Print string to UART
 * @param str Null-terminated string
 * @return EMBEDDIP_OK on success, error code otherwise
 */
embeddip_status_t UART_Print(const char *str)
{
    if (!host_uart.initialized) {
        fprintf(stderr, "[HOST UART] Error: Not initialized\n");
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    if (!str) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    printf("%s", str);
    fflush(stdout);

    return EMBEDDIP_OK;
}

/**
 * @brief Print formatted string to UART (printf-style)
 * @param format Format string
 * @param ... Variable arguments
 * @return EMBEDDIP_OK on success, error code otherwise
 */
embeddip_status_t UART_Printf(const char *format, ...)
{
    if (!host_uart.initialized) {
        fprintf(stderr, "[HOST UART] Error: Not initialized\n");
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    if (!format) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);

    return EMBEDDIP_OK;
}

/**
 * @brief De-initialize HOST UART
 * @return EMBEDDIP_OK on success
 */
embeddip_status_t UART_DeInit(void)
{
    host_uart.initialized = false;
    printf("[HOST UART] De-initialized\n");
    return EMBEDDIP_OK;
}

#endif /* TARGET_BOARD_HOST */
