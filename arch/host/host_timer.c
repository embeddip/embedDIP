// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <board/common.h>

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

static struct timespec start_time;
static bool timer_started;

void tic(void)
{
    (void)clock_gettime(CLOCK_MONOTONIC, &start_time);
    timer_started = true;
}

uint32_t toc(void)
{
    struct timespec end_time;
    uint64_t elapsed_ns;

    if (!timer_started || clock_gettime(CLOCK_MONOTONIC, &end_time) != 0) {
        return 0u;
    }

    elapsed_ns = (uint64_t)(end_time.tv_sec - start_time.tv_sec) * UINT64_C(1000000000);
    elapsed_ns += (uint64_t)(end_time.tv_nsec - start_time.tv_nsec);
    return (uint32_t)elapsed_ns;
}
