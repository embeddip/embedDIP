#!/usr/bin/env python3

import argparse
import hashlib
import json
import pathlib


TOP_LEVEL_RECORDS = {
    "model",
    "generated",
    "deployment",
    "legal",
    "provenance",
    "io",
    "memory",
}
HASH_HEX_LENGTH = hashlib.sha256().digest_size * 2
UINT16_MAX = (1 << 16) - 1
UINT32_MAX = (1 << 32) - 1
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1


def require_record(parent, name, fields):
    record = parent.get(name)
    if type(record) is not dict:
        raise ValueError(f"{name} must be an object")
    for field, expected_type in fields.items():
        if field not in record or type(record[field]) is not expected_type:
            raise ValueError(f"{name}.{field} must be {expected_type.__name__}")
        if expected_type is str and not record[field]:
            raise ValueError(f"{name}.{field} must not be empty")
        if expected_type is str and any(ord(character) < 0x20 for character in record[field]):
            raise ValueError(f"{name}.{field} must not contain JSON control characters")
    return record


def validate_tensor(io_record, name):
    tensor = require_record(
        io_record,
        name,
        {
            "width": int,
            "height": int,
            "channels": int,
            "type": str,
            "layout": str,
            "scale": float,
            "zero_point": int,
        },
    )
    if tensor["width"] <= 0 or tensor["height"] <= 0 or tensor["channels"] <= 0:
        raise ValueError(f"io.{name} dimensions must be positive")
    if any(tensor[field] > UINT16_MAX for field in ("width", "height", "channels")):
        raise ValueError(f"io.{name} dimensions must fit uint16_t")
    if tensor["type"] not in {"u8", "i8", "f32"}:
        raise ValueError(f"io.{name}.type is unsupported")
    if tensor["layout"] not in {"hwc", "chw"}:
        raise ValueError(f"io.{name}.layout is unsupported")
    if not INT32_MIN <= tensor["zero_point"] <= INT32_MAX:
        raise ValueError(f"io.{name}.zero_point must fit int32_t")
    element_bytes = 4 if tensor["type"] == "f32" else 1
    byte_count = tensor["width"] * tensor["height"] * tensor["channels"] * element_bytes
    if byte_count > UINT32_MAX:
        raise ValueError(f"io.{name} derived byte count must fit uint32_t")
    return tensor


def validate_manifest(record):
    if type(record) is not dict or set(record) != TOP_LEVEL_RECORDS:
        raise ValueError("manifest must contain precisely the seven required top-level records")

    model = require_record(record, "model", {"id": str, "onnx_file": str, "source_sha256": str})
    generated = require_record(
        record, "generated", {"artifact_sha256": str, "weights_blob": str}
    )
    deployment = require_record(
        record,
        "deployment",
        {"inference_location": str, "stedgeai_version": str, "cube_n6_version": str},
    )
    require_record(record, "legal", {"license": str, "label_map_id": str, "dataset_license": str})
    require_record(
        record, "provenance", {"training_recipe": str, "quantization_recipe": str}
    )
    io_record = require_record(record, "io", {"input": dict, "output": dict})
    input_tensor = validate_tensor(io_record, "input")
    output_tensor = validate_tensor(io_record, "output")
    memory = require_record(
        record,
        "memory",
        {
            "weights_bytes": int,
            "activations_bytes": int,
            "weights_region": str,
            "activations_region": str,
        },
    )

    hexadecimal = set("0123456789abcdefABCDEF")
    for name, value in (
        ("model.source_sha256", model["source_sha256"]),
        ("generated.artifact_sha256", generated["artifact_sha256"]),
    ):
        if len(value) != HASH_HEX_LENGTH or any(character not in hexadecimal for character in value):
            raise ValueError(f"{name} must contain 64 hexadecimal characters")
    if deployment["inference_location"] != "mcu":
        raise ValueError("deployment.inference_location must be mcu")
    if memory["weights_bytes"] <= 0 or memory["activations_bytes"] <= 0:
        raise ValueError("memory byte counts must be positive")
    if memory["weights_bytes"] > UINT32_MAX or memory["activations_bytes"] > UINT32_MAX:
        raise ValueError("memory byte counts must fit uint32_t")
    if memory["weights_region"] != "external_flash":
        raise ValueError("memory.weights_region must be external_flash")
    if memory["activations_region"] not in {"fast_sram", "psram"}:
        raise ValueError("memory.activations_region must be fast_sram or psram")

    return input_tensor, output_tensor


def read_and_validate(path):
    record = json.loads(path.read_text(encoding="utf-8"))
    input_tensor, output_tensor = validate_manifest(record)
    return record, input_tensor, output_tensor


def c_identifier(model_id):
    suffix = "".join(
        character
        if ("a" <= character <= "z" or "A" <= character <= "Z" or "0" <= character <= "9")
        else "_"
        for character in model_id
    )
    return "embeddip_model_" + suffix


def c_string(value):
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def tensor_initializer(tensor):
    type_names = {"u8": "CV_TENSOR_U8", "i8": "CV_TENSOR_I8", "f32": "CV_TENSOR_F32"}
    layout_names = {"hwc": "CV_TENSOR_HWC", "chw": "CV_TENSOR_CHW"}
    element_bytes = 4 if tensor["type"] == "f32" else 1
    byte_count = tensor["width"] * tensor["height"] * tensor["channels"] * element_bytes
    return (
        "{ .data = NULL, "
        f".bytes = {byte_count}u, .width = {tensor['width']}u, .height = {tensor['height']}u, "
        f".channels = {tensor['channels']}u, .type = {type_names[tensor['type']]}, "
        f".layout = {layout_names[tensor['layout']]}, .scale = {tensor['scale']!r}f, "
        f".zero_point = {tensor['zero_point']}, .region = EMBEDDIP_MEMORY_REGION_DEFAULT, .flags = 0u }}"
    )


def render_c(record, input_tensor, output_tensor):
    model = record["model"]
    generated = record["generated"]
    deployment = record["deployment"]
    legal = record["legal"]
    provenance = record["provenance"]
    memory = record["memory"]
    activation_regions = {
        "fast_sram": "EMBEDDIP_MEMORY_REGION_FAST_SRAM",
        "psram": "EMBEDDIP_MEMORY_REGION_PSRAM",
    }
    return f"""// Generated by tools/model_manifest.py; contains metadata only.
#include <stddef.h>
#include "runtime/model_manifest.h"

const cv_model_manifest_t {c_identifier(model['id'])} = {{
    .model_id = {c_string(model['id'])},
    .source_sha256 = {c_string(model['source_sha256'])},
    .generated_sha256 = {c_string(generated['artifact_sha256'])},
    .stedgeai_version = {c_string(deployment['stedgeai_version'])},
    .cube_n6_version = {c_string(deployment['cube_n6_version'])},
    .license = {c_string(legal['license'])},
    .dataset_license = {c_string(legal['dataset_license'])},
    .label_map_id = {c_string(legal['label_map_id'])},
    .training_recipe = {c_string(provenance['training_recipe'])},
    .quantization_recipe = {c_string(provenance['quantization_recipe'])},
    .input = {tensor_initializer(input_tensor)},
    .output = {tensor_initializer(output_tensor)},
    .weights_bytes = {memory['weights_bytes']}u,
    .activations_bytes = {memory['activations_bytes']}u,
    .weights_region = EMBEDDIP_MEMORY_REGION_EXTERNAL_FLASH,
    .activations_region = {activation_regions[memory['activations_region']]},
    .deployment_location = CV_DEPLOYMENT_MCU
}};
"""


def main():
    parser = argparse.ArgumentParser(description="Validate and render EmbedDIP model manifests")
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate_parser = subparsers.add_parser("validate")
    validate_parser.add_argument("manifest", type=pathlib.Path)
    render_parser = subparsers.add_parser("render-c")
    render_parser.add_argument("manifest", type=pathlib.Path)
    render_parser.add_argument("--output", type=pathlib.Path, required=True)
    arguments = parser.parse_args()

    try:
        record, input_tensor, output_tensor = read_and_validate(arguments.manifest)
        if arguments.command == "render-c":
            arguments.output.write_text(
                render_c(record, input_tensor, output_tensor), encoding="utf-8"
            )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
