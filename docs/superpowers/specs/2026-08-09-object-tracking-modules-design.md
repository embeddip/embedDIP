# Object Tracking Modules for embedDIP

## Context

Three standalone STM32 tracker apps exist at `../object-trackers/`: `F429_Tracker`,
`H7_KCF`, `Tracker_H7`. They implement six tracking algorithms in app-local
code (Particle, Belief, Kalman, MeanShift, TemplateMatching, Bhattacharya,
KCF), duplicated across the three apps, plus infra (`ImProc.filter2d`, custom
FFT) that already exists in embedDIP (`imgproc/filter.c`, `imgproc/fft.c`).

This spec covers porting four of those algorithms into embedDIP as
first-class `cv/` modules: **Kalman**, **Template matching**, **KCF**,
**Particle filter**. Belief propagation and Bhattacharya-distance particle
variants are dropped as near-duplicates of the plain particle filter.
MeanShift is dropped (niche, color-blob only). Dedup of `filter2d`/FFT in the
tracker apps themselves is out of scope — this spec is embedDIP-side only.

## Goals

- Add four independent tracker modules to `cv/`, following existing
  embedDIP conventions (flat C API, `embeddip_status_t` returns, Doxygen
  headers, `core/image.h` + `core/error.h`).
- No shared tracker interface/vtable — each module has its own
  `cv_<name>_init`/`cv_<name>_update` naming, own state struct. Matches how
  `haar.h`/`hog.h`/`detect.h` are independent today.
- No new external dependencies. KCF's FFT need is met by existing
  `imgproc/fft.c`. Kalman's matrix math is hand-written (state dim ≤ 6,
  no matrix library needed).
- Each module is usable standalone: caller provides an `Image`, gets back
  a bounding box / state estimate.

## Non-goals

- Board/HAL glue (DMA2D, RNG peripheral, GPIO) — stays in the app layer,
  not ported.
- Belief propagation, Bhattacharya distance, MeanShift — dropped per above.
- Runtime tracker-swapping abstraction — explicitly rejected in favor of
  independent APIs.
- CMSIS-DSP acceleration path — pure C only for now.

## Modules

### `cv/tracker_kalman.c/h`

Constant-acceleration linear Kalman filter (position, velocity,
acceleration per axis). Ported from `TrackerCommon.c`'s F/Q/R matrix setup.
State dim is fixed at compile time (2D pos+vel+accel = 6), so matrix ops are
unrolled loops, not a general matrix library.

API: `cv_kalman_init(CvKalmanState*, initial_box)`,
`cv_kalman_predict(CvKalmanState*)`, `cv_kalman_update(CvKalmanState*, measured_box)`.

### `cv/tracker_template.c/h`

Normalized cross-correlation template matcher. Ported from
`TemplateMatching.c`. Simplest tracker — fixed template, no scale/rotation,
drifts over time. Serves as baseline/fallback.

API: `cv_template_set(CvTemplateState*, const Image*, Rectangle roi)`,
`cv_template_match(CvTemplateState*, const Image* frame, Rectangle* out_box)`.

### `cv/tracker_kcf.c/h`

FFT-based Kernel Correlation Filter. Ported from `H7_KCF/KCFTracker.c`.
Replaces its hand-rolled `ifft2d`/`PolynomialCorrelation` with calls into
`imgproc/fft.c` for forward/inverse 2D FFT. Best accuracy/speed tradeoff of
the four; most integration work since kernel regularization math must be
adapted to embedDIP's FFT calling convention.

API: `cv_kcf_init(CvKcfState*, const Image*, Rectangle roi)`,
`cv_kcf_update(CvKcfState*, const Image* frame, Rectangle* out_box)`.

### `cv/tracker_particle.c/h`

Particle filter with corner-based likelihood weighting. Ported from
`Particle.c`, using the plain particle-filter formulation (not the
Belief or Bhattacharya variants). Particle count `N` is a caller-supplied
config, letting MCU budget dictate cost.

API: `cv_particle_init(CvParticleState*, N, const Image*, Rectangle roi)`,
`cv_particle_update(CvParticleState*, const Image* frame, Rectangle* out_box)`,
`cv_particle_free(CvParticleState*)` (particle buffer is heap/pool allocated).

## Data flow (typical usage)

```
Image frame
   -> cv_<tracker>_init(state, frame, initial_roi)   // once
loop:
   frame = capture()
   cv_<tracker>_update(state, frame, &box)           // per frame
   draw box via imgproc/drawing.c                     // app's choice
```

Kalman is the odd one out: it takes a *measurement* box (from an external
detector or another tracker), not a frame directly — it's a smoother/
predictor, not a standalone visual tracker. `cv_kalman_update` signature
reflects that.

## Error handling

Follow `core/error.h` conventions: `embeddip_status_t` return codes
(`EMBEDDIP_OK`, `EMBEDDIP_ERR_INVALID_ARG`, etc.) for init/update failures
(null pointers, zero-size ROI, allocation failure in particle filter).
No exceptions, no aborts — matches rest of `cv/`.

## Testing

One self-check per module (ponytail-style, no framework): synthetic image
with a known moving block, run init + a few update() calls, assert the
returned box tracks within tolerance. Mirrors how `detect.c`/`haar.c` are
likely tested today — confirm during implementation planning and match
existing test file location (`tests/`).

## Open questions / risks

- KCF's kernel math (Gaussian/polynomial kernel correlation) needs careful
  adaptation to `imgproc/fft.c`'s API (real vs complex buffer layout) —
  flag for the implementation plan to spike first if `fft.c`'s interface
  doesn't cleanly support the required convolution-theorem usage.
- Particle filter's corner-detection likelihood step in the original uses
  Sobel/Harris from `ImProc.c`. `imgproc/filter.h` already exposes a Sobel
  aperture parameter, so gradient computation is covered; Harris response
  itself is not a separate `imgproc` primitive today. Plan: build the
  Harris corner score locally inside `tracker_particle.c` from
  `imgproc/filter.c`'s Sobel output rather than adding a new `imgproc`
  module — keeps the addition scoped to the tracker.
