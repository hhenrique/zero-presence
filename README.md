# Waveshare RP2040-Zero Mouse Jiggler

Turns the board into a USB HID mouse jiggler, with the onboard NeoPixel
showing status. Two implementations live side by side, each with matching
tooling under `tools/`:

- **`firmware-circuitpython/`** (tooling: `tools/circuitpython/`) —
  CircuitPython. No on/off button: this board's CircuitPython port does not
  expose the BOOTSEL pin in software (confirmed via `microcontroller.pin`,
  see `TROUBLESHOOTING.md`), so it runs continuously from power-on. Quick to
  iterate on.
- **`firmware-c/`** (tooling: `tools/c/`) — C/C++ with the Pico SDK +
  TinyUSB. Reads BOOTSEL directly via register-level access (bypasses the
  limitation above), used as an on/off toggle. More setup, but gives full
  hardware control.

## Behavior

Both variants enumerate as a standard USB HID mouse — no special drivers
needed on the host — and use the same movement cycle: 5s moving / 5s idle /
10s moving / 10s idle, repeating. USB manufacturer/product strings are set
to `"Generic"` / `"USB Mouse"` rather than the board name, so it isn't
singled out by name (the real Raspberry Pi VID/PID is left as-is).

The two variants differ in on/off control and LED behavior, because the
CircuitPython port on this board can't read the BOOTSEL button (see
`TROUBLESHOOTING.md`):

- **`firmware-circuitpython/`**: always on from power-on, no button. LED
  cycles **red** while moving, **green** while idle, for as long as it's
  plugged in.
- **`firmware-c/`**: off by default. BOOTSEL toggles on/off. On power-on
  (off → on transition), the LED blinks **green twice** at 0.2 brightness;
  otherwise the LED stays off the whole time, including during the
  move/idle cycle.

## `firmware-circuitpython/`

- `code.py` — the firmware source, deployed as `code.py` on the board.
- `boot.py` — sets the generic USB manufacturer/product strings via
  `supervisor.set_usb_identification()`. Only takes effect on a hard
  reset/power-cycle, not a `code.py` save.

### Setup

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r tools/circuitpython/requirements.txt
```

### Flashing CircuitPython (first time only)

1. Put the board in bootloader mode: hold **BOOT**, tap/replug **RESET**,
   then release. It should mount as a drive named `RPI-RP2`.
2. Run:

   ```bash
   ./tools/circuitpython/flash.sh
   ```

   The board will reboot and re-mount as `CIRCUITPY`.

### Deploying

With the board mounted as `CIRCUITPY`:

```bash
./tools/circuitpython/deploy.sh
```

This installs the required libraries (`adafruit_hid`, `neopixel`) via
`circup` and copies `firmware-circuitpython/code.py` and `boot.py` onto the
board, which CircuitPython runs automatically.

Both `tools/circuitpython/flash.sh` and `tools/circuitpython/deploy.sh`
auto-detect the mounted drive under `/media/*` or `/run/media/*`; pass the
mount path explicitly as an argument if autodetection fails.

## `firmware-c/`

Has its own on/off toggle on BOOTSEL (no RESET toggle — RESET is wired to
the chip's RUN/reset pin, so pressing it just restarts the program).

### Setup

Requires `cmake`, an `arm-none-eabi-gcc` toolchain, and a `pico-sdk`
checkout (with its submodules initialized). Point `PICO_SDK_PATH` at it.

### Building

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
./tools/c/build.sh
```

Produces `firmware-c/build/mouse_jiggler.uf2`.

### Flashing

1. Put the board in bootloader mode: hold **BOOT**, tap/replug **RESET**,
   then release. It should mount as a drive named `RPI-RP2`.
2. Run:

   ```bash
   ./tools/c/flash.sh
   ```

Both `tools/c/build.sh` and `tools/c/flash.sh` auto-detect paths relative to
the repo root; `flash.sh` also auto-detects the mounted `RPI-RP2` drive
under `/media/*` or `/run/media/*`, or takes it as an explicit argument.
