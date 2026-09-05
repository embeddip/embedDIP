# embedDIP Tracker Method Expansion — Design

Date: 2026-09-03
Status: Draft for review
Related: `survey.txt` (Object Tracking on Microcontrollers at the Extreme Edge)

## Motivation

The survey catalogues the classical and learning-based trackers that fit MCU
constraints and rates each by platform class (M1/M2/M3). Mapping it against the
embedDIP `cv/` module shows four high-value gaps, all of which have tested
reference implementations already living in `object-trackers/` (the pre-library
CubeIDE projects F429_Tracker, Tracker_H7, H7_KCF — the ref [1] probabilistic
tracking work):

| Survey method | Current embedDIP state |
|---|---|
| SIR particle filter with cheap likelihood (M2, ref [1]) | `cv/tracker_particle` uses a weak edge-magnitude weighting, not an appearance likelihood |
| KCF / DCF with online update (M2) | `cv/tracker_kcf` learns the template spectrum once at init; `update()` only correlates — no adaptation (survey axis A3) |
| Mean shift / CAMShift (M1, best accuracy-per-byte classical) | absent |
| Tiny detector + IoU/Kalman association / SORT (M1) | `cv/tracker_kalman` exists; no association stage |

The goal is to close these gaps by **porting** the reference algorithms into the
embedDIP library contract (not rewriting from scratch, not leaving them as
app-level code), then validating on real M2 hardware (STM32H7) over PyDIPLink.
Work is phased: library methods first (they unlock everything downstream), a
measurement/benchmark harness second.

## Non-goals

- Deep trackers requiring a training pipeline (Siamese, GOTURN, learned
  filters) — the runtime can infer but no models are bundled; out of scope here.
- Event-driven / SNN trackers — need an event sensor and immature tooling.
- The Phase 2 benchmark harness is scoped and deferred to its own spec; this
  document only fixes its shape.
- Hungarian assignment for W1 (greedy is the starting point; see W1).

## Contract every new or changed `cv/` module must hold

Matches the existing `cv/` trackers and the module's stated conventions:

1. **Input format.** Geometry/intensity trackers (W1 association, W4 KCF
   correlation) take grayscale `ImageView` (`IMAGE_FORMAT_GRAYSCALE`/`MASK`,
   `IMAGE_DEPTH_U8`, `row_stride_bytes >= width`) — color buys them nothing.
   The histogram trackers (W2, W3) accept **both grayscale and color** and pick
   the histogram bin scheme from `ImageView.format`. **RGB565 is the color
   path** — 2 bytes/pixel, the format the OV5640 and the display already use, so
   half the frame memory of ARGB8888 for the same discriminative gain; each 5/6/5
   channel is quantized to a fixed bin count. A 1-D histogram is used for
   grayscale input. ARGB8888 is accepted but not the target. Color is the
   recommended path for W2/W3 where SRAM allows; grayscale is the fallback. See
   Risks for the peak-SRAM trade-off.
2. **No new per-frame allocation.** State lives in a caller-visible struct;
   working memory is a caller-owned scratch buffer passed in. Reference globals
   (`q[1000]`, `template_hist[384]`, `particles[][]`, `weights[]`) become state
   fields. (`cv/tracker_kcf` already allocates FFT `Image`s via the imgproc/fft
   path; W4 reuses those buffers rather than adding new allocation.)
3. **Portable math in the core path.** Plain C / `<math.h>` so the host test
   suite builds and runs off-target. Reference `arm_*` (CMSIS-DSP) calls are
   replaced with portable equivalents; any SIMD acceleration goes behind `arch/`
   as an optional path, never in the portable core.
4. **One assert-based host test** per module in `tests/`, wired into
   `tests/CMakeLists.txt`, following `test_cv_tracker_*.c`. Written before the
   implementation (TDD).
5. **Doc-comment header** matching the Doxygen style of existing `cv/*.h`.

## Phase 1 — work items

Implemented in this priority order, each landing with its test and (where
relevant) demo before the next begins (approach A: incremental, host-first).

### W1 — SORT-style association (`cv/track_assoc.{c,h}`, new)

- IoU matrix between a set of `CvDetection` (from `cv/detect`, fed by the
  existing Haar cascade) and a set of active tracks.
- **Greedy** assignment (highest-IoU-first) above a threshold. Unmatched
  detections spawn tracks; unmatched tracks age out after N misses.
- Each track owns a `CvKalmanState` for predict/correct between detections.
- `ponytail:` greedy assignment, O(n·m) scan. Upgrade to Hungarian only if a
  measured ID-switch rate proves it necessary.
- Test: synthetic detection streams across frames (crossing, occlusion gap);
  assert track IDs stay stable and survive a missed-detection frame.

### W2 — Particle filter appearance likelihood (`cv/tracker_particle`, extend)

- Add a pluggable likelihood: function pointer + context, so the edge-magnitude
  weighting stays available and a histogram Bhattacharyya likelihood becomes the
  recommended default.
- Port from `Bhattacharya.c`: `ColorHist` + `BhattacharyaDistance` (portable
  `sqrt`/`exp`, no `arm_*`). For RGB565 color input, quantize each 5/6/5 channel
  to a fixed bin count (reference used 3×128 on 8-bit channels; RGB565 needs
  fewer, e.g. 3×32); fold to a 1-D histogram for grayscale input.
- State gains the target template histogram; particle buffer stays caller-owned.
- Keep/complete the existing SIR resample.
- Test: target patch with a distinct color/intensity distribution on a
  contrasting background; assert the weighted-mean estimate converges to the
  moving target. Cover both a color and a grayscale frame.

### W3 — Mean shift tracker (`cv/tracker_meanshift.{c,h}`, new)

- Port `MeanShift.c`: target histogram (color when supplied, 1-D for grayscale),
  per-pixel backprojection weight, iterative mode-seek (classic mean-shift
  centroid update; drop the Sobel-gradient variant unless the centroid form
  underperforms).
- Add a histogram backprojection helper (in the module, or `imgproc/histogram`
  if it generalises cleanly).
- Caller-owned kernel/weight scratch. Bounded iteration count.
- Document the survey caveat: a 1-D grayscale histogram confuses target and
  background sharing intensity statistics — color mitigates this where the SRAM
  budget allows it.
- Test: color/intensity blob translating across frames; assert the tracked
  center follows within tolerance.

### W4 — KCF online update (`cv/tracker_kcf`, fix)

- Make `update()` adapt: interpolate the learned template spectrum toward the
  current patch with learning rate η, keeping numerator/denominator (or the
  equivalent template spectrum) in higher precision than the input (survey
  caveat ii).
- Reference: `H7_KCF/KCFTracker.c` for the update math; reuse the existing
  FFT `Image` buffers, add no new per-frame allocation.
- Test: target whose appearance drifts over frames; assert lock is maintained
  where the current fixed-template version would lose it.

### W5 — deferred (note only)

`Belief.c` is a Harris-corner + generalized-Hough keypoint-voting consensus
tracker — it maps to the survey's "keypoint voting / GHT" family, another gap.
Larger and color/grayscale-mixed; gets its own spec after W1–W4.

## Phase 1 — hardware validation

- Host tests are the primary correctness gate (run off-target in CI-style).
- On-hardware demo: a **new CubeIDE project under `object-trackers/`** (templated
  from `Tracker_H7`) that links the embedDIP library modules and runs on the
  STM32H7 (M2 class). Frames are streamed from the PC via **PyDIPLink** using the
  existing `FROM_PC` / `Sequences.c` pattern; the board runs the selected tracker
  and returns the state/box for overlay on the host.
- W1's demo uses the **Haar cascade** (`cv/detect`) as the detection source —
  pure classical, no NPU, runs on H7 M2 end-to-end.

## Phase 2 — measurement harness (deferred, separate spec)

Realises survey direction D1 / the reporting protocol:

- PyDIPLink-driven "MCU-ified" protocol: grayscale, QVGA/QQVGA, temporal
  subsample at 30/15/5 Hz.
- Per-frame metrics collected on-board: latency via the DWT cycle counter
  (mean / p95 / worst), peak SRAM via stack-watermark, energy via board shunt
  where the hardware allows.
- Report accuracy as a function of resolution and frame rate, not a single point.

## Risks

- **Color vs peak SRAM.** Color makes W2/W3 markedly more discriminative and is
  the recommended path. Frame sizes: QVGA grayscale ~76.8 KB, QVGA RGB565
  ~150 KB, QVGA ARGB8888 ~305 KB (QQVGA RGB565 ~38 KB). RGB565 is chosen as the
  color path to keep peak SRAM affordable — fine on H7 M2 (≥1 MB SRAM), still
  too heavy for M1 at QVGA. The histogram trackers support both color and
  grayscale; the deployment picks per its SRAM budget, and the grayscale path
  documents that a 1-D histogram is weaker.
- **KCF update stability under fixed/limited precision.** η and periodic
  renormalisation may be needed to bound drift (survey D2). Watch in W4's test
  over a long synthetic sequence.
- **Reference-to-contract drift.** malloc→caller-buffer and color→grayscale are
  real edits, not copy-paste; each port is validated against its host test, not
  against the reference's on-target behaviour.
