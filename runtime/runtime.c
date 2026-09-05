// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "runtime/runtime.h"

#include <stddef.h>
#include <stdint.h>

#include "board/common.h"

static cv_runtime_backend_t runtime_backend;
static int runtime_is_initialized;

static embeddip_status_t tensor_contract_validate(const cv_tensor_t *tensor)
{
    uint64_t element_bytes;
    uint64_t expected_bytes;

    if (tensor->bytes == 0u || tensor->width == 0u || tensor->height == 0u ||
        tensor->channels == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (tensor->type != CV_TENSOR_U8 && tensor->type != CV_TENSOR_I8 &&
        tensor->type != CV_TENSOR_F32) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }
    if (tensor->layout != CV_TENSOR_HWC && tensor->layout != CV_TENSOR_CHW) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    element_bytes = tensor->type == CV_TENSOR_F32 ? 4u : 1u;
    expected_bytes = (uint64_t)tensor->width * tensor->height * tensor->channels * element_bytes;
    if (expected_bytes != tensor->bytes) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    return EMBEDDIP_OK;
}

static embeddip_status_t tensor_matches_contract(const cv_tensor_t *tensor,
                                                  const cv_tensor_t *contract)
{
    if (tensor->bytes != contract->bytes || tensor->width != contract->width ||
        tensor->height != contract->height || tensor->channels != contract->channels) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (tensor->type != contract->type || tensor->layout != contract->layout) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    return EMBEDDIP_OK;
}

embeddip_status_t cv_runtime_init(const cv_runtime_backend_t *backend)
{
    embeddip_status_t status;

    if (backend == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (backend->invoke == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    status = tensor_contract_validate(&backend->input_contract);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    status = tensor_contract_validate(&backend->output_contract);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    runtime_backend = *backend;
    runtime_is_initialized = 1;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_runtime_infer(const cv_tensor_t *input, cv_tensor_t *output,
                                   uint32_t *elapsed_cycles)
{
    embeddip_status_t status;
    embeddip_status_t invalidate_status = EMBEDDIP_OK;
    uint32_t cycles;

    if (!runtime_is_initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    if (input == NULL || output == NULL || input->data == NULL || output->data == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    status = tensor_matches_contract(input, &runtime_backend.input_contract);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    status = tensor_matches_contract(output, &runtime_backend.output_contract);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    if ((output->flags & EMBEDDIP_BUFFER_READ_ONLY) != 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    if ((input->flags & EMBEDDIP_BUFFER_NPU_READ) != 0u) {
        status = memory_cache_clean(input->data, input->bytes);
        if (status != EMBEDDIP_OK) {
            return status;
        }
    }

    tic();
    status = runtime_backend.invoke(runtime_backend.context, input, output);
    cycles = toc();

    if ((output->flags & EMBEDDIP_BUFFER_NPU_WRITE) != 0u) {
        invalidate_status = memory_cache_invalidate(output->data, output->bytes);
    }
    if (elapsed_cycles != NULL) {
        *elapsed_cycles = cycles;
    }
    if (status != EMBEDDIP_OK) {
        return status;
    }
    return invalidate_status;
}
