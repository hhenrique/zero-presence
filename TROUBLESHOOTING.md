# Troubleshooting Log

## 2026-10-11 — C firmware: board doesn't enumerate over USB at all

### Observed behavior

After building and flashing `firmware-c/` for the first time, the board
disappeared from USB entirely — not even a generic/unnamed device showed up:

```bash
lsusb -d 2e8a: -v
lsusb
```

Neither command showed the board (no `2e8a:` entry anywhere in `lsusb`).

### Root cause

`bootsel_button_pressed()` (in `firmware-c/src/bootsel_button.h` at the
time) was declared `static inline` while also being pinned to a RAM
section via `__attribute__((section(".time_critical...")))`. The `inline`
keyword lets the compiler duplicate the function body into a
flash-resident caller instead of only keeping the RAM copy. Since the
function's whole purpose is to run from RAM while flash/XIP is briefly
disabled (BOOTSEL shares a pin with QSPI_SS, the flash chip-select line),
an inlined copy executing from flash during that window hangs/crashes the
MCU before USB ever gets a chance to enumerate.

### Fix applied

Moved the function to its own `bootsel_button.c`/`.h` pair and used the
Pico SDK's own `__no_inline_not_in_flash_func` macro, which both places the
function in RAM and forbids inlining — matching the SDK's own convention
for this exact pattern. Rebuilt and reflashed; `lsusb` then showed the
device enumerating correctly.

## 2026-10-11 — C firmware: LED stuck on one color (always red)

### Observed behavior

After the fix above, USB enumeration, the BOOTSEL on/off toggle, and the
mouse-jiggle timing all worked — but the status LED never changed color,
staying red through both the "moving" and "idle" phases.

### Root cause

In `firmware-c/src/main.c`, the PIO program load was written as:

```c
uint offset = pio_claim_free_sm_and_add_program(&ws2812_program, &pio, &pio_sm, NULL);
```

`pio_claim_free_sm_and_add_program()` returns a `bool` (success/failure)
and writes the *actual* instruction-memory offset through its 4th
parameter — which was passed as `NULL` here, discarding it. The boolean
return value (`1` on success) was then used as if it were the offset,
so the state machine was told to start executing from the wrong address in
PIO instruction memory. This produced a malformed WS2812 bit waveform,
which apparently only ever latched the first color successfully sent.

### Fix applied

```c
uint offset;
pio_claim_free_sm_and_add_program(&ws2812_program, &pio, &pio_sm, &offset);
```

Rebuilt, reflashed; the LED then changed color correctly between phases.

## 2026-10-11 — C firmware: "green" power-on blink displays as red

### Observed behavior

After redesigning the LED behavior (off by default, blink green x2 on
power-on, stay off otherwise — see README), the power-on blink visibly
showed **red**, not green.

### Root cause

The RGB-to-wire-byte-order helper (then named `grb()`) assumed the more
common WS2812 wire order (green byte first, then red, then blue), copied
from the canonical `pico-examples` WS2812 driver. This board's onboard
WS2812 apparently expects **RGB** order instead (red byte first) — sending
"green" first-byte data to an RGB-expecting LED displays as red.

### Fix applied

Swapped the byte packing in `firmware-c/src/main.c` (renamed to
`rgb_word()` for clarity) from `g<<24 | r<<16 | b<<8` to
`r<<24 | g<<16 | b<<8`. Rebuilt, reflashed; the power-on blink then showed
green correctly.

## 2026-10-11 — LED cycles correctly, but pointer doesn't visibly move

### Observed behavior

After fixing the crash above, the LED cycled red/green on the correct 5/10s
schedule, but the on-screen pointer never appeared to move.

### Actions taken and commands run

1. Confirmed the board enumerates with a HID interface and the kernel binds
   it as an input device:

   ```bash
   lsusb -v -d 2e8a:
   ls /dev/input/by-id/
   ```

   Result: interface 3 is `bInterfaceClass 3 Human Interface Device`, and
   `/dev/input/by-id/usb-Waveshare_Electronics_RP2040-Zero_..._-if03-event-mouse`
   exists — Linux does see it as a mouse.

2. Captured raw input events directly off that device node during a "moving"
   (red) phase:

   ```bash
   pip install evdev
   python3 - <<'EOF'
   import evdev, time, select
   dev = evdev.InputDevice('/dev/input/by-id/usb-Waveshare_Electronics_RP2040-Zero_E6635C469F0F3F30-if03-event-mouse')
   end = time.time() + 15
   while time.time() < end:
       r, _, _ = select.select([dev.fd], [], [], 0.5)
       if r:
           for event in dev.read():
               print(evdev.categorize(event), event)
   EOF
   ```

   Result: real `REL_X` events arrived every ~0.5s, alternating `val 01` then
   `val -1` about 8ms apart — exactly matching the firmware's
   `mouse.move(x=1); mouse.move(x=-1)` jiggle. So the HID reports were
   genuinely being generated and received by the OS; this ruled out a
   firmware/USB-level bug.

### Root cause

Net-zero, near-instant movement: `+1px` immediately followed by `-1px` only
~8ms later. Desktop compositors commonly coalesce or don't visually render a
pointer round-trip that small and that fast — the data was correct, but
nothing was perceptible on screen.

### Fix applied

In `firmware/code.py`, increased the jiggle distance from ±1px to ±10px and
added a 100ms pause between the two legs (`mouse.move(x=10); time.sleep(0.1);
mouse.move(x=-10)`), so the pointer visibly shifts during the moving phase.
Redeployed with `./tools/deploy.sh`; confirmed working by the user.

## 2026-10-11 — Mouse jiggler doesn't move the mouse; LED blinks green x2 then idles

### Observed behavior

After flashing CircuitPython and deploying `firmware/code.py`, the board was
unplugged and replugged. Resulting behavior:

- LED blinks **green twice**, then goes **off for ~5 seconds**, repeating.
- Expected on-state LED colors (red/green cycle) or off-state **yellow** per
  `firmware/code.py` are never shown.
- The mouse does not move at all, even after pressing BOOTSEL.

This pattern (two green flashes, then a pause, repeating) matches
CircuitPython's own status-LED heartbeat for "`code.py` ran to completion / is
not looping", which means `code.py` is exiting (likely via an unhandled
exception) instead of staying in its intended `while True:` loop.

### Actions taken and commands run

1. Checked block devices and mount state to confirm the board enumerates and
   `CIRCUITPY` is mounted:

   ```bash
   lsblk -o NAME,LABEL,MOUNTPOINT,SIZE
   udisksctl mount -b /dev/sda1
   ```

   Result: `CIRCUITPY` mounted successfully at
   `/run/media/henrique/CIRCUITPY`.

2. Verified `boot_out.txt`, `code.py`, and `lib/` contents on the board:

   ```bash
   cat /run/media/henrique/CIRCUITPY/boot_out.txt
   ls -la /run/media/henrique/CIRCUITPY/
   ls /run/media/henrique/CIRCUITPY/lib
   ```

   Result:
   ```
   Adafruit CircuitPython 10.3.1 on 2026-09-14; Waveshare RP2040-Zero with rp2040
   Board ID:waveshare_rp2040_zero
   UID:E6635C469F0F3F30
   ```
   `code.py` present and matches repo source; `lib/` contains `adafruit_hid`,
   `adafruit_pixelbuf.mpy`, `neopixel.mpy` — all expected files in place, so
   the deploy itself looks correct.

3. Tried to attach to the board's serial console (`/dev/ttyACM0`, exposed as
   `/dev/serial/by-id/usb-Waveshare_Electronics_RP2040-Zero_E6635C469F0F3F30-if00`)
   to read the Python traceback CircuitPython prints on crash:

   ```bash
   pip install pyserial
   python3 - <<'EOF'
   import serial, time
   s = serial.Serial('/dev/serial/by-id/usb-Waveshare_Electronics_RP2040-Zero_E6635C469F0F3F30-if00', 115200, timeout=1)
   s.write(b'\x03')  # ctrl-c to interrupt
   time.sleep(0.5)
   print(s.read(4000).decode(errors='replace'))
   EOF
   ```

   Result: `PermissionError: [Errno 13] Permission denied` — the current user
   is not in the group that owns the device node.

4. Checked device ownership/groups:

   ```bash
   groups
   ls -la /dev/ttyACM0
   getent group dialout uucp lock
   ```

   Result: `/dev/ttyACM0` is `root:uucp`, mode `660`. User `henrique` is in
   `henrique docker input wheel` — **not** `uucp` — so serial access is
   blocked.

5. Attempted `sudo` to read the port directly; blocked because sudo requires
   an interactive password/terminal that isn't available in this session:

   ```bash
   sudo stty -F /dev/ttyACM0 115200 raw -echo
   sudo cat /dev/ttyACM0
   ```

   Result: `sudo: a password is required` / `a terminal is required to read
   the password`.

### Expected result (not yet obtained)

Reading the serial console after a reset should show either:
- A normal REPL banner with no traceback (would mean `code.py` finished
  cleanly — unexpected given the `while True:` loop, would point to a logic
  bug), or
- A Python traceback from CircuitPython (most likely culprit: `board.GPIO33`
  not being a valid attribute on the `waveshare_rp2040_zero` board module —
  this alias for reading the BOOTSEL pin is defined per-board and isn't
  confirmed to exist on this third-party board's CircuitPython port).

### Next step / blocker

Need either:
- The user to run `sudo usermod -aG uucp henrique` (then re-login / `newgrp
  uucp`), or temporarily `sudo chmod 666 /dev/ttyACM0`, so serial console
  access works without a password prompt inside the agent's shell; or
- The user to manually open a serial terminal (e.g. `screen
  /dev/ttyACM0 115200` or the Mu/Thonny serial console) and paste back the
  traceback shown after pressing Ctrl-C / resetting the board.

Leading hypothesis to verify once the traceback is available: `board.GPIO33`
(the BOOTSEL-pin alias used in `firmware/code.py` for the on/off button) may
not be defined for this board's CircuitPython port, causing an
`AttributeError` on the line:

```python
button = digitalio.DigitalInOut(board.GPIO33)
```

which would exit `code.py` immediately — consistent with the "ran to
completion, never reached the loop" LED pattern observed.

### Resolution

Once `/dev/ttyACM0` was made readable (`sudo chmod 666 /dev/ttyACM0`), the
actual traceback was captured over serial:

```bash
python3 - <<'EOF'
import serial, time
s = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
s.write(b'\x03')  # ctrl-c, interrupt any running code
time.sleep(0.3)
s.write(b'\x04')  # ctrl-d, soft reboot to re-run code.py and capture its output
time.sleep(2)
print(s.read(8000).decode(errors='replace'))
EOF
```

Output confirmed the hypothesis exactly:

```
code.py output:
Traceback (most recent call last):
  File "code.py", line 22, in <module>
AttributeError: 'module' object has no attribute 'GPIO33'
```

Further check, to see whether the BOOTSEL pin was reachable by any name at
the port level (not just missing a `board` alias):

```python
import microcontroller
dir(microcontroller.pin)
```

Result: only `GPIO0`–`GPIO29` are listed, no `GPIO33` or any BOOTSEL-named
pin. This means the feature isn't just unaliased on this board — the
underlying pseudo-pin support for reading BOOTSEL in software isn't compiled
into this board's CircuitPython port at all. The low-level readout route is a
dead end on this hardware/firmware combination.

**Fix applied:** dropped the BOOTSEL on/off toggle entirely.
`firmware/code.py` now runs the 5s-move/5s-wait/10s-move/10s-wait cycle
continuously from power-on, with no button logic. Verified by redeploying
(`./tools/deploy.sh`) and reading the serial console again — `code.py`
now stays in `code.py output:` indefinitely (no "Code done running"
message), confirming it's looping as expected instead of crashing.
