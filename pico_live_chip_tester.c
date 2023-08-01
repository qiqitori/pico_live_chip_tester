/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "pico_live_chip_tester.pio.h"
#include "hardware/irq.h"  // interrupts

#define ALL_REGULAR_GPIO_PINS 0b00011100011111111111111111111111

#define PIN_BASE 0
#define POISON16 0x5555
#define POISON8 0x55
#define UNKNOWN 2

// #define REPORT_SLOW_CAPTURE
#define DEBUG

// TODO: could easily drive the RX/TX status LEDs from the PIO, would save precious CPU cycles
// could probably even be done using the PIO's side-set feature
#define STATUS_LEDS
#ifdef STATUS_LEDS
#define LED_RX 16
#define LED_TX 17
#define LED_NO_ERRORS 18
#define LED_ERROR 19
#define LED_COMPARISON_SUCCESSFUL 20
#endif

uint8_t g_memory[1] = { UNKNOWN };
uint8_t address_buffer[196608] = { UNKNOWN };
bool g_is_read = false, g_is_write = false;
bool g_din = false, g_dout = false;
// uint16_t g_state_at_ras = POISON16, g_state_at_cas = POISON16;
// uint8_t g_ras_address = POISON8, g_cas_address = POISON8;
uint16_t g_state_at_ras = 0, g_state_at_cas = 0;
uint8_t g_ras_address = 0, g_cas_address = 0;
uint32_t g_waited = 0;

int main() {
    uint32_t n_successful_comps = 0;
    uint32_t n_reads = 0;
    uint32_t n_writes = 0;

    gpio_init_mask(ALL_REGULAR_GPIO_PINS);
    gpio_set_dir_masked(ALL_REGULAR_GPIO_PINS, GPIO_IN);
    gpio_set_function(13, GPIO_FUNC_PIO0); // TODO: use constant or something
    stdio_init_all();
    sleep_ms(5000);
    printf("Hello world\n");

//     memset(g_memory, UNKNOWN, 65536);

#ifdef STATUS_LEDS
    bool led_rx_state = false, led_tx_state = false, led_comp_state = false;

    gpio_init(LED_RX);
    gpio_init(LED_TX);
    gpio_init(LED_NO_ERRORS);
    gpio_init(LED_ERROR);
    gpio_set_dir(LED_RX, GPIO_OUT);
    gpio_set_dir(LED_TX, GPIO_OUT);
    gpio_set_dir(LED_NO_ERRORS, GPIO_OUT);
    gpio_set_dir(LED_ERROR, GPIO_OUT);
    gpio_put(LED_RX, led_rx_state);
    gpio_put(LED_TX, led_tx_state);
    gpio_put(LED_NO_ERRORS, true);
    gpio_put(LED_ERROR, false);
#endif

    // Init state machine for PIO
    PIO pio = pio0;
    uint sm1 = 0;
    uint sm2 = 1;
    uint offset1 = pio_add_program(pio, &ras_trigger_program);
    uint offset2 = pio_add_program(pio, &cas_trigger_program);
    ras_trigger_program_init(pio, sm1, offset1, PIN_BASE);
    cas_trigger_program_init(pio, sm2, offset2, PIN_BASE);

#ifdef DEBUG
//     uint16_t prev_cas = 0;
    for (int i = 0; i < 120000; i += 3) {
//         g_waited = 0;
        get_bus_at_cas(pio, sm1, sm2);
//         if (g_waited == 0) {
//             printf("%d: capture code is too slow (g_waited == %u)\n", i, g_waited);
//         }
        address_buffer[i] = g_ras_address;
        address_buffer[i+1] = g_cas_address;
        address_buffer[i+2] = (int)g_is_write << 2 | g_din << 1 | g_dout;
    }
    for (int i = 0; i < 120000; i += 3) {
        if (address_buffer[i+2] >> 2) {
            printf("%06d: write %u to %u\n", i, (address_buffer[i+2] >> 1) & 1, address_buffer[i]*256 + address_buffer[i+1]);
        } else {
            printf("%06d: read %u from %u\n", i, address_buffer[i+2] & 1, address_buffer[i]*256 + address_buffer[i+1]);
        }
//         printf("%05d: %u*256+%u = %u\n", i, address_buffer[i], address_buffer[i+1], address_buffer[i]*256+address_buffer[i+1]);
    }
    printf("g_waited: %d\n", g_waited);
    return 0;
#endif
#ifdef BORING_DEBUG
    for (;;) {
        get_bus_at_cas(pio, sm1, sm2);
        printf("g_state_at_ras: %04x\n", g_state_at_ras);
        printf("g_state_at_cas: %04x\n", g_state_at_cas);
        printf("ras: %u\n", g_ras_address);
        printf("cas: %u\n", g_cas_address);
        printf("address: %u\n", g_ras_address*256+g_cas_address);
        printf("is_read: %u is_write: %u\n", g_is_read, g_is_write);
        printf("din: %u\n", g_din);
        printf("dout: %u\n", g_dout);
        printf("\n\n");
    }
    return;
#endif
    uint8_t *memory_under_test = NULL;
    while (true) {
        g_waited = 0;
        get_bus_at_cas(pio, sm1, sm2);
#ifdef REPORT_SLOW_CAPTURE
        if (g_waited == 0) {
            printf("Capture code is too slow\n");
        }
#endif
        memory_under_test = &g_memory[g_ras_address*256+g_cas_address];
        if (g_is_write) {
            n_writes++;
#ifdef STATUS_LEDS
            led_rx_state = !led_rx_state;
            gpio_put(LED_RX, led_rx_state);
            if (led_tx_state)
                led_tx_state = false; // makes it easier to see if LED is just never updated
#endif
            *memory_under_test = g_din;
        } else {
            n_reads++;
#ifdef STATUS_LEDS
            led_tx_state = !led_tx_state;
            gpio_put(LED_TX, led_tx_state);
            if (led_rx_state)
                led_rx_state = false; // makes it easier to see if LED is just never updated
#endif
            if (*memory_under_test != UNKNOWN) {
                if (*memory_under_test != g_din) {
#ifdef STATUS_LEDS
                    gpio_put(LED_NO_ERRORS, false);
                    gpio_put(LED_ERROR, true);
#endif
                    printf("Possibly bad memory at %u*256+%u = %u, have %u but read %u\n", g_ras_address, g_cas_address, g_ras_address*256+g_cas_address, *memory_under_test, g_din);
                    printf("Successful comparisons up to now: %u\n", n_successful_comps);
                    printf("Read ops up to now: %u\n", n_reads);
                    printf("Write ops up to now: %u\n", n_writes);
                    break;
                } else {
                    printf("Successful read at %u*256+%u = %u, have %u and read %u\n", g_ras_address, g_cas_address, g_ras_address*256+g_cas_address, *memory_under_test, g_din);
                    n_successful_comps++;
                    led_comp_state = !led_comp_state;
                    gpio_put(LED_COMPARISON_SUCCESSFUL, led_comp_state);
                }
            }
        }
    }
}
