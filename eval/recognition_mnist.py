#!/usr/bin/env python3
"""MNIST digit-recognition accuracy of embedDIP's kNN (cv/knn.c) — the SAME C
that runs on the MCU — on a labeled benchmark (real ground truth).

Downsamples digits to 14x14 float features, runs kNN via knn_ref, reports test
accuracy. MNIST is fetched by keras (cached in ~/.keras), not fetch_data.sh.

Usage: python recognition_mnist.py [n_train] [n_test] [k]
"""
import os
import subprocess
import sys

import numpy as np
from tensorflow import keras

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "knn_ref")


def features(x):
    """28x28 uint8 -> 14x14 float32 (2x2 average), flattened, /255."""
    x = x.reshape(-1, 14, 2, 14, 2).mean(axis=(2, 4))
    return (x.reshape(len(x), -1) / 255.0).astype(np.float32)


def main():
    n_train = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
    n_test = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
    k = int(sys.argv[3]) if len(sys.argv) > 3 else 3

    (xtr, ytr), (xte, yte) = keras.datasets.mnist.load_data()
    ftr = features(xtr[:n_train]); ltr = ytr[:n_train].astype(np.int32)
    fte = features(xte[:n_test]); lte = yte[:n_test].astype(np.int32)
    dim = ftr.shape[1]

    d = "/tmp/knn_mnist"
    os.makedirs(d, exist_ok=True)
    ftr.tofile(f"{d}/train.f32"); ltr.tofile(f"{d}/trainlab.i32")
    fte.tofile(f"{d}/test.f32"); lte.tofile(f"{d}/testlab.i32")

    out = subprocess.run(
        [REF, f"{d}/train.f32", f"{d}/trainlab.i32", f"{d}/test.f32", f"{d}/testlab.i32",
         str(n_train), str(n_test), str(dim), str(k)],
        capture_output=True, text=True).stdout.strip()
    correct, total, acc = out.split()
    print(f"MNIST kNN (k={k}, train={n_train}, test={n_test}, dim={dim}=14x14): "
          f"{correct}/{total} = {acc}")


if __name__ == "__main__":
    sys.exit(main())
