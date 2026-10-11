# Waveshare RP2040-Zero Mouse Jiggler

CircuitPython firmware that turns the board into a USB HID mouse jiggler, with
the onboard NeoPixel showing status.

## Behavior

Runs continuously from power-on, cycling forever through:
1. 5s moving (LED **red**)
2. 5s idle (LED **green**)
3. 10s moving (LED **red**)
4. 10s idle (LED **green**)

The board enumerates as a standard USB HID mouse (`adafruit_hid.mouse.Mouse`) —
no special drivers needed on the host.

There is no physical on/off button: this board's CircuitPython port does not
expose the BOOTSEL pin in software (confirmed via `microcontroller.pin`, see
`TROUBLESHOOTING.md`), and the BOOTSEL/GPIO toggle plan was dropped in favor
of always-on. Unplug the board to stop it.

## Layout

- `firmware/code.py` — the firmware source, deployed as `code.py` on the board.
- `tools/flash_circuitpython.sh` — flashes CircuitPython onto a board in
  bootloader mode (mounted as `RPI-RP2`).
- `tools/deploy.sh` — installs required CircuitPython libraries
  (`adafruit_hid`, `neopixel`) via `circup` and copies `firmware/code.py` onto a
  mounted `CIRCUITPY` drive.
- `tools/requirements.txt` — Python deps for the tooling scripts (just `circup`).

## Setup

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r tools/requirements.txt
```

## Flashing CircuitPython (first time only)

1. Put the board in bootloader mode: hold **BOOT**, tap/replug **RESET**, then
   release. It should mount as a drive named `RPI-RP2`.
2. Run:

   ```bash
   ./tools/flash_circuitpython.sh
   ```

   The board will reboot and re-mount as `CIRCUITPY`.

## Deploying the firmware

With the board mounted as `CIRCUITPY`:

```bash
./tools/deploy.sh
```

This installs the required libraries and copies `firmware/code.py` onto the
board as `code.py`, which CircuitPython runs automatically.

Both scripts auto-detect the mounted drive under `/media/*` or
`/run/media/*`; pass the mount path explicitly as an argument if
autodetection fails.
