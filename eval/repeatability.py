#!/usr/bin/env python3
"""Oxford-affine repeatability of the embedDIP Harris detector
(Mikolajczyk & Schmid, 2005). The detector is the SAME C code that runs on the
MCU (host==board parity is verified separately); here it runs on host at full
resolution against the dataset's ground-truth homographies.

Usage:
    python repeatability.py graf        # or: boat
Requires: eval/fetch_data.sh already run, and corner_ref built (see README).
"""
import glob
import os
import subprocess
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "corner_ref")
CAP, THR, NMS = 1500, 10000.0, 3
EPS = 3.0  # pixel localization tolerance


def detect(ppm):
    im = Image.open(ppm).convert("L")
    w, h = im.size
    raw = ppm + ".u8"
    np.asarray(im, np.uint8).tofile(raw)
    out = subprocess.run([REF, raw, str(w), str(h), str(CAP), str(THR), str(NMS)],
                         capture_output=True, text=True).stdout
    pts = np.array([[float(v) for v in ln.split()] for ln in out.splitlines() if ln],
                   dtype=np.float64)
    return pts, (w, h)


def in_bounds(p, size):
    w, h = size
    return (p[:, 0] >= 0) & (p[:, 0] < w) & (p[:, 1] >= 0) & (p[:, 1] < h)


def warp(pts, H):
    h = np.hstack([pts, np.ones((len(pts), 1))])
    w = (H @ h.T).T
    return w[:, :2] / w[:, 2:3]


def repeatability(p1, s1, p2, s2, H):
    a = warp(p1, H)[in_bounds(warp(p1, H), s2)]         # img1 pts seen in img2
    b = p2[in_bounds(warp(p2, np.linalg.inv(H)), s1)]   # img2 pts seen in img1
    if len(a) == 0 or len(b) == 0:
        return 0.0, 0, 0, 0
    used = np.zeros(len(b), bool)
    reps = 0
    for pt in a:
        d = np.hypot(b[:, 0] - pt[0], b[:, 1] - pt[1])
        j = int(np.argmin(d))
        if d[j] <= EPS and not used[j]:
            used[j] = True
            reps += 1
    return reps / min(len(a), len(b)), reps, len(a), len(b)


def img_path(d, n):
    m = glob.glob(os.path.join(d, f"img{n}.p?m"))  # .ppm (graf) or .pgm (boat)
    if not m:
        raise FileNotFoundError(f"img{n} not found in {d}")
    return m[0]


def main():
    seq = sys.argv[1] if len(sys.argv) > 1 else "graf"
    d = os.path.join(HERE, "data", seq)
    p1, s1 = detect(img_path(d, 1))
    print(f"[{seq}] img1: {len(p1)} Harris corners (cap {CAP}, thr {THR}, nms {NMS}, eps {EPS})")
    print(f"{'pair':<12}{'rep%':>7}{'#corr':>8}{'#common1':>10}{'#common2':>10}")
    for n in range(2, 7):
        pN, sN = detect(img_path(d, n))
        H = np.loadtxt(os.path.join(d, f"H1to{n}p"))
        rep, corr, na, nb = repeatability(p1, s1, pN, sN, H)
        print(f"img1-img{n:<6}{rep*100:>6.1f}{corr:>8}{na:>10}{nb:>10}")


if __name__ == "__main__":
    sys.exit(main())
