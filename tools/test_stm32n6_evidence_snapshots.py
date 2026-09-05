#!/usr/bin/env python3
"""Verify published STM32N6 evidence snapshots match companion sources."""

import json
import pathlib


ROOT = pathlib.Path(__file__).resolve().parents[1]
COMPANION = ROOT.parent / "examples-stm32n6"
EVIDENCE = ROOT / "docs/benchmarks/evidence"
FOUNDATION = ROOT / "docs/benchmarks/stm32n6-foundation-gate.md"
PAIRS = (
    (
        EVIDENCE / "ch12_local_classifier.json",
        COMPANION / "docs/benchmarks/ch12_local_classifier.json",
    ),
    (
        EVIDENCE / "ch12_local_classifier_manifest.json",
        COMPANION / "models/ch12_local_classifier/manifest.json",
    ),
)


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    for snapshot, source in PAIRS:
        assert snapshot.is_file(), f"missing publication snapshot: {snapshot}"
        assert source.is_file(), f"missing companion source: {source}"
        assert load(snapshot) == load(source), f"snapshot differs from {source}"

    page = FOUNDATION.read_text(encoding="utf-8")
    for snapshot, _source in PAIRS:
        relative = snapshot.relative_to(FOUNDATION.parent).as_posix()
        assert f"]({relative})" in page, f"foundation page does not link {relative}"


if __name__ == "__main__":
    main()
