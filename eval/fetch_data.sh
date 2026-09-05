#!/usr/bin/env bash
# Fetch benchmark test data for embedDIP corner/keypoint validation.
# Data is NOT committed (large binaries); this downloads it reproducibly and
# verifies each file against a pinned SHA-256. Re-runnable / idempotent.
set -euo pipefail
cd "$(dirname "$0")"
DATA=data
mkdir -p "$DATA"

# name|url|sha256
ITEMS=(
"graf.tar.gz|https://www.robots.ox.ac.uk/~vgg/research/affine/det_eval_files/graf.tar.gz|999871b945ee968a00a0d5f9af957d1382fb9dae1511cdee9553366817b53b5b"
"boat.tar.gz|https://www.robots.ox.ac.uk/~vgg/research/affine/det_eval_files/boat.tar.gz|6e5721f37fbf9c3e974fd05935a42c1670f3e71aa3e5b218807ea046a068c603"
"camera.png|https://raw.githubusercontent.com/scikit-image/scikit-image/v0.19.0/skimage/data/camera.png|b0793d2adda0fa6ae899c03989482bff9a42d3d5690fc7e3648f2795d730c23a"
"astronaut.png|https://raw.githubusercontent.com/scikit-image/scikit-image/v0.19.0/skimage/data/astronaut.png|88431cd9653ccd539741b555fb0a46b61558b301d4110412b5bc28b5e3ea6cb5"
"brick.png|https://raw.githubusercontent.com/scikit-image/scikit-image/v0.19.0/skimage/data/brick.png|7966caf324f6ba843118d98f7a07746d22f6a343430add0233eca5f6eaaa8fcf"
"chessboard.png|https://raw.githubusercontent.com/opencv/opencv/4.10.0/samples/data/chessboard.png|7f5ce40e4fa457ad05d84b247f62ef1a3fc4f5fdb09f15a6c8c5a558784c59e7"
"building.jpg|https://raw.githubusercontent.com/opencv/opencv/4.10.0/samples/data/building.jpg|742a1baad62ac82e91e718e77eedf7e85c2eddc4badfb8c87c6cbc86c45a8b07"
)

for item in "${ITEMS[@]}"; do
    IFS='|' read -r name url sha <<<"$item"
    out="$DATA/$name"
    if [ -f "$out" ] && echo "$sha  $out" | sha256sum -c --status 2>/dev/null; then
        echo "ok   $name (cached)"
        continue
    fi
    echo "get  $name"
    curl -fsSL -m 120 "$url" -o "$out"
    echo "$sha  $out" | sha256sum -c --status || { echo "SHA MISMATCH: $name" >&2; exit 1; }
done

# Extract Oxford sequences into their own dirs (img1..6.ppm + H1to*p).
for seq in graf boat; do
    mkdir -p "$DATA/$seq"
    tar xzf "$DATA/$seq.tar.gz" -C "$DATA/$seq"
done
echo "all data verified in $DATA/"
