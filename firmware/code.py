import time

import board
import neopixel
import usb_hid
from adafruit_hid.mouse import Mouse

RED = (255, 0, 0)
GREEN = (0, 255, 0)

MOVE_SECONDS = (5, 10)
WAIT_SECONDS = (5, 10)

JIGGLE_INTERVAL = 0.5

pixel = neopixel.NeoPixel(board.NEOPIXEL, 1, brightness=0.2)
mouse = Mouse(usb_hid.devices)


def hold(duration, led_color, jiggle=False):
    pixel.fill(led_color)
    start = time.monotonic()
    last_jiggle = start
    while time.monotonic() - start < duration:
        if jiggle and (time.monotonic() - last_jiggle) >= JIGGLE_INTERVAL:
            mouse.move(x=10)
            time.sleep(0.1)
            mouse.move(x=-10)
            last_jiggle = time.monotonic()
        time.sleep(0.02)


while True:
    for move_secs, wait_secs in zip(MOVE_SECONDS, WAIT_SECONDS):
        hold(move_secs, RED, jiggle=True)
        hold(wait_secs, GREEN, jiggle=False)
