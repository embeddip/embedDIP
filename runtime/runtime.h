// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_RUNTIME_RUNTIME_H
#define EMBEDDIP_RUNTIME_RUNTIME_H

#include <stdint.h>

#include "core/error.h"
#include "core/memory_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { CV_TENSOR_U8, CV_TENSOR_I8, CV_TENSOR_F32 } cv_tensor_type_t;
typedef enum { CV_TENSOR_HWC, CV_TENSOR_CHW } cv_tensor_layout_t;

typedef struct {
    void *data;
    uint32_t bytes;
    uint16_t width;
    uint16_t height;
    uint16_t channels;
    cv_tensor_type_t type;
    cv_tensor_layout_t layout;
    float scale;
    int32_t zero_point;
    embeddip_memory_region_t region;
    uint32_t flags;
} cv_tensor_t;

typedef embeddip_status_t (*cv_runtime_invoke_fn)(void *context, const cv_tensor_t *input,
                                                  cv_tensor_t *output);

typedef struct {
    void *context;
    cv_tensor_t input_contract;
    cv_tensor_t output_contract;
    cv_runtime_invoke_fn invoke;
} cv_runtime_backend_t;

embeddip_status_t cv_runtime_init(const cv_runtime_backend_t *backend);
embeddip_status_t cv_runtime_infer(const cv_tensor_t *input, cv_tensor_t *output,
                                   uint32_t *elapsed_cycles);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_RUNTIME_RUNTIME_H */
