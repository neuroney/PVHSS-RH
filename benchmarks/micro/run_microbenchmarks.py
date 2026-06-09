#!/usr/bin/env python3
"""Run microbenchmark executables and write one compact timing table."""

from __future__ import annotations

import argparse
import csv
import subprocess
from io import StringIO
from pathlib import Path


OUTPUT_FIELDNAMES = ["category", "operation", "mean_ms"]
MICROBENCH_BINARIES = ["microbench", "microbench_vhss_rlwe"]


def add_optional_arg(command: list[str], name: str, value: int | float | None) -> None:
    if value is not None:
        command.extend([name, str(value)])


def format_ms(value: str) -> str:
    return f"{float(value):.6f}"


def compact_rows(stdout: str) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    reader = csv.DictReader(StringIO(stdout))
    for row in reader:
        if not row:
            continue
        rows.append(
            {
                "category": row["category"].strip(),
                "operation": row["primitive"].strip(),
                "mean_ms": format_ms(row["mean_ms"]),
            }
        )
    return rows


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=OUTPUT_FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)


def benchmark_command(binary: Path, args: argparse.Namespace) -> list[str]:
    command = [str(binary)]
    add_optional_arg(command, "--cheap-samples", args.cheap_samples)
    add_optional_arg(command, "--cheap-iters", args.cheap_iters)
    add_optional_arg(command, "--expensive-samples", args.expensive_samples)
    add_optional_arg(command, "--expensive-iters", args.expensive_iters)
    add_optional_arg(command, "--min-sample-ms", args.min_sample_ms)
    add_optional_arg(command, "--max-adaptive-iters", args.max_adaptive_iters)
    return command


def run_binary(binary: Path, args: argparse.Namespace) -> list[dict[str, str]]:
    if not binary.exists():
        raise FileNotFoundError(f"Missing microbenchmark binary: {binary}")

    command = benchmark_command(binary, args)
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        raise subprocess.CalledProcessError(
            completed.returncode, command, completed.stdout, completed.stderr
        )
    return compact_rows(completed.stdout)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--out-dir", default="benchmarks/results/micro")
    parser.add_argument("--cheap-samples", type=int)
    parser.add_argument("--cheap-iters", type=int)
    parser.add_argument("--expensive-samples", type=int)
    parser.add_argument("--expensive-iters", type=int)
    parser.add_argument("--min-sample-ms", type=float)
    parser.add_argument("--max-adaptive-iters", type=int)
    args = parser.parse_args()

    rows: list[dict[str, str]] = []
    for binary_name in MICROBENCH_BINARIES:
        binary = Path(args.build_dir) / "benchmarks/micro" / binary_name
        rows.extend(run_binary(binary, args))

    output_path = Path(args.out_dir) / "micro_timing.csv"
    write_csv(output_path, rows)
    print(f"Wrote {len(rows)} micro timing rows to {output_path}")


if __name__ == "__main__":
    main()
