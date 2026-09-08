import os
import subprocess
import pathlib

# Resolve crux binary: env var takes precedence, else try common relative locations
EXE_CANDIDATES = [
    os.environ.get("CRUX_EXE"),
    os.environ.get("CRUX_BINARY"),
    "../../build/crux",
    "../../build/crux.exe",
    "../build/crux",
    "../build/crux.exe",
    "./build/crux",
    "./build/crux.exe",
]
EXE_PATH = next((p for p in EXE_CANDIDATES if p and os.path.isfile(p)), "../../build/crux")

# Ensure stdlib resolution: default to repo/std if CRUX_STDLIB not set
if "CRUX_STDLIB" not in os.environ:
    # std/_std_test -> std
    std_path = pathlib.Path(__file__).resolve().parent.parent
    os.environ["CRUX_STDLIB"] = str(std_path)

files = sorted([f for f in os.listdir(".") if f.endswith(".crux")])


def run_scripts():
    codes = []
    env = os.environ.copy()
    for file in files:
        print(f"=== RUNNING {file} ===", flush=True)
        result = subprocess.run([EXE_PATH, file], env=env)
        print(f"=== END OF {file} --- CODE {result.returncode} ===\n", flush=True)
        codes.append(result.returncode)
    return codes


if __name__ == "__main__":
    if not files:
        print("No .crux test files found")
        exit(0)
    if not os.path.isfile(EXE_PATH):
        print(f"Crux binary not found at {EXE_PATH}, tried {EXE_CANDIDATES}")
        exit(1)
    codes = run_scripts()
    for c in codes:
        if c != 0:
            exit(-1)
    print("All Stdlib Tests Ran Successfully")
