#include <stdbool.h>
#include <stdint.h>

#include "bsp/board_api.h"
#include "tusb.h"

#include "hardware/pio.h"
#include "pico/stdlib.h"

#include "bootsel_button.h"
#include "ws2812.pio.h"

#define NEOPIXEL_PIN 16
#define NEOPIXEL_FREQ_HZ 800000.f

#define BUTTON_DEBOUNCE_MS 50
#define JIGGLE_INTERVAL_MS 500
#define JIGGLE_STEP_PX 10
#define JIGGLE_SETTLE_MS 100

typedef struct {
    uint32_t move_ms;
    uint32_t wait_ms;
} phase_t;

static const phase_t PHASES[] = {
    {5000, 5000},
    {10000, 10000},
};
#define NUM_PHASES (sizeof(PHASES) / sizeof(PHASES[0]))

static PIO pio;
static uint pio_sm;

static inline uint32_t grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
}

static void set_led(uint8_t r, uint8_t g, uint8_t b) {
    pio_sm_put_blocking(pio, pio_sm, grb(r, g, b));
}

static bool button_edge_pressed(void) {
    static bool last_state = false;
    static uint32_t last_change_ms = 0;

    bool current = bootsel_button_pressed();
    uint32_t now = board_millis();

    if (current != last_state && (now - last_change_ms) > BUTTON_DEBOUNCE_MS) {
        last_state = current;
        last_change_ms = now;
        return current;
    }
    return false;
}

static void hid_jiggle(void) {
    if (!tud_hid_ready()) {
        return;
    }
    tud_hid_mouse_report(0, 0, JIGGLE_STEP_PX, 0, 0, 0);
    board_delay(JIGGLE_SETTLE_MS);
    tud_hid_mouse_report(0, 0, -JIGGLE_STEP_PX, 0, 0, 0);
}

/* Runs `duration_ms`, polling the button throughout. Returns false if the
 * button was pressed during this hold, meaning the caller should stop. */
static bool hold(uint32_t duration_ms, uint8_t r, uint8_t g, uint8_t b, bool jiggle) {
    set_led(r, g, b);
    uint32_t start = board_millis();
    uint32_t last_jiggle = start;

    while (board_millis() - start < duration_ms) {
        tud_task();

        if (button_edge_pressed()) {
            return false;
        }

        if (jiggle && (board_millis() - last_jiggle) >= JIGGLE_INTERVAL_MS) {
            hid_jiggle();
            last_jiggle = board_millis();
        }

        board_delay(1);
    }
    return true;
}

static void wait_off(void) {
    set_led(255, 150, 0);
    while (!button_edge_pressed()) {
        tud_task();
        board_delay(1);
    }
}

int main(void) {
    board_init();
    tusb_init();

    uint offset = pio_claim_free_sm_and_add_program(&ws2812_program, &pio, &pio_sm, NULL);
    ws2812_program_init(pio, pio_sm, offset, NEOPIXEL_PIN, NEOPIXEL_FREQ_HZ);

    bool enabled = false;
    set_led(0, 0, 0);

    while (true) {
        tud_task();

        if (!enabled) {
            wait_off();
            enabled = true;
            continue;
        }

        bool completed = true;
        for (size_t i = 0; i < NUM_PHASES && completed; ++i) {
            completed = hold(PHASES[i].move_ms, 255, 0, 0, true);
            if (completed) {
                completed = hold(PHASES[i].wait_ms, 0, 255, 0, false);
            }
        }

        if (!completed) {
            enabled = false;
        }
    }
}

/* TinyUSB HID callbacks */

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
