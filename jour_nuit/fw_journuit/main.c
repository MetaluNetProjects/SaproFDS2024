/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define BOARD pico

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.h"
#include "fraise_eeprom.h"
#include "fraise.h"

#define IS_RGBW false
#define NUM_PIXELS 60

uint32_t framebuffer[NUM_PIXELS];
uint32_t framebuffer2[NUM_PIXELS];

static inline uint32_t color_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (g) << 24) |
            ((uint32_t) (r) << 16) |
            ((uint32_t) (b) << 8 );
}

void set_pixel(int n, uint8_t r, uint8_t g, uint8_t b) {
	if(n >= 0 && n < NUM_PIXELS) {
		framebuffer[n] = color_u32(r, g, b);
	}
}

repeating_timer_t tick_timer;

int buttons_debounce[2];
bool buttons[2];
bool last_buttons[2];
int buttons_pin[2] = {4, 5};
uint8_t fade_speed = 8;
uint16_t cycle_ms = 2000;
#define TICK_MS 5
#define BUTTONS_DEBOUNCE_MS 50
#define BUTTONS_DEBOUNCE_COUNT (BUTTONS_DEBOUNCE_MS / TICK_MS)
#define FLASH_MS 150
#define FLASH_COUNT 6
#define SWITCHOFF_TIMEOUT_MS (3600 * 1000)

uint32_t color_day;
uint32_t color_night;
uint32_t color;

//enum {STOP = 0, ON} mode;
bool mode_on = true;
int mode_time;
int switchoff_time; // ms
const uint LED_PIN = PICO_DEFAULT_LED_PIN;

void buttons_update() {
	for(int i = 0; i < 2; i++) {
		if(!gpio_get(buttons_pin[i])) {
			if(buttons_debounce[i] < BUTTONS_DEBOUNCE_COUNT) buttons_debounce[i]++;
			else buttons[i] = true;
		} else {
			if(buttons_debounce[i] > 0) buttons_debounce[i]--;
			else buttons[i] = false;
		}
		if(last_buttons[i] != buttons[i]) {
			last_buttons[i] = buttons[i];
			printf("b %d %d\n", i, buttons[i]?1:0);
			if(switchoff_time <= 0) mode_time = 0;
			switchoff_time = SWITCHOFF_TIMEOUT_MS;
		}
	}
	//if(!button[0] && !button[1]) mode = STOP;
	if((buttons[0] || buttons[1]) && (mode_time * TICK_MS) < (cycle_ms + TICK_MS))
		mode_time++;
	else if(mode_time > 0) mode_time--;
	if(buttons[0]) color = color_day;
	else if(buttons[1]) color = color_night;
}

void fade_buffer() {
	for(int i = 0; i < NUM_PIXELS; i++) {
		uint8_t *b = (uint8_t *)(framebuffer + i);
		uint8_t *b2 = (uint8_t *)(framebuffer2 + i);
		for(int c = 1; c < 4; c++)
			b2[c] += (((int)b[c] - (int)b2[c]) / fade_speed) + (b[c] > b2[c] ? 1 : b[c] < b2[c] ? -1 : 0);
	}
}

void light_one(int which, uint32_t color) {
	for(int i = 0; i < NUM_PIXELS; i++) {
		framebuffer[i] = color * (i == which);
	}
}

void light_greater(int which, uint32_t color) {
	for(int i = 0; i < NUM_PIXELS; i++) {
		framebuffer[i] = color * (i >= which);
	}
}


void mode_update() {
	/*switch(mode) {
		case STOP:
			if(mode_time > 0) {
				light_greater(NUM_PIXELS - (mode_time * NUM_PIXELS * TICK_MS) / cycle_ms, color);
				mode_time--;
			}
			break;
		case ON:
			if((mode_time * TICK_MS) < cycle_ms) {
				light_greater(NUM_PIXELS - (mode_time * NUM_PIXELS * TICK_MS) / cycle_ms, color);
				mode_time++;
			}
			break;
	}*/
	if(switchoff_time > 0) {
		light_greater(NUM_PIXELS - (mode_time * NUM_PIXELS * TICK_MS) / cycle_ms, color);
		switchoff_time -= TICK_MS;
	} else light_greater(0, 0);
}

int led_count;
bool tick_callback(repeating_timer_t *rt)
{
	buttons_update();
	if(mode_on) mode_update();
	fade_buffer();
	ws2812_dma_transfer(framebuffer2, NUM_PIXELS);
	gpio_put(LED_PIN, ((led_count++ * TICK_MS) / 350) % 2);
	return true;
}

void setup() {
	ws2812_setup(0, IS_RGBW);
	add_repeating_timer_ms(TICK_MS, tick_callback, NULL, &tick_timer);
	for(int i = 0; i < 2; i++) {
		gpio_init(buttons_pin[i]);
		gpio_set_dir(buttons_pin[i], GPIO_IN);
		gpio_pull_up(buttons_pin[i]);
	}
	color_day = color_u32(64, 64, 64);
	color_night = color_u32(0, 0, 128);
	eeprom_load();
	//mode = STOP;
	mode_time = 0;
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
}

void fraise_receivebytes(const char* data, uint8_t len) {
	char command = data[0];
	switch(command) {
		case 20: set_pixel((int)data[1], data[2], data[3], data[4]); break;
		case 22: light_one(data[1], color_u32(data[2] , data[3] , data[4])); break;
		case 23: light_greater(data[1], color_u32(data[2] , data[3] , data[4])); break;
		case 24: fade_speed = data[1]; break;
		case 25: color_day = color_u32(data[1] , data[2] , data[3]); light_greater(0, color_day); break;
		case 26: color_night = color_u32(data[1] , data[2] , data[3]); light_greater(0, color_night); break;
		case 28: cycle_ms = (data[1] << 8) | data[2];
		case 30: mode_on = data[1]; light_greater(0, 0); break;
		case 128: if(data[1] == 128) { // save eeprom
			printf("saving eeprom\n");
			eeprom_save();
		}
	}
}

void fraise_receivechars(const char *data, uint8_t len){
	if(data[0] == 'E') { // Echo
		printf("E%s\n", data + 1);
	}
}

void eeprom_declare_main() {
	eeprom_declare_uint32(&color_day);
	eeprom_declare_uint32(&color_night);
	eeprom_declare_uint8(&fade_speed);
	eeprom_declare_uint16(&cycle_ms);
}
