# STM32N6 foundation model-evidence gate

Status: **Deferred — not accepted.**

The published classifier gate snapshot is [rejected](evidence/ch12_local_classifier.json).
Its deployment design and source/build provenance snapshot are in the [classifier manifest](evidence/ch12_local_classifier_manifest.json).
Neither document is evidence of an observed hardware result.

The companion repository owns the live record and manifest. These committed
snapshots are updated atomically with their companion sources for publication,
so this repository's evidence links remain self-contained.

## Completed software checks

- The local-classifier manifest and generated binding source are checked for their
  documented model identity, tensor contract, memory plan, and MCU-only design.
- A disposable compilation-only reference probe completed; it does not establish
  model provenance, camera inference, accuracy, latency, FPS, or local display.
- The model-gate checker validates the rejected record and prevents the same
  unmeasured record from being promoted to `accepted`.

## Evidence required for acceptance

To change the record to `accepted`, obtain the licensed model-generation inputs
and pinned ST Edge AI Core v4.0.0 toolchain, then use an STM32N6570-DK with its
IMX335 camera, LCD, and persistent-flash setup to observe and record all of:

- a reproducible generated-model provenance record with separate NPU and CPU
  fallback operator counts;
- at least 100 camera-triggered, on-MCU inferences from IMX335 pipe 2;
- a locally displayed result for those inferences, with no network fallback;
- accuracy, median and p95 NPU/end-to-end cycles, and FPS; and
- the board/toolchain details plus a clean cache-coherency and stale-frame audit.

The rejected snapshot distinguishes the manifest-pinned expected generated hash
from the still-unobserved generated artifact hash, which remains `null`.
