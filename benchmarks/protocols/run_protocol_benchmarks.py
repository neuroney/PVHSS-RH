#!/usr/bin/env python3
"""Run protocol benchmarks and write a compact phase-timing table."""

from __future__ import annotations

import argparse
import csv
import re
import subprocess
from collections import defaultdict
from pathlib import Path
from typing import Iterable


TARGETS = {
    "group-hss": {
        "backend": "group",
        "scheme": "hss",
        "binary": "benchmarks/protocols/protocol_group_hss_bench",
    },
    "group-vhss": {
        "backend": "group",
        "scheme": "vhss",
        "binary": "benchmarks/protocols/protocol_group_vhss_bench",
    },
    "group-ot": {
        "backend": "group",
        "scheme": "ot",
        "binary": "benchmarks/protocols/protocol_group_ot_bench",
    },
    "group-dc": {
        "backend": "group",
        "scheme": "dc",
        "binary": "benchmarks/protocols/protocol_group_dc_bench",
    },
    "rlwe-hss": {
        "backend": "rlwe",
        "scheme": "hss",
        "binary": "benchmarks/protocols/protocol_rlwe_hss_bench",
    },
    "rlwe-vhss": {
        "backend": "rlwe",
        "scheme": "vhss",
        "binary": "benchmarks/protocols/protocol_rlwe_vhss_bench",
    },
    "rlwe-ot": {
        "backend": "rlwe",
        "scheme": "ot",
        "binary": "benchmarks/protocols/protocol_rlwe_ot_bench",
    },
    "rlwe-dc": {
        "backend": "rlwe",
        "scheme": "dc",
        "binary": "benchmarks/protocols/protocol_rlwe_dc_bench",
    },
    "rlwe-cz": {
        "backend": "rlwe",
        "scheme": "cz",
        "binary": "benchmarks/protocols/protocol_cz_bench",
    },
}

COMPONENT_FIELDNAMES = [
    "backend",
    "scheme",
    "target",
    "degree",
    "phase",
    "label",
    "samples",
    "mean_ms",
    "rsd_percent",
    "log_path",
]

TIMING_FIELDNAMES = [
    "backend",
    "scheme",
    "degree",
    "setup_ms",
    "gen_ms",
    "input_ms",
    "eval_ms",
    "verify_ms",
    "decode_ms",
]

SCHEME_ORDER = {"hss": 0, "vhss": 1, "ot": 2, "dc": 3, "cz": 4}

DEGREE_RE = re.compile(r"^(?:degree_f|Degree):\s*(\d+)\b")
TIMING_RE = re.compile(
    r"^(.+?):\s*([0-9.+\-eE]+)\s*ms\s+RSD:\s*([0-9.+\-eE]+)%"
)


def normalize_phase(label: str) -> str:
    value = label.strip().lower()
    if "setup base" in value:
        return "SetupBase"
    if "setup incremental" in value:
        return "SetupIncremental"
    if "keygen incremental" in value or "gen incremental" in value:
        return "GenIncremental"
    if "evaluation base 0" in value:
        return "EvalBase0"
    if "evaluation base 1" in value:
        return "EvalBase1"
    if "evaluation incremental 0" in value:
        return "EvalIncremental0"
    if "evaluation incremental 1" in value:
        return "EvalIncremental1"
    if "prove" in value:
        return "EvalIncremental"
    if "evaluation 0" in value or "evaluate0" in value or "server 1" in value:
        return "Eval0"
    if "evaluation 1" in value or "evaluate1" in value or "server 2" in value:
        return "Eval1"
    if "eval" in value:
        return "Eval"
    if "verification" in value or value.startswith("ver "):
        return "Verify"
    if "decryption" in value or "decoding" in value or value.startswith("dec "):
        return "Decode"
    if "keygen" in value or "key gen" in value:
        return "Gen"
    if value.startswith("gen ") or "hss_gen" in value or " gen " in value:
        return "Gen"
    if "enc time setup" in value or "input" in value or "hss_input" in value:
        return "Input"
    if value.startswith("enc "):
        return "Input"
    if "setup" in value:
        return "Setup"
    if "output" in value:
        return "Decode"
    return label.strip()


def parse_log(
    text: str, target: str, meta: dict[str, str], samples: int, log_path: Path
) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    degree: int | None = None

    for line in text.splitlines():
        degree_match = DEGREE_RE.match(line.strip())
        if degree_match:
            degree = int(degree_match.group(1))
            continue

        timing_match = TIMING_RE.match(line.strip())
        if not timing_match or degree is None:
            continue

        label = timing_match.group(1).strip()
        rows.append(
            {
                "backend": meta["backend"],
                "scheme": meta["scheme"],
                "target": target,
                "degree": degree,
                "phase": normalize_phase(label),
                "label": label,
                "samples": samples,
                "mean_ms": float(timing_match.group(2)),
                "rsd_percent": float(timing_match.group(3)),
                "log_path": str(log_path),
            }
        )

    return rows


def write_csv(path: Path, rows: Iterable[dict[str, object]], fieldnames: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def index_rows(rows: Iterable[dict[str, object]]):
    index: dict[tuple[str, str, int, str], list[float]] = defaultdict(list)
    for row in rows:
        try:
            backend = str(row["backend"]).strip().lower()
            scheme = str(row["scheme"]).strip().lower()
            degree = int(row["degree"])
            phase = str(row["phase"]).strip()
            mean_ms = float(row["mean_ms"])
        except (KeyError, TypeError, ValueError):
            continue
        index[(backend, scheme, degree, phase)].append(mean_ms)
    return index


def mean(values: list[float]) -> float | None:
    if not values:
        return None
    return sum(values) / len(values)


def mean_phase(index, backend: str, scheme: str, degree: int, phase: str) -> float | None:
    return mean(index.get((backend, scheme, degree, phase), []))


def max_phase(index, backend: str, scheme: str, degree: int, phases: list[str]) -> float | None:
    values: list[float] = []
    for phase in phases:
        values.extend(index.get((backend, scheme, degree, phase), []))
    if not values:
        return None
    return max(values)


def sum_present(*values: float | None) -> float | None:
    present = [value for value in values if value is not None]
    if not present:
        return None
    return sum(present)


def setup_total(index, backend: str, scheme: str, degree: int) -> float | None:
    direct = mean_phase(index, backend, scheme, degree, "Setup")
    if direct is not None:
        return direct
    return sum_present(
        mean_phase(index, backend, scheme, degree, "SetupBase"),
        mean_phase(index, backend, scheme, degree, "SetupIncremental"),
    )


def gen_total(index, backend: str, scheme: str, degree: int) -> float | None:
    direct = mean_phase(index, backend, scheme, degree, "Gen")
    if direct is not None:
        return direct
    return mean_phase(index, backend, scheme, degree, "GenIncremental")


def eval_total(index, backend: str, scheme: str, degree: int) -> float | None:
    direct = max_phase(index, backend, scheme, degree, ["Eval", "Eval0", "Eval1"])
    if direct is not None:
        return direct

    per_server_totals: list[float] = []
    for server in ("0", "1"):
        base = mean_phase(index, backend, scheme, degree, f"EvalBase{server}")
        increment = mean_phase(index, backend, scheme, degree, f"EvalIncremental{server}")
        total = sum_present(base, increment)
        if total is not None:
            per_server_totals.append(total)
    if per_server_totals:
        return max(per_server_totals)

    return sum_present(
        mean_phase(index, backend, scheme, degree, "EvalBase"),
        mean_phase(index, backend, scheme, degree, "EvalIncremental"),
    )


def stage_row(index, backend: str, scheme: str, degree: int) -> dict[str, object]:
    return {
        "backend": backend,
        "scheme": scheme,
        "degree": degree,
        "setup_ms": format_ms(setup_total(index, backend, scheme, degree)),
        "gen_ms": format_ms(gen_total(index, backend, scheme, degree)),
        "input_ms": format_ms(mean_phase(index, backend, scheme, degree, "Input")),
        "eval_ms": format_ms(eval_total(index, backend, scheme, degree)),
        "verify_ms": format_ms(mean_phase(index, backend, scheme, degree, "Verify")),
        "decode_ms": format_ms(mean_phase(index, backend, scheme, degree, "Decode")),
    }


def format_ms(value: float | None) -> str:
    if value is None:
        return ""
    return f"{value:.6f}"


def timing_rows(rows: Iterable[dict[str, object]]) -> list[dict[str, object]]:
    index = index_rows(rows)
    keys = {
        (backend, scheme, degree)
        for backend, scheme, degree, _phase in index
    }
    return [
        stage_row(index, backend, scheme, degree)
        for backend, scheme, degree in sorted(
            keys,
            key=lambda item: (
                item[0],
                item[2],
                SCHEME_ORDER.get(item[1], 99),
                item[1],
            ),
        )
    ]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--out-dir", default="benchmarks/results/protocols")
    parser.add_argument("--degrees", default="5,10,15")
    parser.add_argument("--samples", type=int, default=1)
    parser.add_argument("--msg-num", type=int, default=5)
    parser.add_argument("--targets", nargs="+", choices=sorted(TARGETS), default=sorted(TARGETS))
    parser.add_argument("--list", action="store_true", help="List available protocol benchmark targets and exit")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without running them")
    args = parser.parse_args()

    if args.list:
        for name in sorted(TARGETS):
            meta = TARGETS[name]
            print(f"{name}: backend={meta['backend']} scheme={meta['scheme']} binary={meta['binary']}")
        return

    build_dir = Path(args.build_dir)
    out_dir = Path(args.out_dir)
    log_dir = out_dir / "logs"
    component_csv_path = log_dir / "protocol_components.csv"
    timing_csv_path = out_dir / "protocol_timing.csv"
    rows: list[dict[str, object]] = []

    for target in args.targets:
        meta = TARGETS[target]
        binary = build_dir / meta["binary"]
        command = [
            str(binary),
            "--msg-num",
            str(args.msg_num),
            "--samples",
            str(max(1, args.samples)),
            "--degrees",
            args.degrees,
        ]

        if args.dry_run:
            print(" ".join(command))
            continue

        if not binary.exists():
            raise FileNotFoundError(f"Missing benchmark binary: {binary}")

        completed = subprocess.run(command, text=True, capture_output=True, check=False)
        log_dir.mkdir(parents=True, exist_ok=True)
        log_path = log_dir / f"{target}.log"
        log_path.write_text(completed.stdout + completed.stderr)

        if completed.returncode != 0:
            raise subprocess.CalledProcessError(
                completed.returncode, command, completed.stdout, completed.stderr
            )

        rows.extend(parse_log(completed.stdout, target, meta, max(1, args.samples), log_path))

    if args.dry_run:
        return

    summary_rows = timing_rows(rows)
    write_csv(timing_csv_path, summary_rows, TIMING_FIELDNAMES)
    write_csv(component_csv_path, rows, COMPONENT_FIELDNAMES)
    print(f"Wrote {len(summary_rows)} protocol timing rows to {timing_csv_path}")
    print(f"Wrote {len(rows)} internal component rows to {component_csv_path}")


if __name__ == "__main__":
    main()
