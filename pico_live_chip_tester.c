/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "pico_live_chip_tester.pio.h"
#include "hardware/irq.h"  // interrupts

#define PIN_BASE 0
#define AUDIO_PIN 28

// #define LINEAR_PCM_S16_INPUT 1
// #define DEBUG 1
// #define JT51 1

#ifdef JT51
#define DESIRED_SAMPLE_RATE 62000
#else
#define DESIRED_SAMPLE_RATE 57000 // 315/88 / 2 / 32
#endif

uint16_t samples[110000] = { 0 };

uint16_t last_sample;

// #define DEBUG 1

int main() {
#ifdef DEBUG
    stdio_init_all();
    sleep_ms(5000);
    printf("Hello world\n");
#endif

    // Init PWM for audio out
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    // Setup PWM for audio output
    // We run at around 125 MHz. If we set the pwm counter's top value (== wrap value) to 8192 (generally, bigger is better), the pwm counter can reach the top value 15258.7890625 times per second, which would be our effective sample rate. (Calculation: 125000000/8192)
    // However, our target sample rate is larger than that. Let's say if we wanted 44100 Hz: 125000000/44100 = 2834.46712018, so that's the max top value we should set.
    // However, our target sample rate is even larger than that. Let's say we want 60 KHz. Then the max top value is 2083.33333333.
    // In that case, our samples' max loudness should be about half that, 1041.66666667.
    // That's pretty close to 1024. That's good.
    // Let's not hard-code this but calculate based on the desired sample rate.
    // Note that the desired sample rate depends on the VGM tune played.
    uint16_t pwm_wrap = clock_get_hz(clk_sys)/DESIRED_SAMPLE_RATE-24; // TODO: Check if -24 actually improves anything (original intent is to buy microcontroller some time to move to the next sample -- if we don't have enough time, pwm_set_gpio_level might not make it in time and the entire next PWM cycle would be played using the level of the previous sample. I think so anyway.)
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f);
    pwm_config_set_wrap(&config, pwm_wrap);
    pwm_set_gpio_level(AUDIO_PIN, 0);
//     pwm_set_phase_correct(audio_pin_slice, true); // TODO: maybe test if this changes anything?
    pwm_init(audio_pin_slice, &config, true);

    // Init state machine for PIO
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ym3012_dac_program);
    ym3012_dac_init(pio, sm, offset, PIN_BASE);

#ifdef DEBUG
    for (int j = 0; j < 15; j++) {
        for (int i = 0; i < 110000; i++) {
            samples[i] = ym3012_dac_get_sample(pio, sm);
        }
        for (int i = 0; i < 110000; i+=8) {
            printf("%04x %04x %04x %04x %04x %04x %04x %04x\n", samples[i], samples[i+1], samples[i+2], samples[i+3], samples[i+4], samples[i+5], samples[i+6], samples[i+7]);
        }
    }

#else

    while (true) {
#ifdef LINEAR_PCM_S16_INPUT
        last_sample = ym3012_dac_get_sample(pio, sm) >> 5; // rsh 5 <=> div 32. Input is 16-bit, i.e. max value is 65535. rsh 5 max value becomes 2047.
#else
// #ifdef JT51 // jt15 but mod+exp mode
        last_sample = ym3012_dac_get_sample(pio, sm); // same as above
//         printf("%04x\n", last_sample);
        last_sample = last_sample >> 5;
// #else // actual YM2151
//         last_sample = ym3012_dac_get_sample(pio, sm) >> 2; // Input is 13-bit (max value is 8192) // to get to around 2048 we need to divide by 4
// #endif
#endif

//pwm_set_enabled(audio_pin_slice, false); // makes sound worse
        pwm_set_gpio_level(AUDIO_PIN, last_sample);
//pwm_set_enabled(audio_pin_slice, true); // makes sound worse
    }
#endif
}
