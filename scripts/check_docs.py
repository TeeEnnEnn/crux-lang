#!/usr/bin/env python3
"""Validate Crux documentation links, API coverage, and runnable examples."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
REGISTRATION = ROOT / "src/native/native_registration.c"
OPCODES = ROOT / "src/_headers/chunk.h"

SECTIONS = {
    "core functions": "core.md",
    "string methods": "string.md",
    "array methods": "array.md",
    "Table methods": "table.md",
    "Error methods": "error.md",
    "Result methods": "result.md",
    "Option methods": "option.md",
    "File methods": "file.md",
    "Random methods  +  module constructor": "random.md",
    "GC module": "gc.md",
    "Vector methods  +  module constructor": "vector.md",
    "Complex methods  +  module constructor": "complex.md",
    "Matrix methods  +  module constructors": "matrix.md",
    "Range methods  +  module constructor": "range.md",
    "Tuple methods  +  module constructor": "tuple.md",
    "Buffer methods  +  module constructor": "buffer.md",
    "Math module": "math.md",
    "IO module": "io.md",
    "Time module": "time.md",
    "System module": "sys.md",
    "Filesystem module": "fs.md",
}

LINK_RE = re.compile(r"!?\[[^\]]*]\(([^)]+)\)")
CALLABLE_RE = re.compile(r'\{"([A-Za-z_][A-Za-z0-9_]*)"\s*,')
SECTION_RE = re.compile(r"^\s*// (.+?)\s*$", re.MULTILINE)
PUBLIC_RE = re.compile(r"^\s*pub\s+(?:fn|struct)\s+([A-Za-z_][A-Za-z0-9_]*)", re.MULTILINE)
OPCODE_RE = re.compile(r"\bOP_[A-Z][A-Z0-9_]*\b")
DOCUMENTED_OPCODE_RE = re.compile(r"`(OP_[A-Z][A-Z0-9_]*)`")
GENERATION_ARTIFACT_RE = re.compile(
    r"user choice|choice 5|relative links `\]\(", re.IGNORECASE
)


def check_links() -> list[str]:
    errors: list[str] = []
    for document in sorted(DOCS.rglob("*.md")):
        text = document.read_text(encoding="utf-8")
        for raw_target in LINK_RE.findall(text):
            target = raw_target.strip().split("#", 1)[0]
            if not target or "://" in target or target.startswith(("mailto:", "data:")):
                continue
            resolved = (document.parent / target).resolve()
            if not resolved.exists():
                errors.append(f"{document.relative_to(ROOT)}: broken link {raw_target!r}")
    return errors


def check_generation_artifacts() -> list[str]:
    errors: list[str] = []
    for document in sorted(DOCS.rglob("*.md")):
        for line_number, line in enumerate(
            document.read_text(encoding="utf-8").splitlines(), 1
        ):
            if GENERATION_ARTIFACT_RE.search(line):
                errors.append(
                    f"{document.relative_to(ROOT)}:{line_number}: generation artifact"
                )
    return errors


def registration_sections() -> dict[str, set[str]]:
    source = REGISTRATION.read_text(encoding="utf-8")
    markers = list(SECTION_RE.finditer(source))
    result: dict[str, set[str]] = {}
    for index, marker in enumerate(markers):
        name = marker.group(1)
        if name not in SECTIONS:
            continue
        end = markers[index + 1].start() if index + 1 < len(markers) else len(source)
        result[name] = set(CALLABLE_RE.findall(source[marker.end() : end]))
    return result


def check_native_coverage() -> list[str]:
    errors: list[str] = []
    for section, symbols in registration_sections().items():
        page = DOCS / "stdlib" / SECTIONS[section]
        text = page.read_text(encoding="utf-8")
        for symbol in sorted(symbols):
            if not re.search(rf"\b{re.escape(symbol)}\b", text):
                errors.append(
                    f"{page.relative_to(ROOT)}: missing registered callable {symbol!r}"
                )
    return errors


def check_package_coverage() -> list[str]:
    errors: list[str] = []
    for package in ("collections", "statistics"):
        page = DOCS / "stdlib" / f"{package}.md"
        documentation = page.read_text(encoding="utf-8")
        source = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted((ROOT / "std" / package).glob("*.crux"))
        )
        for symbol in sorted(set(PUBLIC_RE.findall(source))):
            if not re.search(rf"\b{re.escape(symbol)}\b", documentation):
                errors.append(
                    f"{page.relative_to(ROOT)}: missing public package symbol {symbol!r}"
                )
    return errors


def check_opcode_references() -> list[str]:
    implemented = set(OPCODE_RE.findall(OPCODES.read_text(encoding="utf-8")))
    page = DOCS / "reference/opcodes.md"
    documented = set(DOCUMENTED_OPCODE_RE.findall(page.read_text(encoding="utf-8")))
    return [
        f"{page.relative_to(ROOT)}: unknown opcode {opcode!r}"
        for opcode in sorted(documented - implemented)
    ]


def find_crux_binary(explicit: str | None) -> Path | None:
    candidates = [
        explicit,
        os.environ.get("CRUX_EXE"),
        ROOT / "build/crux",
        ROOT / "build/crux.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    return None


def run_examples(binary: Path) -> list[str]:
    errors: list[str] = []
    environment = os.environ.copy()
    environment.setdefault("CRUX_STDLIB", str(ROOT / "std"))
    for example in sorted((DOCS / "examples").glob("*.crux")):
        result = subprocess.run(
            [str(binary), str(example)],
            cwd=ROOT,
            env=environment,
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode:
            output = (result.stdout + result.stderr).strip()
            errors.append(
                f"{example.relative_to(ROOT)}: exited {result.returncode}\n{output}"
            )
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--crux", help="path to the Crux executable")
    parser.add_argument(
        "--require-runtime",
        action="store_true",
        help="fail instead of skipping examples when no Crux executable is available",
    )
    arguments = parser.parse_args()

    errors = (
        check_links()
        + check_generation_artifacts()
        + check_native_coverage()
        + check_package_coverage()
        + check_opcode_references()
    )
    binary = find_crux_binary(arguments.crux)
    if binary:
        errors.extend(run_examples(binary))
    elif arguments.require_runtime:
        errors.append("Crux executable not found; build it or pass --crux")
    else:
        print("Crux executable not found; runnable documentation examples skipped.")

    if errors:
        print("\n\n".join(errors), file=sys.stderr)
        return 1

    print("Documentation links, API coverage, and examples are valid.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
