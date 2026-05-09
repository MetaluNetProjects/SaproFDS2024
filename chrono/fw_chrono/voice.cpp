#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
#include <list>
#include "fraise.h"
#include "voice.h"

int16_t Osc::sine_wave_table[SINE_WAVE_TABLE_LEN];
std::list<Osc> oscList;

int16_t Osc::getSample() {
	pos += step + lfoval;
	if (pos >= pos_max) pos -= pos_max;
	else if(pos < 0) pos += pos_max;
	return ((vol * sine_wave_table[pos >> 16u]) >> 16u);
}

bool Osc::update() {
	if(release && vol > 64) {
		vol = (vol * release) >> 16;
	}
	lfopos += lfofreq << 12;
	if (lfopos >= pos_max) lfopos -= pos_max;
	lfoval = ((int32_t)lfoamp * sine_wave_table[lfopos >> 16u]) >> 8u;
	return (vol > 64);
}

void voice_setup(){
	Osc::setup();
}

int32_t voice_getSample(){
	int32_t s = 0;
	for(Osc &o : oscList) s += o.getSample();
	return s;
}

void voice_update() {
	for (auto itr = oscList.begin(); itr != oscList.end(); itr++) {
		if(!itr->update()) 
			oscList.erase(itr--);
	}
}

void voice_addNote(uint32_t step, uint8_t vol, uint16_t release, uint16_t lfoamp, uint16_t lfofreq) {
	oscList.emplace_back(step, vol, release, lfoamp, lfofreq);
}

void voice_clearNotes() {
	oscList.clear();
}

uint16_t voice_getNbNotes() {
	return oscList.size();
}

int16_t voice_getCos(uint16_t phase) {
	return Osc::sine_wave_table[phase % SINE_WAVE_TABLE_LEN];
}

void voice_debug() {
	printf("pos %ld step %ld\n", oscList.back().pos, oscList.back().step);
}
