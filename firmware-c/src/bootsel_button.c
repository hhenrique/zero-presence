#include "bootsel_button.h"

#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/sync.h"
#include "pico/platform.h"

/*
 * The BOOTSEL button is wired to the QSPI_SS line, which is normally driven
 * by the QSPI peripheral to talk to flash. Reading it requires pausing XIP
 * (flash execution) briefly: disconnect QSPI_SS from the peripheral, read it
 * as a plain input, then reconnect. This function must actually execute from
 * RAM, not flash, since flash is inaccessible for the duration -
 * __no_inline_not_in_flash_func both places it in RAM and forbids inlining,
 * since an inlined copy in a flash-resident caller would defeat the point.
 * Interrupts are disabled around it so nothing else tries to execute from
 * flash while QSPI is detached.
 */
bool __no_inline_not_in_flash_func(bootsel_button_pressed)(void) {
    const uint32_t CS_PIN_INDEX = 1;

    uint32_t flags = save_and_disable_interrupts();

    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                     GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                     IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    for (volatile int i = 0; i < 1000; ++i) {
    }

    bool button_state = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                     GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                     IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return button_state;
}
