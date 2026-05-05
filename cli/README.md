# Crux CLI

The Crux Command Line Interface (CLI) provides tools for managing projects, installing dependencies, and executing Crux code.

## Usage

### Run a script directly
```bash
crux path/to/script.crux
```

### Start the interactive REPL
```bash
crux
```

### Initialize a new project
Creates a `crux.json` manifest, a `main.crux` entry point, and a `pkg.crux` for re-exports.
```bash
crux init [project_name]
```

### Install dependencies
Clones Git-based dependencies specified in `crux.json` into the `crux_modules/` directory.
```bash
crux install
```

### Run a project
Executes the entry point defined in the `main` field of `crux.json` (defaults to `main.crux`).
```bash
crux run [path]
```

## Manifest (`crux.json`)

Every Crux project contains a manifest file:

```json
{
  "name": "my_project",
  "version": "0.1.0",
  "main": "main.crux",
  "dependencies": {
    "math": "https://github.com/user/crux-math.git"
  }
}
```

## Re-exports (`pkg.crux`)

The `pkg.crux` file is the entry point for your package when others import it via `pkg:`. Use `pub use` to expose internal symbols:

```crux
// pkg.crux
pub use my_func from "./internal.crux";
```

## Project Structure

```text
my_project/
├── crux.json       # Project manifest
├── main.crux       # Application entry point
├── pkg.crux        # Package entry point (re-exports)
└── crux_modules/   # Installed dependencies
```
