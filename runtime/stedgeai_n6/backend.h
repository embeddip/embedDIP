// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_RUNTIME_STEDGEAI_N6_BACKEND_H
#define EMBEDDIP_RUNTIME_STEDGEAI_N6_BACKEND_H

#include "runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef embeddip_status_t (*stedgeai_n6_init_fn)(void *context);
typedef embeddip_status_t (*stedgeai_n6_run_fn)(void *context, const cv_tensor_t *input,
                                                cv_tensor_t *output);

typedef struct {
    void *context;
    stedgeai_n6_init_fn init;
    stedgeai_n6_run_fn run;
} stedgeai_n6_binding_t;

embeddip_status_t stedgeai_n6_backend_create(const stedgeai_n6_binding_t *binding,
                                              const cv_tensor_t *input_contract,
                                              const cv_tensor_t *output_contract,
                                              cv_runtime_backend_t *out_backend);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_RUNTIME_STEDGEAI_N6_BACKEND_H */
