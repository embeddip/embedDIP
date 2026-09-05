# Deferred STM32N6 Model Evidence Gate Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve the N6 classifier's unmeasured hardware state in a machine-checkable rejected gate record so software chapter work can continue without a false hardware claim.

**Architecture:** The companion repository owns the concrete classifier record and a dependency-free Python checker. EmbedDIP owns the reusable JSON template, committed publication snapshots of the companion record and manifest, and a Markdown foundation-gate status page that links those self-contained snapshots. The checker applies common provenance requirements to every record, strict camera/LCD/measurement requirements only to `accepted` records, and explicit blocker requirements to `rejected` records.

**Tech Stack:** JSON, Python 3 standard library, CTest/CMake existing host profile, Git.

## Global Constraints

- `decision` is exactly `accepted` or `rejected`.
- An `accepted` record requires manifest-correlated source/build provenance, non-empty board/tool/evidence observations, `inference_location == "mcu"`, `compile_result == "passed"`, `camera_frames >= 100`, `local_display == true`, numeric NPU/frame timing and FPS metrics, a non-empty accuracy metric, and separate non-negative NPU/CPU operator counts.
- A `rejected` record requires a non-empty `blockers` array and may use `null` for unmeasured metrics; it cannot be cited as a supported book listing.
- The initial classifier record is `rejected`; it must not invent a flash, camera-frame count, accuracy, latency, FPS, or operator allocation.
- No record authorizes a host or network inference fallback. The intended deployment location remains `mcu`.
- Do not change the feasibility design status to an accepted foundation result until a real accepted record exists.
- Keep `Book STM32/` unmodified and untracked.

---

## File structure

| File | Responsibility |
| --- | --- |
| `../examples-stm32n6/tests/test_model_gate_record.py` | Validate a supplied model-gate JSON file, with separate accepted and rejected constraints. |
| `../examples-stm32n6/docs/benchmarks/ch12_local_classifier.json` | Preserve the classifier's actual deferred/rejected hardware status and exact blockers. |
| `docs/benchmarks/stm32n6-model-gate-template.json` | Give later chapter/model work the full, reusable record schema. |
| `docs/benchmarks/stm32n6-foundation-gate.md` | Describe software evidence, the rejected classifier record, and the exact hardware transition criteria. |
| `docs/benchmarks/evidence/*.json` | Publish atomic snapshots of the companion-owned gate record and manifest without checkout-escaping links. |

## Task 1: Add an auditable deferred model-evidence gate

**Files:**
- Create: `../examples-stm32n6/tests/test_model_gate_record.py`
- Create: `../examples-stm32n6/docs/benchmarks/ch12_local_classifier.json`
- Create: `docs/benchmarks/stm32n6-model-gate-template.json`
- Create: `docs/benchmarks/stm32n6-foundation-gate.md`

**Interfaces:**
- Consumes: `../examples-stm32n6/models/ch12_local_classifier/manifest.json` and its unmeasured Markdown benchmark.
- Produces: `python3 tests/test_model_gate_record.py <record.json>`, which exits 0 only for structurally valid records that obey their declared decision state.

- [ ] **Step 1: Write the checker before a record exists.**

```python
#!/usr/bin/env python3
import json
import pathlib
import sys

COMMON = {
    "candidate_name", "chapter", "compile_result", "cpu_fallback_operators",
    "weights_bytes", "activations_bytes", "inference_location", "camera_frames",
    "accuracy_metric", "npu_cycles_median", "npu_cycles_p95",
    "frame_cycles_median", "frame_cycles_p95", "fps", "local_display",
    "decision", "blockers"
}

record = json.loads(pathlib.Path(sys.argv[1]).read_text())
missing = COMMON - record.keys()
assert not missing, f"missing: {sorted(missing)}"
assert record["decision"] in {"accepted", "rejected"}
assert record["inference_location"] == "mcu"
```

- [ ] **Step 2: Verify the initial command is red.**

Run: `python3 ../examples-stm32n6/tests/test_model_gate_record.py ../examples-stm32n6/docs/benchmarks/ch12_local_classifier.json`

Expected: non-zero because the checker and record do not exist.

- [ ] **Step 3: Implement common, accepted, and rejected validation.**

Use only the Python standard library. Reject malformed JSON, a missing/extra command-line argument, wrong scalar types, a negative frame count, and a non-list `blockers` field. The checker must require non-empty strings for `candidate_name` and `compile_result`, positive `chapter`, positive `weights_bytes`/`activations_bytes`, and an integer or `null` for `cpu_fallback_operators`.

For `accepted`, assert this exact policy:

```python
assert record["compile_result"] == "passed"
assert record["camera_frames"] >= 100
assert record["local_display"] is True
assert record["blockers"] == []
assert isinstance(record["cpu_fallback_operators"], int)
for key in ("npu_cycles_median", "npu_cycles_p95", "frame_cycles_median",
            "frame_cycles_p95", "fps"):
    assert isinstance(record[key], (int, float)) and record[key] >= 0
assert isinstance(record["accuracy_metric"], str) and record["accuracy_metric"].strip()
```

For `rejected`, assert every blocker is a non-empty string. Permit `null` only for `cpu_fallback_operators`, `accuracy_metric`, the five timing/FPS fields; do not require a non-zero `camera_frames` or `local_display` value.

- [ ] **Step 4: Add the rejected classifier record.**

Create `ch12_local_classifier.json` with these non-measured facts:

```json
{
  "candidate_name": "ch12_local_classifier",
  "chapter": 12,
  "compile_result": "disposable_reference_probe_passed",
  "cpu_fallback_operators": null,
  "weights_bytes": 8283441,
  "activations_bytes": 2883576,
  "inference_location": "mcu",
  "camera_frames": 0,
  "accuracy_metric": null,
  "npu_cycles_median": null,
  "npu_cycles_p95": null,
  "frame_cycles_median": null,
  "frame_cycles_p95": null,
  "fps": null,
  "local_display": false,
  "decision": "rejected",
  "blockers": [
    "licensed_model_artifact_redistribution_not_authorized",
    "pinned_stedgeai_core_v4_0_0_not_provisioned",
    "stm32n6570_dk_imx335_lcd_flash_hardware_not_available",
    "100_camera_triggered_local_inferences_not_observed"
  ]
}
```

Add `manifest_path`, `source_sha256`, `generated_sha256`, and a `notes` string that identifies the disposable probe as compilation-only, not model evidence.

- [ ] **Step 5: Add the reusable template and foundation status page.**

The template contains every checker-required property with `decision: "rejected"`, `camera_frames: 0`, `local_display: false`, nullable unknown metrics, and one example `blockers` value. The Markdown page must state **Deferred — not accepted**, link the classifier JSON and manifest, list the completed software checks, and list the required board/model/toolchain evidence needed to change the record to `accepted`.

- [ ] **Step 6: Verify green and prove that acceptance cannot be faked.**

Run:

```bash
python3 ../examples-stm32n6/tests/test_model_gate_record.py \
  ../examples-stm32n6/docs/benchmarks/ch12_local_classifier.json
python3 - <<'PY'
import json, pathlib, subprocess, sys, tempfile
root = pathlib.Path('../examples-stm32n6')
record = json.loads((root / 'docs/benchmarks/ch12_local_classifier.json').read_text())
record['decision'] = 'accepted'
record['blockers'] = []
with tempfile.NamedTemporaryFile(mode='w', suffix='.json') as output:
    json.dump(record, output)
    output.flush()
    result = subprocess.run([sys.executable, root / 'tests/test_model_gate_record.py', output.name])
    assert result.returncode != 0
PY
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
python3 ../examples-stm32n6/tests/test_classifier_manifest.py
```

Expected: the rejected classifier record validates; changing it to accepted without hardware evidence fails; all existing host and classifier source checks pass.

- [ ] **Step 7: Commit in both repositories.**

```bash
git add docs/benchmarks docs/superpowers/plans
git commit -m "docs: record deferred N6 model evidence gate"

cd ../examples-stm32n6
git add docs/benchmarks tests/test_model_gate_record.py
git commit -m "docs: add rejected local classifier gate"
```
