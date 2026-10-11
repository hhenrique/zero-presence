#!/usr/bin/env bash
# Deploy firmware/code.py, firmware/boot.py, and CircuitPython library deps to a mounted CIRCUITPY drive.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

CIRCUITPY="${1:-}"
if [[ -z "$CIRCUITPY" ]]; then
    CIRCUITPY=$(find /run/media/*/CIRCUITPY /media/*/CIRCUITPY -maxdepth 0 2>/dev/null | head -n1 || true)
fi

if [[ -z "$CIRCUITPY" || ! -d "$CIRCUITPY" ]]; then
    echo "Could not find a mounted CIRCUITPY drive. Pass its path as an argument." >&2
    exit 1
fi

echo "Deploying to: $CIRCUITPY"

source "$REPO_ROOT/.venv/bin/activate"
circup --path "$CIRCUITPY" install adafruit_hid neopixel

cp "$REPO_ROOT/firmware/code.py" "$CIRCUITPY/code.py"
cp "$REPO_ROOT/firmware/boot.py" "$CIRCUITPY/boot.py"
sync

echo "Done."
