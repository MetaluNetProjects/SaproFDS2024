#include <math.h>

class Osc {
	#define SINE_WAVE_TABLE_LEN 2048
public:
	Osc(uint32_t _step = 0, int8_t _vol = 0, int16_t _release = 0, int16_t _lfoamp = 0, int16_t _lfofreq = 0):
		vol(_vol << 8), release(_release), lfoamp(_lfoamp), lfofreq(_lfofreq)
	{
		setStep(_step);
	}
	int16_t getSample();
	bool update();
	void setStep(uint32_t _step) {
		if(_step < 0x10000) step = 0x10000;
		else if(_step > (SINE_WAVE_TABLE_LEN / 16) * 0x20000) step = (SINE_WAVE_TABLE_LEN / 16) * 0x20000;
		else step = _step;
	}
	void setVol(uint _vol) { vol = _vol; }

	static void setup() {
		for (int i = 0; i < SINE_WAVE_TABLE_LEN; i++) {
			sine_wave_table[i] = 32767 * cosf((i + SINE_WAVE_TABLE_LEN / 4)* 2 * (float) (M_PI / SINE_WAVE_TABLE_LEN));
		}
	}

//private:
	static int16_t sine_wave_table[SINE_WAVE_TABLE_LEN];
	static const uint32_t pos_max = 0x10000 * SINE_WAVE_TABLE_LEN;

	uint32_t step = 0x200000;
	int32_t pos = 0;
	uint16_t vol = 128;
	uint16_t release;
	uint16_t lfoamp;
	uint16_t lfofreq;
	uint32_t lfopos;
	int32_t lfoval;
};

void voice_setup();
int32_t voice_getSample();
void voice_update(); // every 256 samples
void voice_addNote(uint32_t step, uint8_t vol, uint16_t release, uint16_t lfoamp, uint16_t lfofreq);
void voice_clearNotes();
uint16_t voice_getNbNotes();
int16_t voice_getCos(uint16_t phase);
void voice_debug();
