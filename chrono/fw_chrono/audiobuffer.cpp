#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/audio_pwm.h"

struct audio_buffer_pool *init_audio(uint sample_freq, int buffer_size, int nb_buffers, int pin) {

    static audio_format_t audio_format = {
            .sample_freq = sample_freq,
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .channel_count = 1,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 2
    };

    static audio_pwm_channel_config_t channel_config = default_mono_channel_config;
    channel_config.core.base_pin = pin;
    //channel_config.core.pio_sm = 1;
    
    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, nb_buffers,
                                                                      buffer_size); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;
    output_format = audio_pwm_setup(&audio_format, -1, &channel_config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }
    ok = audio_pwm_default_connect(producer_pool, false);
    assert(ok);
    audio_pwm_set_enabled(true);
   return producer_pool;
}

