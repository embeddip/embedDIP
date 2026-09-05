# embedDIP evaluation harness

Quantitative, ground-truth validation for the corner / keypoint detectors
(`imgproc/corner.c`, `cv/keypoint.c`). Test **data is not committed** — it is
downloaded reproducibly and checksum-verified by `fetch_data.sh`. Only the code
and pinned URLs+hashes are tracked here.

## Data (`fetch_data.sh`)

Pinned + SHA-256 verified:
- **Oxford Affine Covariant Features** (Mikolajczyk & Schmid, 2005): `graf`
  (viewpoint) and `boat` (scale+rotation) sequences — 6 images each plus
  ground-truth homographies `H1to{2..6}p`.
- Classic single images: `camera` (cameraman), `astronaut`, `brick`,
  `chessboard`, `building` — for on-device montages.

```sh
./fetch_data.sh          # -> eval/data/ (gitignored)
```

## Repeatability (paper metric)

```sh
# build the host reference (== the code that runs on the MCU)
cc -I.. corner_ref.c ../build/libembedDIP.a -lm -o corner_ref
python3 repeatability.py graf     # or: boat
```

`corner_ref` runs `cv_corner_detect` (Harris) on a raw grayscale frame; the
Python script warps img1 corners into imgN by the ground-truth homography and
reports repeatability (eps=3px, common region), the standard Mikolajczyk-Schmid
metric.

## Descriptor matching (ORB orientation)

```sh
cc -I.. kp_match_ref.c ../build/libembedDIP.a -lm -o kp_match_ref
python3 matching_score.py boat    # or: graf
```

Reports matching precision for plain BRIEF vs oriented rBRIEF vs scale-invariant
ORB pyramid (`cv_orb_detect_and_describe`); correctness is decided by the
ground-truth homography. See `docs/results/RESULTS.md`.

## Image recognition (kNN)

```sh
cc -I.. knn_ref.c ../build/libembedDIP.a -lm -o knn_ref
python3 recognition_mnist.py 3000 1000 3   # n_train n_test k
```

MNIST digit recognition accuracy of `cv_knn_classify_f32` (14x14 features);
ground truth = dataset labels. MNIST is fetched by keras (cached in ~/.keras),
not by `fetch_data.sh`. Reference: ~91% with 3000 train / k=3.

## Board parity + on-device montages

Board-vs-host parity and the real-image montages are driven from
`examples-stm32h7/tools/corner_host.py` (flashes/streams to the STM32H7S78-DK
`corner_uart` app). Board output is bit-identical to `corner_ref` on the same
bytes — so the repeatability numbers above characterise the on-device detector.

See `docs/results/RESULTS.md` for recorded numbers and images.
