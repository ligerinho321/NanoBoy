#pragma once

#include "utils.h"
#include "blip_buf/blip_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gb_apu_callback_t)(void* data);

typedef struct _gb_apu_t gb_apu_t;

typedef struct _gb_apu_panning_t {
    bool left;
    bool right;
} gb_apu_panning_t;

typedef struct _gb_apu_volume_t {
    uint8_t left;
    uint8_t right;
} gb_apu_volume_t;

typedef struct _gb_apu_sweep_t {
    bool enabled;
    uint8_t period;
    uint8_t timer;
    bool negate;
    bool calc_negate;
    uint8_t shift;
    uint16_t shadow_frequency;
} gb_apu_sweep_t;

typedef struct _gb_apu_envelope_t {
    uint8_t initial_volume;
    uint8_t current_volume;
    bool add_mode;
    uint8_t period;
    uint8_t timer;
    bool automatic_change;
} gb_apu_envelope_t;

typedef struct _gb_apu_length_counter_t {
    bool enabled;
    uint16_t counter;
} gb_apu_length_counter_t;

typedef struct _gb_apu_square_t {
    gb_apu_t* apu;
    bool has_sweep;
    bool enabled;
    bool external_enabled;
    gb_apu_sweep_t sweep;
    gb_apu_envelope_t envelope;
    gb_apu_length_counter_t length_counter;
    uint8_t duty;
    uint8_t duty_pos;
    uint16_t frequency;
    int timer;
    uint8_t output;
} gb_apu_square_t;

typedef struct _gb_apu_wave_t {
    gb_apu_t* apu;
    bool enabled;
    bool external_enabled;
    bool dac_enabled;
    gb_apu_length_counter_t length_counter;
    uint8_t volume_code;
    uint16_t frequency;
    int timer;
    uint8_t sample_buffer;
    uint8_t ram_pos;
    uint8_t ram[0x10];
    uint8_t output;
} gb_apu_wave_t;

typedef struct _gb_apu_noise_t {
    gb_apu_t* apu;
    bool enabled;
    bool external_enabled;
    gb_apu_envelope_t envelope;
    gb_apu_length_counter_t length_counter;
    uint8_t clock_shift;
    bool width_mode;
    uint8_t divisor_code;
    uint16_t lfsr;
    int timer;
    uint8_t output;
} gb_apu_noise_t;


typedef struct _gb_apu_channel_frame_t {
    blip_t* blip;
    int last_output;
    int16_t samples[gb_audio_frame_samples];
    int samples_count;
    float capacitor;
} gb_apu_channel_frame_t;

void gb_apu_channel_frame_end(gb_apu_channel_frame_t* channel_frame);

void gb_apu_channel_frame_reset(gb_apu_channel_frame_t* channel_frame);


typedef struct _gb_apu_mixer_frame_t {
    blip_t* blip_left;
    blip_t* blip_right;
    int last_left_output;
    int last_right_output;
    int16_t samples[gb_audio_mixer_buffer_samples];
    int samples_count;
    float left_capacitor;
    float right_capacitor;
} gb_apu_mixer_frame_t;

void gb_apu_mixer_frame_end(gb_apu_mixer_frame_t* mixer_frame);

void gb_apu_mixer_frame_reset(gb_apu_mixer_frame_t* mixer_frame);


typedef struct _gb_apu_t {
    gb_t* gb;

    gb_apu_channel_frame_t square1_frame;
    gb_apu_channel_frame_t square2_frame;
    gb_apu_channel_frame_t wave_frame;
    gb_apu_channel_frame_t noise_frame;
    gb_apu_mixer_frame_t mixer_frame;

    int frame_cycle;

    gb_ring_buffer_t ring_buffer;

    gb_apu_square_t square1;
    gb_apu_square_t square2;
    gb_apu_wave_t wave;
    gb_apu_noise_t noise;

    gb_apu_panning_t square1_panning;
    gb_apu_panning_t square2_panning;
    gb_apu_panning_t wave_panning;
    gb_apu_panning_t noise_panning;
    gb_apu_panning_t vin_panning;
    
    gb_apu_volume_t volume;

    bool enabled;

    uint8_t frame_sequencer;
    bool skip_first_frame_sequence_event;

    uint64_t last_clock_cycle;
    uint64_t cycles;

    gb_apu_callback_t callback;
    void* callback_data;

    gb_memory_handler_t square1_register_handler;
    gb_memory_handler_t square2_register_handler;
    gb_memory_handler_t wave_register_handler;
    gb_memory_handler_t noise_register_handler;
    gb_memory_handler_t register_handler;
    gb_memory_handler_t wave_ram_handler;
    gb_memory_handler_t pcm12_register_handler;
    gb_memory_handler_t pcm34_register_handler;
} gb_apu_t;

void gb_apu_init(gb_apu_t* apu,gb_t* gb);

void gb_apu_set_callback(gb_apu_t* apu,gb_apu_callback_t callback,void* data);
void gb_apu_remove_callback(gb_apu_t* apu);

void gb_apu_update_rates(gb_apu_t* apu);

void gb_apu_run(gb_apu_t* apu);

void gb_apu_frame_sequencer_clock(gb_apu_t* apu);

void gb_apu_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_apu_read_register(void* data,uint16_t address);

void gb_apu_square_clock(gb_apu_square_t* square,int timer);
void gb_apu_write_square_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_apu_read_square_register(void* data,uint16_t address);
uint8_t gb_apu_square_raw_output(gb_apu_square_t* square);
int gb_apu_square_output(gb_apu_square_t* square);
void gb_apu_square_update_output(gb_apu_square_t* square);
void gb_apu_square_reset(gb_apu_square_t* square,bool hardware);

void gb_apu_wave_clock(gb_apu_wave_t* wave,int timer);
void gb_apu_write_wave_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_apu_read_wave_register(void* data,uint16_t address);
void gb_apu_write_wave_ram(void* data,uint8_t value,uint16_t address);
uint8_t gb_apu_read_wave_ram(void* data,uint16_t address);
uint8_t gb_apu_wave_raw_output(gb_apu_wave_t* wave);
int gb_apu_wave_output(gb_apu_wave_t* wave);
void gb_apu_wave_update_output(gb_apu_wave_t* wave);
void gb_apu_wave_reset(gb_apu_wave_t* wave,bool hardware);

void gb_apu_noise_clock(gb_apu_noise_t* noise,int timer);
void gb_apu_write_noise_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_apu_read_noise_register(void* data,uint16_t address);
uint8_t gb_apu_noise_raw_output(gb_apu_noise_t* noise);
int gb_apu_noise_output(gb_apu_noise_t* noise);
void gb_apu_noise_update_output(gb_apu_noise_t* noise);
void gb_apu_noise_reset(gb_apu_noise_t* noise,bool hardware);

uint8_t gb_apu_read_pcm12_register(void* data,uint16_t address);
uint8_t gb_apu_read_pcm34_register(void* data,uint16_t address);

void gb_apu_map_registers(gb_apu_t* apu);

void gb_apu_map_pcm_registers(gb_apu_t* apu);

void gb_apu_unmap_pcm_registers(gb_apu_t* apu);

void gb_apu_reset(gb_apu_t* apu,bool hardware);

void gb_apu_free(gb_apu_t* apu);

#ifdef __cplusplus
}
#endif