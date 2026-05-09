#pragma once

/*#ifdef __cplusplus
extern "C" {
#endif*/

struct audio_buffer_pool *init_audio(uint sample_freq, int buffer_size, int nb_buffers, int pin);

/*#ifdef __cplusplus
}
#endif*/

