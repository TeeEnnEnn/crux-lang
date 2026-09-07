import os
import pathlib
import subprocess
from typing import List

# Resolve crux binary: env var takes precedence, else try common relative locations
EXE_CANDIDATES = [
    os.environ.get("CRUX_EXE"),
    os.environ.get("CRUX_BINARY"),
    "../build/crux",
    "../build/crux.exe",
    "../../build/crux",
    "../../build/crux.exe",
    "./build/crux",
    "./build/crux.exe",
]
EXE_PATH = next((p for p in EXE_CANDIDATES if p and os.path.isfile(p)), "../build/crux")
directories: list[str] = ["builtins", "features", "modules", "importing", "compiler"]
files: list[str] = []


def get_files() -> None:
    for directory in directories:
        path = f"./{directory}/"
        if not os.path.isdir(path):
            # Try repo-root relative when invoked from repo root
            alt = os.path.join(os.path.dirname(__file__), directory)
            if os.path.isdir(alt):
                path = alt + "/"
            else:
                continue
        items = os.listdir(path)
        # Only run *.crux files (ignore test.txt etc)
        crux_items = [item for item in items if item.endswith(".crux")]
        files.extend([f"{path}{item}" for item in crux_items])


def run_scripts() -> List[int]:
    return_codes = []
    env = os.environ.copy()
    # Ensure CRUX_STDLIB defaults to repo std if not set (for std: imports in some tests)
    if "CRUX_STDLIB" not in env:
        std_path = pathlib.Path(__file__).resolve().parent.parent / "std"
        if std_path.is_dir():
            env["CRUX_STDLIB"] = str(std_path)
    for file in files:
        print(f"=== RUNNING {file} ===", flush=True)
        result = subprocess.run([EXE_PATH, file], env=env)
        print(f"=== END OF {file} --- CODE {result.returncode} ===\n", flush=True)
        return_codes.append(result.returncode)
    return return_codes


if __name__ == "__main__":
    if not os.path.isfile(EXE_PATH):
        print(f"Crux binary not found at {EXE_PATH}, tried {EXE_CANDIDATES}")
        exit(1)
    get_files()
    if not files:
        print("No .crux test files found")
        exit(0)
    codes = run_scripts()
    for code in codes:
        if code != 0:
            exit(-1)
    else:
        print("All Tests Ran Successfully")
