// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "runtime/stedgeai_n6/backend.h"

#include <stddef.h>

embeddip_status_t stedgeai_n6_backend_create(const stedgeai_n6_binding_t *binding,
                                              const cv_tensor_t *input_contract,
                                              const cv_tensor_t *output_contract,
                                              cv_runtime_backend_t *out_backend)
{
    embeddip_status_t status;

    if (binding == NULL || input_contract == NULL || output_contract == NULL ||
        out_backend == NULL || binding->init == NULL || binding->run == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    status = binding->init(binding->context);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    out_backend->context = binding->context;
    out_backend->input_contract = *input_contract;
    out_backend->output_contract = *output_contract;
    out_backend->invoke = binding->run;
    return EMBEDDIP_OK;
}
