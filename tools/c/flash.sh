#!/usr/bin/env bash
# Flash the built firmware-c UF2 onto a board mounted in bootloader mode (RPI-RP2 drive).
# Put the board in bootloader mode first: hold BOOT, tap/replug RESET, then release.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
UF2_PATH="$REPO_ROOT/firmware-c/build/mouse_jiggler.uf2"

if [[ ! -f "$UF2_PATH" ]]; then
    echo "No build found at $UF2_PATH. Run tools/c/build.sh first." >&2
    exit 1
fi

RPI_RP2="${1:-}"
if [[ -z "$RPI_RP2" ]]; then
    RPI_RP2=$(find /run/media/*/RPI-RP2 /media/*/RPI-RP2 -maxdepth 0 2>/dev/null | head -n1 || true)
fi

if [[ -z "$RPI_RP2" || ! -d "$RPI_RP2" ]]; then
    echo "Could not find a mounted RPI-RP2 drive. Put the board in bootloader mode (hold BOOT, tap RESET) and retry, or pass its mount path as an argument." >&2
    exit 1
fi

cp "$UF2_PATH" "$RPI_RP2/"
sync

echo "Flashed. Board will reboot into the mouse jiggler firmware."
