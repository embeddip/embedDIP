// SPDX-License-Identifier: MIT

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "runtime/model_manifest.h"
#include "runtime/runtime.h"
#include "runtime/stedgeai_n6/backend.h"

static int calls;
static int init_calls;
static int event_count;
static char events[8];
static embeddip_status_t invoke_result = EMBEDDIP_OK;

void tic(void)
{
    events[event_count++] = 'T';
}

uint32_t toc(void)
{
    events[event_count++] = 't';
    return 1234u;
}

embeddip_status_t memory_cache_clean(const void *address, size_t size)
{
    assert(address != NULL);
    assert(size == 12u);
    events[event_count++] = 'C';
    return EMBEDDIP_OK;
}

embeddip_status_t memory_cache_invalidate(const void *address, size_t size)
{
    assert(address != NULL);
    assert(size == 1u);
    events[event_count++] = 'I';
    return EMBEDDIP_OK;
}

static embeddip_status_t mock_invoke(void *context, const cv_tensor_t *input,
                                     cv_tensor_t *output)
{
    assert(context == (void *)0x1234u);
    ++calls;
    events[event_count++] = 'R';
    ((uint8_t *)output->data)[0] = ((const uint8_t *)input->data)[0];
    return invoke_result;
}

static cv_tensor_t input_contract(void)
{
    cv_tensor_t tensor = {0};
    tensor.bytes = 12u;
    tensor.width = 2u;
    tensor.height = 2u;
    tensor.channels = 3u;
    tensor.type = CV_TENSOR_U8;
    tensor.layout = CV_TENSOR_HWC;
    return tensor;
}

static cv_tensor_t output_contract(void)
{
    cv_tensor_t tensor = {0};
    tensor.bytes = 1u;
    tensor.width = 1u;
    tensor.height = 1u;
    tensor.channels = 1u;
    tensor.type = CV_TENSOR_U8;
    tensor.layout = CV_TENSOR_HWC;
    return tensor;
}

static cv_runtime_backend_t valid_backend(void)
{
    cv_runtime_backend_t backend = {0};
    backend.context = (void *)0x1234u;
    backend.input_contract = input_contract();
    backend.output_contract = output_contract();
    backend.invoke = mock_invoke;
    return backend;
}

static void test_runtime_rejects_inference_before_initialisation(void)
{
    uint8_t input_data[12] = {7u};
    uint8_t output_data[1] = {0u};
    cv_tensor_t input = input_contract();
    cv_tensor_t output = output_contract();
    input.data = input_data;
    output.data = output_data;
    assert(cv_runtime_infer(&input, &output, NULL) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_runtime_rejects_invalid_backends(void)
{
    cv_runtime_backend_t backend = valid_backend();
    assert(cv_runtime_init(NULL) == EMBEDDIP_ERROR_NULL_PTR);
    backend.invoke = NULL;
    assert(cv_runtime_init(&backend) == EMBEDDIP_ERROR_NULL_PTR);

    backend = valid_backend();
    backend.input_contract.bytes = 0u;
    assert(cv_runtime_init(&backend) == EMBEDDIP_ERROR_INVALID_SIZE);
    backend = valid_backend();
    backend.output_contract.width = 0u;
    assert(cv_runtime_init(&backend) == EMBEDDIP_ERROR_INVALID_SIZE);
    backend = valid_backend();
    backend.input_contract.type = (cv_tensor_type_t)99;
    assert(cv_runtime_init(&backend) == EMBEDDIP_ERROR_NOT_SUPPORTED);
    backend = valid_backend();
    backend.output_contract.layout = (cv_tensor_layout_t)99;
    assert(cv_runtime_init(&backend) == EMBEDDIP_ERROR_NOT_SUPPORTED);
}

static void test_runtime_validates_tensors_and_runs_with_coherency(void)
{
    uint8_t input_data[12] = {7u};
    uint8_t output_data[1] = {0u};
    uint32_t cycles = 0u;
    cv_runtime_backend_t backend = valid_backend();
    cv_tensor_t input = backend.input_contract;
    cv_tensor_t output = backend.output_contract;

    assert(cv_runtime_init(&backend) == EMBEDDIP_OK);
    input.data = input_data;
    output.data = output_data;
    input.flags = EMBEDDIP_BUFFER_NPU_READ;
    output.flags = EMBEDDIP_BUFFER_NPU_WRITE;
    calls = 0;
    event_count = 0;
    assert(cv_runtime_infer(&input, &output, &cycles) == EMBEDDIP_OK);
    assert(calls == 1);
    assert(output_data[0] == 7u);
    assert(cycles == 1234u);
    assert(event_count == 5);
    assert(memcmp(events, "CTRtI", 5u) == 0);

    input.width = 3u;
    assert(cv_runtime_infer(&input, &output, &cycles) == EMBEDDIP_ERROR_INVALID_SIZE);
    input = backend.input_contract;
    input.data = input_data;
    input.type = CV_TENSOR_I8;
    assert(cv_runtime_infer(&input, &output, &cycles) == EMBEDDIP_ERROR_INVALID_FORMAT);
    input = backend.input_contract;
    input.data = input_data;
    output.flags = EMBEDDIP_BUFFER_READ_ONLY;
    assert(cv_runtime_infer(&input, &output, &cycles) == EMBEDDIP_ERROR_INVALID_ARG);
    output = backend.output_contract;
    output.data = output_data;
    assert(cv_runtime_infer(NULL, &output, &cycles) == EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_runtime_infer(&input, NULL, &cycles) == EMBEDDIP_ERROR_NULL_PTR);
    input.data = NULL;
    assert(cv_runtime_infer(&input, &output, &cycles) == EMBEDDIP_ERROR_NULL_PTR);
}

static embeddip_status_t binding_init(void *context)
{
    assert(context == (void *)0x5678u);
    ++init_calls;
    return EMBEDDIP_OK;
}

static embeddip_status_t binding_run(void *context, const cv_tensor_t *input,
                                     cv_tensor_t *output)
{
    assert(context == (void *)0x5678u);
    return mock_invoke((void *)0x1234u, input, output);
}

static void test_stedgeai_backend_uses_only_explicit_binding(void)
{
    stedgeai_n6_binding_t binding = {(void *)0x5678u, binding_init, binding_run};
    cv_tensor_t input = input_contract();
    cv_tensor_t output = output_contract();
    cv_runtime_backend_t backend = {0};
    uint8_t input_data[12] = {9u};
    uint8_t output_data[1] = {0u};

    assert(stedgeai_n6_backend_create(NULL, &input, &output, &backend) ==
           EMBEDDIP_ERROR_NULL_PTR);
    binding.run = NULL;
    assert(stedgeai_n6_backend_create(&binding, &input, &output, &backend) ==
           EMBEDDIP_ERROR_NULL_PTR);
    binding.run = binding_run;
    init_calls = 0;
    assert(stedgeai_n6_backend_create(&binding, &input, &output, &backend) == EMBEDDIP_OK);
    assert(init_calls == 1);
    assert(backend.context == binding.context);
    assert(backend.input_contract.bytes == 12u);
    assert(backend.output_contract.bytes == 1u);
    input.data = input_data;
    output.data = output_data;
    assert(backend.invoke(backend.context, &input, &output) == EMBEDDIP_OK);
    assert(output_data[0] == 9u);
}

static cv_model_manifest_t valid_manifest(void)
{
    cv_model_manifest_t manifest = {0};
    manifest.model_id = "unit_classifier";
    manifest.source_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    manifest.generated_sha256 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    manifest.stedgeai_version = "4.0.0";
    manifest.cube_n6_version = "1.0.0";
    manifest.license = "MIT";
    manifest.dataset_license = "CC0-1.0";
    manifest.label_map_id = "unit-labels";
    manifest.training_recipe = "unit-train-v1";
    manifest.quantization_recipe = "unit-int8-v1";
    manifest.input = input_contract();
    manifest.output = output_contract();
    manifest.weights_bytes = 64u;
    manifest.activations_bytes = 128u;
    manifest.weights_region = EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH;
    manifest.activations_region = EMBEDDIP_MEMORY_REGION_FAST_SRAM;
    manifest.deployment_location = CV_DEPLOYMENT_MCU;
    return manifest;
}

static void test_manifest_enforces_local_deployment_contract(void)
{
    cv_model_manifest_t manifest = valid_manifest();
    assert(cv_model_manifest_validate(NULL) == EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_OK);
    manifest.activations_region = EMBEDDIP_MEMORY_REGION_PSRAM;
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_OK);
    manifest.deployment_location = CV_DEPLOYMENT_HOST;
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_ERROR_NOT_SUPPORTED);
    manifest = valid_manifest();
    manifest.activations_region = EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH;
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_ERROR_NOT_SUPPORTED);
    manifest = valid_manifest();
    manifest.weights_region = EMBEDDIP_MEMORY_REGION_FAST_SRAM;
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_ERROR_NOT_SUPPORTED);
    manifest = valid_manifest();
    manifest.activations_bytes = 0u;
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_ERROR_INVALID_SIZE);
    manifest = valid_manifest();
    manifest.training_recipe = "";
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_ERROR_INVALID_ARG);
    manifest = valid_manifest();
    manifest.output.channels = 0u;
    assert(cv_model_manifest_validate(&manifest) == EMBEDDIP_ERROR_INVALID_SIZE);
}

int main(void)
{
    test_runtime_rejects_inference_before_initialisation();
    test_runtime_rejects_invalid_backends();
    test_runtime_validates_tensors_and_runs_with_coherency();
    test_stedgeai_backend_uses_only_explicit_binding();
    test_manifest_enforces_local_deployment_contract();
    return 0;
}
