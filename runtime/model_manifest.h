// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_RUNTIME_MODEL_MANIFEST_H
#define EMBEDDIP_RUNTIME_MODEL_MANIFEST_H

#include <stdint.h>

#include "runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CV_DEPLOYMENT_MCU = 0,
    CV_DEPLOYMENT_HOST
} cv_deployment_location_t;

typedef struct {
    const char *model_id;
    const char *source_sha256;
    const char *generated_sha256;
    const char *stedgeai_version;
    const char *cube_n6_version;
    const char *license;
    const char *dataset_license;
    const char *label_map_id;
    const char *training_recipe;
    const char *quantization_recipe;
    cv_tensor_t input;
    cv_tensor_t output;
    uint32_t weights_bytes;
    uint32_t activations_bytes;
    embeddip_memory_region_t weights_region;
    embeddip_memory_region_t activations_region;
    cv_deployment_location_t deployment_location;
} cv_model_manifest_t;

embeddip_status_t cv_model_manifest_validate(const cv_model_manifest_t *manifest);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_RUNTIME_MODEL_MANIFEST_H */
