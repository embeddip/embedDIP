#!/usr/bin/env python3
"""Descriptor matching score on the Oxford-affine dataset: plain BRIEF vs
oriented rBRIEF (ORB-style), using the SAME C code that runs on the MCU.

For each img1<->imgN pair: nearest-neighbour match img1 descriptors to imgN by
Hamming distance with Lowe's ratio test; a match is CORRECT if the ground-truth
homography maps the img1 keypoint to within EPS px of the matched imgN keypoint.
Reports precision (#correct / #matches) for plain and oriented descriptors.

Usage: python matching_score.py boat     # or: graf
"""
import glob
import os
import subprocess
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "kp_match_ref")
CAP, THR, EPS, RATIO = 1500, 20, 3.0, 0.8


def detect(ppm, mode):
    """mode: 0 plain BRIEF, 1 oriented rBRIEF, 2 scale-invariant ORB pyramid."""
    im = Image.open(ppm).convert("L")
    w, h = im.size
    raw = ppm + ".u8"
    np.asarray(im, np.uint8).tofile(raw)
    out = subprocess.run([REF, raw, str(w), str(h), str(int(mode)), str(CAP), str(THR)],
                         capture_output=True, text=True).stdout
    xy, desc = [], []
    for ln in out.splitlines():
        p = ln.split()
        if len(p) != 4:
            continue
        xy.append((float(p[0]), float(p[1])))
        desc.append(np.frombuffer(bytes.fromhex(p[3]), dtype=np.uint8))
    return np.array(xy), np.array(desc, dtype=np.uint8), (w, h)


_POP = np.array([bin(i).count("1") for i in range(256)], np.uint16)


def match(dq, dt):
    """NN + ratio test. Returns list of (qi, ti)."""
    out = []
    for i in range(len(dq)):
        d = _POP[np.bitwise_xor(dq[i], dt)].sum(1)
        j = np.argpartition(d, 2)[:2]
        j = j[np.argsort(d[j])]
        best, second = d[j[0]], d[j[1]]
        if second == 0 or best / second < RATIO:
            out.append((i, int(j[0])))
    return out


def warp(pts, H):
    h = np.hstack([pts, np.ones((len(pts), 1))])
    w = (H @ h.T).T
    return w[:, :2] / w[:, 2:3]


def score(seq, mode):
    d = os.path.join(HERE, "data", seq)
    def img(n):
        return glob.glob(os.path.join(d, f"img{n}.p?m"))[0]
    q_xy, q_d, _ = detect(img(1), mode)
    rows = []
    for n in range(2, 7):
        t_xy, t_d, _ = detect(img(n), mode)
        H = np.loadtxt(os.path.join(d, f"H1to{n}p"))
        m = match(q_d, t_d)
        if not m:
            rows.append((n, 0, 0, 0.0)); continue
        qi = np.array([a for a, _ in m]); ti = np.array([b for _, b in m])
        proj = warp(q_xy[qi], H)
        dist = np.hypot(proj[:, 0] - t_xy[ti, 0], proj[:, 1] - t_xy[ti, 1])
        correct = int((dist <= EPS).sum())
        rows.append((n, correct, len(m), 100.0 * correct / len(m)))
    return rows


def main():
    seq = sys.argv[1] if len(sys.argv) > 1 else "boat"
    print(f"[{seq}] matching precision  (correct/matches, eps={EPS}px, ratio={RATIO})")
    print(f"{'pair':<12}{'plain%':>8}{'oriented%':>11}{'ORB-pyr%':>10}")
    plain = score(seq, 0)
    orient = score(seq, 1)
    orb = score(seq, 2)
    for (n, cp, mp, pp), (_, _, _, po), (_, cb, mb, pb) in zip(plain, orient, orb):
        print(f"img1-img{n:<6}{pp:>7.1f}{po:>11.1f}{pb:>10.1f}"
              f"    (orb {cb}/{mb})")


if __name__ == "__main__":
    sys.exit(main())
