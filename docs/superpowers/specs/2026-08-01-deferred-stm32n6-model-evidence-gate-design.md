# Deferred STM32N6 Model Evidence Gate Design

**Status:** Approved design — implementation requested

## Decision

Hardware validation is deferred, not passed. The first N6 classifier retains
an explicit `rejected` model-gate record until an STM32N6570-DK, IMX335,
licensed model generation inputs, and the pinned toolchain produce the
required 100 live, camera-triggered local inferences.

## Purpose

The book needs software-only work to continue while preventing any chapter,
benchmark, or model manifest from implying that the physical gate has passed.
The evidence tooling must make the distinction machine-checkable.

## Record states

Every model-gate record contains a `decision` whose value is exactly one of:

- `accepted`: hardware evidence is complete. The checker requires MCU
  inference, `camera_frames >= 100`, `local_display == true`, non-negative
  latency measurements, and a complete build/model provenance record. Its
  manifest must report `generation_status: "generated"`; observed ST Edge AI
  and CubeN6 versions must match that manifest; the board revision must begin
  with `STM32N6570-DK`; and every evidence reference must resolve to a regular
  file inside the companion repository.
- `rejected`: the candidate is not eligible for a book claim. It records the
  attempted source/build facts, `camera_frames` may be zero, and it must list
  one or more concrete blockers. A rejected record is preserved for audit but
  cannot be cited by a chapter as supported hardware.

The first classifier record is `rejected` with `hardware_pending` blockers.
It has no invented accuracy, frame count, timing, or operator-allocation
values.

## Components and data flow

`examples-stm32n6/tests/test_model_gate_record.py` validates one JSON record.
It always requires model identity, compiler/build status, memory sizes,
inference location, candidate chapter, decision, separate NPU/CPU operator
counts, and manifest-correlated source/expected-artifact provenance. Observed
generated hash, board, and tool versions remain null while rejected. The
checker applies the stronger hardware requirements only to `accepted` records
and requires a non-empty `blockers` array for `rejected` records.

`examples-stm32n6/docs/benchmarks/ch12_local_classifier.json` is the
machine-readable rejected record for the classifier. It mirrors the existing
manifest and Markdown benchmark's unmeasured state.

`embedDIP/docs/benchmarks/stm32n6-model-gate-template.json` is a reusable
record template. `embedDIP/docs/benchmarks/evidence/` contains publication
snapshots of the companion-owned live record and manifest; the two snapshots
are updated atomically with those sources. `embedDIP/docs/benchmarks/stm32n6-foundation-gate.md`
links only these committed snapshots, describes the current software evidence,
and names the hardware conditions that must be observed before acceptance.

## Constraints

- The record must never assert an on-MCU model measurement that was not
  observed on the board.
- The current classifier keeps `inference_location: "mcu"` as its deployment
  design; this does not mean the hardware gate is accepted.
- No cloud or host inference fallback is permitted.
- The existing feasibility design remains unpromoted until an `accepted`
  record exists.
- `Book STM32/` remains untracked and unmodified.

## Verification

The new Python checker accepts the committed rejected record and rejects an
otherwise-identical `accepted` record that lacks 100 local camera frames. The
existing classifier manifest/graph checks and EmbedDIP host CTests continue to
run unchanged.
