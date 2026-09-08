#!/usr/bin/env bash
set -euo pipefail

# Crux installer for Unix (Linux/macOS)
# Usage: curl -fsSL https://raw.githubusercontent.com/TheophilusNenhanga/crux-lang/main/scripts/install/install.sh | bash
#        curl -fsSL https://raw.githubusercontent.com/TheophilusNenhanga/crux-lang/main/scripts/install/install.sh | bash -s -- v0.22.0

REPO="${GITHUB_REPOSITORY:-TheophilusNenhanga/crux-lang}"
VERSION="${1:-latest}"
INSTALL_DIR="${CRUX_INSTALL_DIR:-$HOME/.local/bin}"
STDLIB_DIR="${CRUX_STDLIB_DIR:-$HOME/.local/share/crux/stdlib}"

detect_platform() {
  OS="$(uname -s)"
  ARCH="$(uname -m)"
  case "$OS" in
    Linux*) PLATFORM="linux" ;;
    Darwin*) PLATFORM="macos" ;;
    *) echo "Unsupported OS: $OS" >&2; exit 1 ;;
  esac
  case "$ARCH" in
    x86_64|amd64) ARCH="amd64" ;;
    arm64|aarch64) ARCH="arm64" ;;
    *) echo "Unsupported arch: $ARCH" >&2; exit 1 ;;
  esac
  # macos supports amd64 vs arm64, linux currently only amd64 (release matrix)
  if [ "$PLATFORM" = "linux" ] && [ "$ARCH" != "amd64" ]; then
    echo "Unsupported linux arch: $ARCH (only amd64 supported)" >&2
    exit 1
  fi
  echo "${PLATFORM}-${ARCH}"
}

if ! PLATFORM_ARCH="$(detect_platform)"; then
  exit 1
fi
PLATFORM="${PLATFORM_ARCH%%-*}"
ARCH="${PLATFORM_ARCH##*-}"
BINARY="crux-${PLATFORM_ARCH}"
STDLIB_ARCHIVE="crux-stdlib.tar.gz"

# Allow overriding base URL for local testing (e.g. CRUX_BASE_URL=http://localhost:8000)
if [ -n "${CRUX_BASE_URL:-}" ]; then
  URL_BASE="${CRUX_BASE_URL}"
elif [ "$VERSION" = "latest" ]; then
  URL_BASE="https://github.com/${REPO}/releases/latest/download"
else
  URL_BASE="https://github.com/${REPO}/releases/download/${VERSION}"
  # allow v prefix
  if [[ "$VERSION" != v* ]]; then
    URL_BASE="https://github.com/${REPO}/releases/download/v${VERSION}"
  fi
fi

echo "Installing Crux ${VERSION} for ${PLATFORM_ARCH}..."

mkdir -p "$INSTALL_DIR" "$STDLIB_DIR"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

echo "Downloading ${URL_BASE}/${BINARY}..."
curl -fsSL "${URL_BASE}/${BINARY}" -o "${TMP_DIR}/crux"
chmod +x "${TMP_DIR}/crux"
mv "${TMP_DIR}/crux" "${INSTALL_DIR}/crux"
echo "Installed crux to ${INSTALL_DIR}/crux"

echo "Downloading stdlib ${STDLIB_ARCHIVE}..."
if curl -fsSL "${URL_BASE}/${STDLIB_ARCHIVE}" -o "${TMP_DIR}/${STDLIB_ARCHIVE}"; then
  mkdir -p "${TMP_DIR}/stdlib"
  tar -xzf "${TMP_DIR}/${STDLIB_ARCHIVE}" -C "${TMP_DIR}"
  if [ -d "${TMP_DIR}/stdlib" ]; then
    cp -r "${TMP_DIR}/stdlib"/* "${STDLIB_DIR}/"
  fi
  echo "Installed stdlib to ${STDLIB_DIR}"
else
  echo "Warning: stdlib archive not found, skipping" >&2
fi

echo "Add ${INSTALL_DIR} to your PATH if not already."
echo "Crux ${VERSION} installed successfully."
