# Install Scripts `scripts/install/install.sh:1` + `install.ps1:1`

**Unix `install.sh` 76 lines:**

```bash
curl -fsSL .../install.sh | bash
curl -fsSL .../install.sh | bash -s -- v0.22.0
CRUX_INSTALL_DIR=~/.local/bin CRUX_STDLIB_DIR=~/.local/share/crux/stdlib bash install.sh
CRUX_BASE_URL=http://127.0.0.1:8766 bash install.sh # local test
```

Env: `REPO` `GITHUB_REPOSITORY:-TheophilusNenhanga/crux-lang`, `VERSION` `latest|v0.22.0`, `INSTALL_DIR` `CRUX_INSTALL_DIR:-$HOME/.local/bin`, `STDLIB_DIR` `CRUX_STDLIB_DIR:-$HOME/.local/share/crux/stdlib`, `CRUX_BASE_URL` override `https://github.com/.../releases/...`.

Detect `uname -s/m` → `linux-amd64` / `macos-amd64` `macos-arm64` `install.sh:13` `detect_platform`, `PLATFORM_ARCH` → `PLATFORM`/`ARCH` `PLATFORM_ARCH%%-*`, `BINARY=crux-${PLATFORM_ARCH}`, `STDLIB_ARCHIVE=crux-stdlib.tar.gz` (`tar -czf -C dist stdlib`).

Steps: `mkdir -p INSTALL/STDLIB`, `mktemp -d`, `curl -fsSL ${URL_BASE}/${BINARY} -o $TMP/crux` `chmod +x` `mv` → `INSTALLED`, `curl ${STDLIB} -o $TMP/crux-stdlib.tar.gz` `tar -xzf -C $TMP` `cp -r $TMP/stdlib/* $STDLIB_DIR`.

**Windows `install.ps1` 45 lines:** `param(Version,InstallDir,StdlibDir)`, `$Repo`, `windows-amd64.exe` + `crux-stdlib.zip`, `Invoke-WebRequest` `Expand-Archive`.

Tested: `bash -n`, `detect_platform`, mocked `curl` + `CRUX_BASE_URL=http://127.0.0.1 python -m http.server` → `crux --version`.

See [CI](ci.md) `release.yml:101` `Create Stdlib Archives`.

## Notes

- **Tests:** see `tests/features/*.crux` + `tests/modules/*.crux` for runnable examples.
- **Related:** [Language Overview](../language/overview.md), [Stdlib](../stdlib/README.md), [Embedding](../embedding/README.md) — all linked from [docs/README.md](../README.md).

## See Also

- [`src/README.md`](../../src/README.md) contributor entry
