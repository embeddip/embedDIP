#!/usr/bin/env python3

import json
import pathlib
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "model_manifest.py"
VALID = ROOT / "tests" / "fixtures" / "valid_model_manifest.json"
INVALID = ROOT / "tests" / "fixtures" / "invalid_model_manifest.json"


def run(*args):
    return subprocess.run([sys.executable, str(TOOL), *map(str, args)], check=False)


def compile_rendered(path):
    return subprocess.run(
        [
            "cc",
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{ROOT}",
            "-fsyntax-only",
            str(path),
        ],
        check=False,
    )


assert run("validate", VALID).returncode == 0
assert run("validate", INVALID).returncode != 0

with tempfile.TemporaryDirectory() as directory:
    temporary = pathlib.Path(directory)
    baseline = json.loads(VALID.read_text(encoding="utf-8"))
    invalid_cases = []

    extra_record = json.loads(json.dumps(baseline))
    extra_record["unexpected"] = {}
    invalid_cases.append(extra_record)
    short_hash = json.loads(json.dumps(baseline))
    short_hash["model"]["source_sha256"] = "abc"
    invalid_cases.append(short_hash)
    bad_type = json.loads(json.dumps(baseline))
    bad_type["io"]["input"]["type"] = "u16"
    invalid_cases.append(bad_type)
    zero_dimension = json.loads(json.dumps(baseline))
    zero_dimension["io"]["output"]["height"] = 0
    invalid_cases.append(zero_dimension)
    zero_bytes = json.loads(json.dumps(baseline))
    zero_bytes["memory"]["weights_bytes"] = 0
    invalid_cases.append(zero_bytes)
    missing_field = json.loads(json.dumps(baseline))
    del missing_field["legal"]["license"]
    invalid_cases.append(missing_field)
    wrong_field_type = json.loads(json.dumps(baseline))
    wrong_field_type["io"]["input"]["scale"] = "1.0"
    invalid_cases.append(wrong_field_type)
    dimension_overflow = json.loads(json.dumps(baseline))
    dimension_overflow["io"]["input"]["width"] = 65536
    invalid_cases.append(dimension_overflow)
    derived_bytes_overflow = json.loads(json.dumps(baseline))
    derived_bytes_overflow["io"]["input"].update(
        {"width": 65535, "height": 65535, "channels": 2, "type": "u8"}
    )
    invalid_cases.append(derived_bytes_overflow)
    weights_overflow = json.loads(json.dumps(baseline))
    weights_overflow["memory"]["weights_bytes"] = 4294967296
    invalid_cases.append(weights_overflow)
    activations_overflow = json.loads(json.dumps(baseline))
    activations_overflow["memory"]["activations_bytes"] = 4294967296
    invalid_cases.append(activations_overflow)
    positive_zero_point_overflow = json.loads(json.dumps(baseline))
    positive_zero_point_overflow["io"]["input"]["zero_point"] = 2147483648
    invalid_cases.append(positive_zero_point_overflow)
    negative_zero_point_overflow = json.loads(json.dumps(baseline))
    negative_zero_point_overflow["io"]["output"]["zero_point"] = -2147483649
    invalid_cases.append(negative_zero_point_overflow)
    control_character = json.loads(json.dumps(baseline))
    control_character["legal"]["license"] = "MIT\nBSD"
    invalid_cases.append(control_character)

    for index, record in enumerate(invalid_cases):
        path = temporary / f"invalid-{index}.json"
        path.write_text(json.dumps(record), encoding="utf-8")
        assert run("validate", path).returncode != 0

    render_record = json.loads(json.dumps(baseline))
    render_record["model"]["id"] = "unit-classifier.1"
    render_input = temporary / "render.json"
    render_output = temporary / "render.c"
    render_input.write_text(json.dumps(render_record), encoding="utf-8")
    assert run("render-c", render_input, "--output", render_output).returncode == 0
    rendered = render_output.read_text(encoding="utf-8")
    assert rendered.count("const cv_model_manifest_t") == 1
    assert "const cv_model_manifest_t embeddip_model_unit_classifier_1 =" in rendered
    assert ".deployment_location = CV_DEPLOYMENT_MCU" in rendered
    assert "unit.onnx" not in rendered
    assert "unit.xSPI2.bin" not in rendered
    assert compile_rendered(render_output).returncode == 0

    for index, model_id in enumerate(("int", "_Foo", "__foo")):
        reserved_record = json.loads(json.dumps(baseline))
        reserved_record["model"]["id"] = model_id
        reserved_input = temporary / f"reserved-{index}.json"
        reserved_output = temporary / f"reserved-{index}.c"
        reserved_input.write_text(json.dumps(reserved_record), encoding="utf-8")
        assert run("render-c", reserved_input, "--output", reserved_output).returncode == 0
        reserved_rendered = reserved_output.read_text(encoding="utf-8")
        sanitized = "".join(character if character.isalnum() else "_" for character in model_id)
        assert f"const cv_model_manifest_t embeddip_model_{sanitized} =" in reserved_rendered
        assert compile_rendered(reserved_output).returncode == 0

    boundary_record = json.loads(json.dumps(baseline))
    boundary_record["io"]["input"].update(
        {"width": 65535, "height": 1, "channels": 1, "zero_point": -2147483648}
    )
    boundary_record["io"]["output"]["zero_point"] = 2147483647
    boundary_record["memory"].update(
        {"weights_bytes": 4294967295, "activations_bytes": 4294967295}
    )
    boundary_input = temporary / "boundary.json"
    boundary_output = temporary / "boundary.c"
    boundary_input.write_text(json.dumps(boundary_record), encoding="utf-8")
    assert run("render-c", boundary_input, "--output", boundary_output).returncode == 0
    boundary_rendered = boundary_output.read_text(encoding="utf-8")
    assert ".width = 65535u" in boundary_rendered
    assert ".zero_point = -2147483648" in boundary_rendered
    assert ".zero_point = 2147483647" in boundary_rendered
    assert ".weights_bytes = 4294967295u" in boundary_rendered
    assert ".activations_bytes = 4294967295u" in boundary_rendered
    assert compile_rendered(boundary_output).returncode == 0
