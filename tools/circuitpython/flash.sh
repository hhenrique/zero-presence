#!/usr/bin/env bash
# Flash CircuitPython onto a Waveshare RP2040-Zero board mounted in bootloader mode (RPI-RP2 drive).
# Put the board in bootloader mode first: hold BOOT, tap/replug RESET, then release.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

UF2_URL="https://downloads.circuitpython.org/bin/waveshare_rp2040_zero/en_US/adafruit-circuitpython-waveshare_rp2040_zero-en_US-10.3.1.uf2"
UF2_PATH="$SCRIPT_DIR/cpy.uf2"

RPI_RP2="${1:-}"
if [[ -z "$RPI_RP2" ]]; then
    RPI_RP2=$(find /run/media/*/RPI-RP2 /media/*/RPI-RP2 -maxdepth 0 2>/dev/null | head -n1 || true)
fi

if [[ -z "$RPI_RP2" || ! -d "$RPI_RP2" ]]; then
    echo "Could not find a mounted RPI-RP2 drive. Put the board in bootloader mode (hold BOOT, tap RESET) and retry, or pass its mount path as an argument." >&2
    exit 1
fi

if [[ ! -f "$UF2_PATH" ]]; then
    echo "Downloading CircuitPython UF2..."
    curl -fL -o "$UF2_PATH" "$UF2_URL"
fi

cp "$UF2_PATH" "$RPI_RP2/"
sync

echo "Flashed. Board will reboot into CircuitPython as CIRCUITPY."
