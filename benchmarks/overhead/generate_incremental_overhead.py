#!/usr/bin/env python3
"""Generate one compact incremental-timing table from protocol components."""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from typing import Iterable


OUTPUT_FIELDNAMES = [
    "backend",
    "scheme",
    "parent",
    "degree",
    "setup_incremental_ms",
    "gen_incremental_ms",
    "input_incremental_ms",
    "eval_incremental_ms",
    "verify_incremental_ms",
    "decode_incremental_ms",
]

SCHEME_ORDER = {"ot": 0, "dc": 1}


def read_protocol_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as input_file:
        return list(csv.DictReader(input_file))


def index_protocol_rows(rows: Iterable[dict[str, str]]):
    index: dict[tuple[str, str, int, str], list[float]] = defaultdict(list)
    for row in rows:
        try:
            backend = row["backend"].strip().lower()
            scheme = row["scheme"].strip().lower()
            degree = int(row["degree"])
            phase = row["phase"].strip()
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


def direct_increment(index, backend: str, scheme: str, degree: int, stage: str) -> float | None:
    if stage == "setup":
        return mean_phase(index, backend, scheme, degree, "SetupIncremental")
    if stage == "gen":
        return mean_phase(index, backend, scheme, degree, "GenIncremental")
    if stage == "eval":
        return max_phase(
            index,
            backend,
            scheme,
            degree,
            ["EvalIncremental", "EvalIncremental0", "EvalIncremental1"],
        )
    return None


def stage_total(index, backend: str, scheme: str, degree: int, stage: str) -> float | None:
    if stage == "setup":
        return setup_total(index, backend, scheme, degree)
    if stage == "gen":
        return gen_total(index, backend, scheme, degree)
    if stage == "input":
        return mean_phase(index, backend, scheme, degree, "Input")
    if stage == "eval":
        return eval_total(index, backend, scheme, degree)
    if stage == "verify":
        return mean_phase(index, backend, scheme, degree, "Verify")
    if stage == "decode":
        return mean_phase(index, backend, scheme, degree, "Decode")
    raise ValueError(f"Unknown stage: {stage}")


def incremental_value(
    index, backend: str, scheme: str, parent: str, degree: int, stage: str
) -> float | None:
    direct = direct_increment(index, backend, scheme, degree, stage)
    if direct is not None:
        return direct

    scheme_value = stage_total(index, backend, scheme, degree, stage)
    if scheme_value is None:
        return None

    parent_value = stage_total(index, backend, parent, degree, stage)
    if stage == "input" and parent_value is not None:
        return 0.0
    if parent_value is None:
        return scheme_value
    return scheme_value - parent_value


def format_ms(value: float | None) -> str:
    if value is None:
        return ""
    return f"{value:.6f}"


def protocol_keys(index) -> list[tuple[str, str, int]]:
    keys = {
        (backend, scheme, degree)
        for backend, scheme, degree, _phase in index
        if scheme in {"ot", "dc"}
    }
    return sorted(
        keys,
        key=lambda item: (
            item[0],
            item[2],
            SCHEME_ORDER.get(item[1], 99),
            item[1],
        ),
    )


def incremental_rows(index) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    parent = "vhss"
    for backend, scheme, degree in protocol_keys(index):
        rows.append(
            {
                "backend": backend,
                "scheme": scheme,
                "parent": parent,
                "degree": degree,
                "setup_incremental_ms": format_ms(
                    incremental_value(index, backend, scheme, parent, degree, "setup")
                ),
                "gen_incremental_ms": format_ms(
                    incremental_value(index, backend, scheme, parent, degree, "gen")
                ),
                "input_incremental_ms": format_ms(
                    incremental_value(index, backend, scheme, parent, degree, "input")
                ),
                "eval_incremental_ms": format_ms(
                    incremental_value(index, backend, scheme, parent, degree, "eval")
                ),
                "verify_incremental_ms": format_ms(
                    incremental_value(index, backend, scheme, parent, degree, "verify")
                ),
                "decode_incremental_ms": format_ms(
                    incremental_value(index, backend, scheme, parent, degree, "decode")
                ),
            }
        )
    return rows


def write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=OUTPUT_FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--protocol-components",
        default="benchmarks/results/protocols/logs/protocol_components.csv",
    )
    parser.add_argument("--out-dir", default="benchmarks/results/overhead")
    args = parser.parse_args()

    protocol_components = Path(args.protocol_components)
    if not protocol_components.exists():
        raise FileNotFoundError(
            f"Missing protocol component CSV: {protocol_components}. "
            "Run benchmarks/protocols/run_protocol_benchmarks.py first."
        )

    index = index_protocol_rows(read_protocol_rows(protocol_components))
    rows = incremental_rows(index)
    output_path = Path(args.out_dir) / "incremental_timing.csv"
    write_csv(output_path, rows)
    print(f"Wrote {len(rows)} incremental timing rows to {output_path}")


if __name__ == "__main__":
    main()
