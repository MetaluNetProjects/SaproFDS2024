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

#include "pico/audio_pwm.h"
#include "voice.h"
#include "echo.h"
#include "audiobuffer.h"

#define SAMPLES_PER_BUFFER 256
struct audio_buffer_pool *ap;
uint vol = 200;
#define AUDIO_PIN 16

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

// PINS
#define WS2812_PIN 0

int buttons_pin[2] = {4, 5};
int pwm_pin = 16;

uint8_t fade_speed = 8;
#define TICK_MS 5
#define BUTTONS_DEBOUNCE_MS 50
#define BUTTONS_DEBOUNCE_COUNT (BUTTONS_DEBOUNCE_MS / TICK_MS)
#define FLASH_MS 150
#define FLASH_COUNT 6

uint32_t color_slow;
uint32_t color_fast;
uint32_t color_flash;

enum class Mode {STOP = 0, SLOW, FAST} mode;
int mode_time;

Osc osc1, osc2;
float pot_freq;
uint16_t freq = 6000;
Echo<29997> echo1;
int freq_speed = 70;


const uint LED_PIN = PICO_DEFAULT_LED_PIN;

void flash(bool);

absolute_time_t reset_timeout;
bool two_buttons_pushed;
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
			if(buttons[i] && (mode == Mode::STOP)) {
				mode_time = 0;
				mode = Mode(i + 1);
			}
		}
	}
	bool two_b = buttons[0] && buttons[1];
	if(two_b != two_buttons_pushed) {
	    two_buttons_pushed = two_b;
	    if(two_b) reset_timeout =  make_timeout_time_ms(3000);
	}
	if(two_b && time_reached(reset_timeout) && mode != Mode::STOP) {
	    mode = Mode::STOP;
	    flash(true);
	    flash(false);
    }
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

void light_greater(int which, int max, uint32_t color) {
	for(int i = 0; i < NUM_PIXELS; i++) {
		framebuffer[i] = color * (i >= which && i < max);
	}
}

bool flashon;
void flash(bool on) {
    if(on == flashon) return;
    flashon = on;
    light_greater(0, 60, flashon * color_flash);
    if(flashon) {
	    osc1.vol = osc2.vol = 65000;
	    pot_freq = freq + (random() % 1000);
    }
}

void mode_update() {
	switch(mode) {
		case Mode::STOP: return; break;
		case Mode::SLOW:
			if((mode_time * TICK_MS) < 60000)
				light_greater((mode_time * TICK_MS) / 1000, 60, color_slow);
			else 
				//light_greater(0, 60, (((mode_time * TICK_MS) / FLASH_MS) % 2) * color_flash);
				flash(((mode_time * TICK_MS) / FLASH_MS) % 2);
			if((mode_time * TICK_MS) > (60000 + FLASH_MS * FLASH_COUNT)) {
				mode = Mode::STOP;
				light_greater(0, 60, 0);
			}
			break;
		case Mode::FAST:
			if((mode_time * TICK_MS) < 15000)
				light_greater((mode_time * TICK_MS) / 1000, 15, color_fast);
			else 
				//light_greater(0, 60, (((mode_time * TICK_MS) / FLASH_MS) % 2) * color_flash);
				flash(((mode_time * TICK_MS) / FLASH_MS) % 2);
			if((mode_time * TICK_MS) > (15000 + FLASH_MS * FLASH_COUNT)) {
				mode = Mode::STOP;
				light_greater(0, 60, 0);
			}
			break;
	}
	mode_time++;
}

int led_count;
bool tick_callback(repeating_timer_t *rt)
{
	buttons_update();
	mode_update();
	fade_buffer();
	ws2812_dma_transfer(framebuffer2, NUM_PIXELS);
	gpio_put(LED_PIN, ((led_count++ * TICK_MS) / 350) % 2);
	return true;
}

#define clip(s) ((s) > 32767 ? 32767 : (s) < -32767 ? -32767 : (s))
void loop() {
	struct audio_buffer *buffer = take_audio_buffer(ap, true);
	int16_t *samples = (int16_t *) buffer->buffer->bytes;
	int32_t s;
	for (uint i = 0; i < buffer->max_sample_count; i++) {
		s = ((int32_t)vol * ((int)osc1.getSample() + osc2.getSample())) >> 8u; s = clip(s);
		s = s + echo1.getSample(s); s = clip(s);
		//s = s + echo2.getSample(s); s = clip(s);
		samples[i] = s;
	}
	buffer->sample_count = buffer->max_sample_count;
	give_audio_buffer(ap, buffer);
	//pot_freq = pot_freq * 0.9 + freq * 0.1;
	pot_freq += freq_speed;
	//pot_freq = 2000;
	//float step = 100000.0 * pow(2.0, (float)pot_freq * (8.0 / 4096.0));
	float step = 100000.0 * pow(2.0, (float)pot_freq * (8.0 / 16384.0));
	osc1.setStep(step);
	osc2.setStep(step * 1.5);
	osc1.update();
	osc2.update();
}

void setup() {
	//pwm_setup();
	ws2812_setup(WS2812_PIN, IS_RGBW);

	add_repeating_timer_ms(TICK_MS, tick_callback, NULL, &tick_timer);
	for(int i = 0; i < 2; i++) {
		gpio_init(buttons_pin[i]);
		gpio_set_dir(buttons_pin[i], GPIO_IN);
		gpio_pull_up(buttons_pin[i]);
	}
	color_slow = color_u32(64, 64, 64);
	color_fast = color_u32(0, 0, 128);
	color_flash = color_u32(48, 0, 0);
	eeprom_load();
	mode = Mode::FAST;
	mode_time = 15000 / TICK_MS;
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	
	ap = init_audio(44000, SAMPLES_PER_BUFFER, 3, AUDIO_PIN);
	Osc::setup();
	echo1.config(8000, 20000, 8000);
	osc1.vol = osc2.vol = 30000;
	osc1.release = osc2.release = 62000;
}

void fraise_receivebytes(const char* data, uint8_t len) {
	char command = fraise_get_int8(); //data[0];
	switch(command) {
		case 20: set_pixel((int)data[1], data[2], data[3], data[4]); break;
		case 22: light_one(data[1], color_u32(data[2] , data[3] , data[4])); break;
		case 23: light_greater(data[1], 60, color_u32(data[2] , data[3] , data[4])); break;
		case 24: fade_speed = data[1]; break;
		case 25: color_slow = color_u32(data[1] , data[2] , data[3]); light_greater(0, 60, color_slow); break;
		case 26: color_fast = color_u32(data[1] , data[2] , data[3]); light_greater(0, 60, color_fast); break;
		case 27: color_flash = color_u32(data[1] , data[2] , data[3]); light_greater(0, 60, color_flash); break;
		case 30: mode = (Mode)data[1]; light_greater(0, 60, 0); break;
        case 40: osc1.vol = osc2.vol = fraise_get_uint16(); break;
        case 41: pot_freq = freq = fraise_get_uint16(); break;
        case 42:
            osc1.lfoamp = osc2.lfoamp = fraise_get_uint16();
            osc1.lfofreq = osc2.lfofreq = fraise_get_uint16();
            break;
        case 43: osc1.release = osc2.release = fraise_get_uint16(); break;
        case 44: freq_speed = fraise_get_uint16(); break;
        case 45: flash(true); flash(false); break;
        case 46: vol = fraise_get_uint8(); break;
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
	/*eeprom_declare_data(&color_slow, 4);
	eeprom_declare_data(&color_fast, 4);
	eeprom_declare_data(&color_flash, 4);*/
	eeprom_declare_uint32(&color_slow);
	eeprom_declare_uint32(&color_fast);
	eeprom_declare_uint32(&color_flash);
}
