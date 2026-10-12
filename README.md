# Waveshare RP2040-Zero Mouse Jiggler

Turns the board into a USB HID mouse jiggler, with the onboard NeoPixel
showing status. Two implementations live side by side:

- **`firmware-circuitpython/`** — CircuitPython. No on/off button: this
  board's CircuitPython port does not expose the BOOTSEL pin in software
  (confirmed via `microcontroller.pin`, see `TROUBLESHOOTING.md`), so it
  runs continuously from power-on. Quick to iterate on.
- **`firmware-c/`** — C/C++ with the Pico SDK + TinyUSB. Reads BOOTSEL
  directly via register-level access (bypasses the limitation above), used
  as an on/off toggle. More setup, but gives full hardware control.

## Behavior (both variants)

Cycles forever through:
1. 5s moving (LED **red**)
2. 5s idle (LED **green**)
3. 10s moving (LED **red**)
4. 10s idle (LED **green**)

Enumerates as a standard USB HID mouse — no special drivers needed on the
host. USB manufacturer/product strings are set to `"Generic"` / `"USB
Mouse"` rather than the board name, so it isn't singled out by name (the
real Raspberry Pi VID/PID is left as-is).

## `firmware-circuitpython/`

- `code.py` — the firmware source, deployed as `code.py` on the board.
- `boot.py` — sets the generic USB manufacturer/product strings via
  `supervisor.set_usb_identification()`. Only takes effect on a hard
  reset/power-cycle, not a `code.py` save.

### Setup

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r tools/requirements.txt
```

### Flashing CircuitPython (first time only)

1. Put the board in bootloader mode: hold **BOOT**, tap/replug **RESET**,
   then release. It should mount as a drive named `RPI-RP2`.
2. Run:

   ```bash
   ./tools/flash_circuitpython.sh
   ```

   The board will reboot and re-mount as `CIRCUITPY`.

### Deploying

With the board mounted as `CIRCUITPY`:

```bash
./tools/deploy_circuitpython.sh
```

This installs the required libraries (`adafruit_hid`, `neopixel`) via
`circup` and copies `firmware-circuitpython/code.py` and `boot.py` onto the
board, which CircuitPython runs automatically.

Both `tools/flash_circuitpython.sh` and `tools/deploy_circuitpython.sh`
auto-detect the mounted drive under `/media/*` or `/run/media/*`; pass the
mount path explicitly as an argument if autodetection fails.

## `firmware-c/`

See `firmware-c/README.md` for build/flash instructions.
