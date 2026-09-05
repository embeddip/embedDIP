# STM32N6 Computer Vision Book Feasibility Design

**Status:** Approved direction — detailed implementation plan pending review

**Decision:** The new book promotes one target: **STM32N6570-DK**. Every neural-network example executes inference locally on that MCU. A host computer is used only to prepare data, train, convert and flash models, or visualize results; it is never part of the deployed inference path.

## 1. Why the book moves from STM32F7 to STM32N6

The existing F7 material proves that EmbedDIP can teach image processing and small on-device neural networks. It cannot, however, make the new book's deep-vision material routine and reproducible at useful image sizes. The new book explicitly covers CNNs, semantic segmentation, object detection, vision transformers, tracking, scene analysis, 3-D vision, and vision-language models. Those topics need a camera-first AI platform rather than a faster version of the previous board.

The STM32N6570-DK is the correct single promoted board because it combines the following on one development kit:

- STM32N6 Cortex-M55 MCU with the Neural-ART NPU;
- 4.2 MB contiguous SRAM, 256 Mbit PSRAM, and 1 Gbit external flash;
- an MB1854B IMX335 camera module, LCD, MIPI CSI-2, DCMIPP/ISP, DMA2D, and display support;
- a maintained ST Edge AI workflow that generates NPU-targeted code and model artifacts.

This changes the book's central claim from “small ML can be demonstrated on an MCU” to “complete vision-model inference runs on an MCU.” ST's reference N6 classification project demonstrates exactly this architecture: camera preprocessing through DCMIPP, local NPU inference, and display output. Its included EfficientNet-v2 B1 model is reported at 44 ms on the N6570-DK.

STM32F7 remains a legacy EmbedDIP target. STM32H7 may become an optional compatibility target, but neither is mentioned as the reference hardware in the new book.

**Sources:** [N6570-DK hardware](https://www.st.com/en/evaluation-tools/stm32n6570-dk), [N6 AI ecosystem](https://www.st.com/en/development-tools/stm32n6-ai.html), [ST N6 on-device classification reference](https://github.com/STMicroelectronics/STM32N6-GettingStarted-ImageClassification), and [ST Model Zoo services](https://github.com/STMicroelectronics/stm32ai-modelzoo-services).

## 2. Existing assets to preserve

The existing [examples-stm32](https://github.com/embeddip/examples-stm32) repository is a substantial asset, not a disposable F7 prototype.

| Existing asset | What it proves | N6 migration decision |
| --- | --- | --- |
| Numbered C and C++ listing applications | The examples map directly to book listings and can be read independently | Preserve numbered C/C++ applications and their book-to-source mapping |
| CMake presets and CubeMX-derived project | The material can be built with a reproducible embedded toolchain | Retain CMake and CubeIDE entry points; pin CubeN6 and ST Edge AI versions |
| Per-listing build script | Every printed listing can be compiled | Replace the current `main.c` replacement script with one CMake target per listing, allowing clean independent and parallel builds |
| Camera, display, UART, JPEG, SDRAM integration | End-to-end F7 image workflows already exist | Replace F7-specific DCMI/OV5640/SDRAM assumptions with N6 camera middleware, DCMIPP, external-flash model loading, and cache-safe buffers |
| `main-c-ai-apps` and `main-cpp-ai-apps` branches | Generated X-CUBE-AI models already run locally on F7 | Migrate their instructional pattern, not their generated F7 artifacts |

The F7 AI branches already contain local X-CUBE-AI examples for MNIST dense/CNN classification, U-Net segmentation, a MobileNetV2 CamVid segmentation variant, MobileNet classification, and TensorFlow Lite Micro C++ examples. For example, the F7 `camvid_int8` model uses a 128×128×3 int8 input, 84.7 M MACCs, 57.6 KiB weights, and a 316 KiB activation buffer. That is strong evidence for the intended pedagogical flow, but it also shows the current limitation: activation buffers are placed at hard-coded F7 SDRAM addresses and live camera calls are often commented out in favor of UART input.

The N6 version must make **live camera → hardware preprocessing → local model → local postprocessing → LCD** the default path. UART image injection stays only as an optional debugging aid.

## 3. Feasibility by book chapter

The later chapters are still outlines. Their named subjects are commitments, but their exact algorithm choices must be fixed before implementation. The table therefore identifies the minimum deliverable that makes each subject real on the N6.

| Chapters / book subject | Feasibility | EmbedDIP and example deliverable |
| --- | --- | --- |
| 1–5: embedded CV, hardware, software, image representation, acquisition | Feasible | N6 board profile, Cortex-M55 profile, image/tensor-aware buffers, N6 camera and display adapters, capture-to-display listing |
| 6: spatial filters, Viola-Jones, Haar, HOG | Feasible | Optimized filtering, integral images, Haar feature evaluator/cascade format, HOG descriptor and linear classifier support |
| 7: edges, corners, line segments, keypoints | Feasible | Preserve Canny; add Harris/Shi-Tomasi or FAST, line-segment extraction, binary keypoints/descriptors, matching API |
| 8: image recognition | Feasible | Classical descriptor/HOG recognition pipeline plus local NPU classifier application and evaluation helpers |
| 9: traditional object detection | Feasible | Contours/components, Haar or HOG detector path, region filtering, drawing and result structures; no dependency on a neural runtime |
| 10: neural-network fundamentals | Feasible | Training remains PC material; EmbedDIP supplies tensor preparation, quantization explanation helpers, and reproducible N6 deployment manifests |
| 11: embedding a model | Feasible | `cv_runtime` abstraction with an ST Edge AI/NPU backend, generated-artifact import rules, live-camera inference listing, cycle and memory reporting |
| 12: CNNs and transfer learning | Feasible | Int8 local classifiers selected from model-zoo-compatible CNNs; preprocessing, top-k, labels, benchmark harness |
| 13: neural segmentation | Feasible | NPU segmentation runner, argmax/palette/overlay postprocessing, local road-scene or person-segmentation demo |
| 14: neural object detection | Feasible | NPU detector runners, model-specific decoder interface, quantization metadata, NMS, overlay and tracking handoff. Use an N6-proven YOLO/MobileNet/FOMO-class model, not a desktop model forced onto the device. |
| 15: vision transformers | Feasible with a model gate | A compact, int8, compiler-supported transformer is required. The selected model must pass NPU compilation, memory, accuracy, and latency gates before this chapter's examples are committed. |
| 16: object tracking | Feasible | KLT optical flow and Kalman/filter-association tracker; detector-to-tracker interface for local NPU detections |
| 17: scene analysis | Feasible as composition | Typed scene results that combine detection, segmentation, tracking, zones, and rules; at least one local end-to-end safety/traffic/people-flow application |
| 18: image understanding | Feasible as composition | On-device semantic results and application rules built from the Chapter 13–17 outputs; this is a pipeline layer, not an undefined monolithic algorithm |
| 19: 3-D vision | Feasible with bounded scope | Baseline: local monocular-depth or depth-sensor fusion application. Full stereo needs a separately proven synchronized-camera acquisition design; the supplied DK reference is a one-camera system. Do not promise stereo until this hardware spike passes. |
| 20: vision-language models | Research gate, MCU-only | The deployed model must run locally. Baseline is a compact image–text matching or fixed-prompt visual-question model with an on-MCU tokenizer/response decoder. A free-form generative captioning VLM is optional only after an N6 compilation, memory, and latency spike succeeds. There is no network fallback. |

The N6 is materially better aligned to the deep-vision rows. ST's maintained Model Zoo currently provides N6 deployment paths for several object detectors, semantic and instance segmentation, re-identification, and related applications, while its H747 target matrix is far more limited. The book must still select models based on measured N6 artifacts, rather than treating an NPU as a guarantee that every ONNX or TFLite model will compile.

## 4. Target architecture

EmbedDIP remains an embedded computer-vision library, not a replacement for ST Edge AI, TensorFlow Lite Micro, model-training frameworks, or a general-purpose neural runtime. It provides reusable image/CV operations and a stable boundary around vendor-generated inference code.

| Layer | Responsibility | Key design rules |
| --- | --- | --- |
| `core/` | Image, image view, ROI, stride, plane and buffer metadata | Add non-owning views and explicit location/alignment/cache attributes; retain existing C API compatibility where practical |
| `imgproc/` | Existing image-processing operators | Make kernels stride-aware and suitable for N6 scalar/MVE/CMSIS-DSP optimization; preserve current API names until a migration period ends |
| New `cv/` modules | Classical CV: integral image, descriptors, features, geometry, tracking, calibration/depth, detectors | Every algorithm has bounded workspace requirements and a host golden-image test |
| New `runtime/` module | Common tensor metadata, model manifest loading, timing, quantization, output dispatch | It never implements convolutions; it calls an optional backend and owns only portable preprocessing/postprocessing |
| `runtime/stedgeai_n6/` | N6 STAI/Neural-ART backend | Generated code and binary weights remain model artifacts; the adapter must not hard-code a model's names, tensor shape, or memory address |
| `arch/arm/cm55/` | Cortex-M55/MVE and CMSIS-DSP configuration | Separate from existing CM7 code; no F7 HAL includes |
| `board/stm32n6/` and device adapters | N6570-DK memory regions, D-cache maintenance, CMW camera/DCMIPP, LCD, timing | Use named allocator regions and cache operations, never raw F7-style addresses in algorithms or examples |
| `examples-stm32n6/` companion repository | Book-facing C/C++ applications, generated CubeN6 support, model manifests, build/flash scripts, performance results | One app target per listing; each app can be compiled, flashed, and measured in isolation |

The N6 camera adapter should model the N6 hardware's two output pipes: one display frame and one cropped/decimated/scaled inference frame. For supported input formats, the inference pipe writes the exact model input dimensions and layout, eliminating a CPU resize/copy from the normal camera path.

### Buffer and memory policy

The existing F7 allocator reserves a camera/display area in a fixed 8 MB SDRAM address range. This cannot move unchanged to N6. N6 support needs named regions such as:

- **camera/display DMA buffers** with required alignment and explicit clean/invalidate operations;
- **fast internal SRAM** for latency-sensitive CPU and NPU workspaces;
- **PSRAM** for large classical-CV workspaces or double buffers when speed permits;
- **external flash** for signed firmware and NPU model weights/binaries.

The N6 has no internal flash. The examples therefore need a documented development-mode path and a reproducible external-flash boot path, including model-weight placement. This is a real onboarding cost and must be taught early in the book rather than hidden in an appendix.

## 5. Local-model deployment contract

Every book model is accepted only when all of the following are true:

1. The source model, dataset license, label map, training recipe, quantization recipe, and converter version are recorded.
2. ST Edge AI analysis/generation succeeds for the N6 target and records any CPU fallback operators.
3. The application executes inference from the N6 camera input and presents results on the N6 LCD or local serial log.
4. Model weights, activations, and application binary fit their assigned N6 memory regions.
5. The application reports on-target inference latency, end-to-end frame latency, FPS, peak workspace, flash/PSRAM use, and accuracy metric.
6. A fixed test-image set produces results within defined tolerances on both host reference tests and hardware.

Each model application has a committed manifest containing at least model hash, input/output tensors, class map, preprocessing, postprocessing, ST Edge AI version/profile, generated-artifact hash, memory placement, benchmark result, and license. Generated NPU artifacts may be regenerated by a pinned script, but a release must include the exact binary needed to flash and reproduce the listing.

## 6. Example-program structure

The N6 examples should retain the approachable one-listing-one-program style from `examples-stm32`, with these changes:

- Store C and C++ listings in one repository tree, not mutually exclusive long-lived branches.
- Give each listing its own CMake target and build preset; do not copy over a shared `main.c` to build it.
- Supply a small common application layer for board startup, camera, display, model manifest, result overlay, timing, and error reporting.
- Make live camera operation the default. File/UART image loading is explicitly labelled as a regression/debug path.
- Keep each neural model's generated ST Edge AI files under a model-specific directory and regenerate them only through a version-pinned script.
- Publish a compact benchmark table with each example's model input, parameter/weight size, activation size, NPU/CPU operator allocation, accuracy, inference latency, and end-to-end FPS.

This preserves the current book's strongest qualities—direct C/C++ listings and independently buildable examples—while preventing board setup and generated-model boilerplate from being copied into every chapter.

## 7. Delivery roadmap

The estimates assume one experienced embedded-CV engineer with access to the N6570-DK and the required ST tooling. They are sequencing estimates, not a promise of calendar duration; model choice, data collection, and Chapter 19/20 research can change the total.

| Phase | Deliverable | Estimate |
| --- | --- | --- |
| 0. N6 proof spikes | Flash/boot workflow; live IMX335 capture and LCD preview; local NPU classifier; 3-D scope decision; transformer and VLM candidate compilation/memory tests | 4–6 weeks |
| 1. Platform foundation | CM55/N6 CMake profiles, DMA/cache-aware allocator, camera/display adapters, tensor/image views, host-test harness | 5–7 weeks |
| 2. Classical CV core | Chapter 6–9 routines and golden tests; first camera-driven classical applications | 8–12 weeks |
| 3. On-device ML integration | Generic runtime API, ST Edge AI backend, manifest/generation flow, Chapter 10–13 applications and measurement tooling | 6–9 weeks |
| 4. Advanced local vision | Chapter 14–18 detection, compact transformer, tracking, scene-understanding applications | 8–12 weeks |
| 5. 3-D and VLM commitments | Complete only the Phase 0-proven Chapter 19 path and the all-local Chapter 20 model; document measured limits | 6–12 weeks |
| 6. Publication hardening | Build every listing, flash/execute benchmark applications, lock tool versions/model artifacts, write migration and troubleshooting documentation | 6–8 weeks |

Expected effort is roughly **10–14 engineer-months** for a complete, measured library-and-examples program. The lower end assumes compact, known-supported models and a monocular/depth-sensor Chapter 19; the upper end includes a successful local compact VLM and a more ambitious 3-D capture design.

## 8. Risks and decisions that must be made early

| Risk | Consequence | Required mitigation / decision |
| --- | --- | --- |
| NPU compiler/operator limitations | A visually attractive model may not deploy or may use unacceptable CPU fallback | Run ST Edge AI analysis and on-board benchmark before writing any final listing; retain a pre-approved fallback model that still runs locally |
| N6 external-flash boot flow | A correct application may not be reproducible after reset | Ship signed-flash scripts, FSBL instructions, generated weight images, and a one-command verification target |
| New camera middleware and cache coherency | Intermittent frames or stale inference input | Establish one validated DCMIPP/camera buffer contract before adding algorithms; test capture, display, and inference concurrently |
| Chapter 15 model choice | ViT may exceed memory or expose unsupported attention operators | Treat compact-transformer selection as a Phase 0 gate; publish only a model with measured local results |
| Chapter 19 scope | The DK's default configuration provides one camera, not a ready-made stereo rig | Make monocular depth or ToF fusion the baseline; approve a separate synchronized stereo design only after hardware proof |
| Chapter 20 wording | “VLM” can imply a desktop-scale generative system | Promise only the locally benchmarked image-language model. General free-form generation is not included unless the N6 spike proves it with usable memory and latency. No cloud alternative is allowed. |
| Existing untested library surface | New routines could inherit latent API/memory defects | Add host golden tests and board benchmarks before porting high-level applications |

## 9. Success criteria

The program is complete when:

- a reader can buy one STM32N6570-DK, flash a documented image, and run the representative application for every completed chapter topic;
- every neural example performs all inference locally on the MCU and reports measured on-board performance;
- EmbedDIP compiles independently for host test, F7 legacy, and N6 targets without F7-specific headers leaking into portable code;
- every published listing is built by CI or a reproducible build command, and every hardware-dependent listing has a recorded board validation result;
- generated model artifacts can be reproduced from their manifest and pinned ST Edge AI version;
- Chapters 15, 19, and 20 contain only claims demonstrated by the designated N6 hardware gates.

