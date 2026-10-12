#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/sync.h"

/*
 * The BOOTSEL button is wired to the QSPI_SS line, which is normally driven
 * by the QSPI peripheral to talk to flash. Reading it requires pausing XIP
 * (flash execution) briefly: disconnect QSPI_SS from the peripheral, read it
 * as a plain input, then reconnect. This must run from RAM, not flash, since
 * flash is inaccessible for the duration. Interrupts are disabled around it
 * so nothing tries to execute from flash while QSPI is detached.
 */
static inline bool __attribute__((section(".time_critical.bootsel_button")))
bootsel_button_pressed(void) {
    const uint CS_PIN_INDEX = 1;

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
