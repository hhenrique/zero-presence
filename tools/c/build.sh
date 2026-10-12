#!/usr/bin/env bash
# Configure and build firmware-c/ with CMake + the Pico SDK.
# Requires PICO_SDK_PATH to point at a pico-sdk checkout (with submodules).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BUILD_DIR="$REPO_ROOT/firmware-c/build"

: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to your pico-sdk checkout first}"

cmake -S "$REPO_ROOT/firmware-c" -B "$BUILD_DIR" -DPICO_BOARD=waveshare_rp2040_zero
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Built: $BUILD_DIR/mouse_jiggler.uf2"
