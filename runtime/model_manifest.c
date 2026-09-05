// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "runtime/model_manifest.h"

#include <stdint.h>

static int string_is_nonempty(const char *value)
{
    return value != NULL && value[0] != '\0';
}

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
    return expected_bytes == tensor->bytes ? EMBEDDIP_OK : EMBEDDIP_ERROR_INVALID_SIZE;
}

embeddip_status_t cv_model_manifest_validate(const cv_model_manifest_t *manifest)
{
    embeddip_status_t status;

    if (manifest == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!string_is_nonempty(manifest->model_id) || !string_is_nonempty(manifest->source_sha256) ||
        !string_is_nonempty(manifest->generated_sha256) ||
        !string_is_nonempty(manifest->stedgeai_version) ||
        !string_is_nonempty(manifest->cube_n6_version) || !string_is_nonempty(manifest->license) ||
        !string_is_nonempty(manifest->dataset_license) ||
        !string_is_nonempty(manifest->label_map_id) ||
        !string_is_nonempty(manifest->training_recipe) ||
        !string_is_nonempty(manifest->quantization_recipe)) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (manifest->deployment_location != CV_DEPLOYMENT_MCU) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }
    if (manifest->weights_bytes == 0u || manifest->activations_bytes == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (manifest->weights_region != EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }
    if (manifest->activations_region != EMBEDDIP_MEMORY_REGION_FAST_SRAM &&
        manifest->activations_region != EMBEDDIP_MEMORY_REGION_PSRAM) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    status = tensor_contract_validate(&manifest->input);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    return tensor_contract_validate(&manifest->output);
}
