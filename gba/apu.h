#pragma once

#include "utils.h"

typedef struct _gba_volume_t {
    uint8_t right;
    uint8_t left;
} gba_volume_t;

typedef struct _gba_panning_t {
    bool right;
    bool left;
} gba_panning_t;


typedef struct _gba_sweep_t {
    uint8_t shift;
    bool negate;
    uint8_t period;
    bool calc_negate;
    uint16_t shadow_frequency;
    uint8_t timer;
} gba_sweep_t;

typedef struct _gba_length_counter_t {
    bool enabled;
    uint16_t counter;
} gba_length_counter_t;

typedef struct _gba_envelope_t {
    uint8_t period;
    bool add_mode;
    uint8_t initial_volume;
    uint8_t timer;
    uint8_t current_volume;
    bool automatic_change;
} gba_envelope_t;


typedef struct _gba_square_t {
    bool enabled;
    bool has_sweep;
    gba_panning_t panning;
    gba_sweep_t sweep;
    gba_length_counter_t length_counter;
    gba_envelope_t envelope;
    uint8_t new_duty;
    uint8_t duty;
    uint8_t duty_pos;
    gba_halfword_t frequency;
    int timer;
} gba_square_t;

typedef struct _gba_wave_t {
    bool enabled;
    gba_panning_t panning;
    bool double_banks;
    bool bank;
    bool dac_enabled;
    gba_length_counter_t length_counter;
    uint8_t volume_code;
    bool force_volume;
    gba_halfword_t frequency;
    int timer;
    uint8_t ram[0x20];
} gba_wave_t;

typedef struct _gba_noise_t {
    bool enabled;
    gba_panning_t panning;
    gba_length_counter_t length_counter;
    gba_envelope_t envelope;
    uint8_t divisor_code;
    bool width_mode;
    uint8_t clock_shift;
    uint16_t lfsr;
    int timer;
} gba_noise_t;

typedef struct _gba_pcm_t {
    gba_panning_t panning;
    bool timer_select;
} gba_pcm_t;

typedef struct _gba_apu_t {
    gba_t* gba;

    bool enabled;
    gba_volume_t master_volume;
    uint8_t psg_volume;
    bool pcm1_volume;
    bool pcm2_volume;
    gba_halfword_t bias;
    uint8_t sample_rate;

    gba_square_t square1;
    gba_square_t square2;
    gba_wave_t wave;
    gba_noise_t noise;
    gba_pcm_t pcm1;
    gba_pcm_t pcm2;
} gba_apu_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_apu_init(gba_apu_t* apu,gba_t* gba);

uint16_t gba_sweep_get_new_frequency(gba_sweep_t* sweep);
void gba_sweep_clock(gba_square_t* square);

void gba_envelope_clock(gba_envelope_t* envelope);
void gba_envelope_write(gba_envelope_t* envelope,uint8_t value,bool *channel_enabled);
uint8_t gba_envelope_read(gba_envelope_t* envelope);
void gba_envelope_trigger(gba_envelope_t* envelope);

void gba_apu_write_register_raw(gba_apu_t* apu,uint8_t value,uint32_t address);
uint8_t gba_apu_read_register_raw(gba_apu_t* apu,uint32_t address);


void gba_square_clock(gba_square_t* square,int cycles);

void gba_square_write_register0(gba_square_t* square,uint8_t value);
uint8_t gba_square_read_register0(gba_square_t* square);

void gba_square_write_register1(gba_square_t* square,uint8_t value);
uint8_t gba_square_read_register1(gba_square_t* square);

void gba_square_write_register2(gba_square_t* square,uint8_t value);
uint8_t gba_square_read_register2(gba_square_t* square);

void gba_square_write_register3(gba_square_t* square,uint8_t value);

void gba_square_write_register4(gba_square_t* square,uint8_t value);
uint8_t gba_square_read_register4(gba_square_t* square);


void gba_wave_clock(gba_wave_t* wave,int cycles);

void gba_wave_write_register0(gba_wave_t* wave,uint8_t value);
uint8_t gba_wave_read_register0(gba_wave_t* wave);

void gba_wave_write_register1(gba_wave_t* wave,uint8_t value);

void gba_wave_write_register2(gba_wave_t* wave,uint8_t value);
uint8_t gba_wave_read_register2(gba_wave_t* wave);

void gba_wave_write_register3(gba_wave_t* wave,uint8_t value);

void gba_wave_write_register4(gba_wave_t* wave,uint8_t value);
uint8_t gba_wave_read_register4(gba_wave_t* wave);


void gba_noise_clock(gba_noise_t* noise,int cycles);

void gba_noise_write_register1(gba_noise_t* noise,uint8_t value);

void gba_noise_write_register2(gba_noise_t* noise,uint8_t value);
uint8_t gba_noise_read_register2(gba_noise_t* noise);

void gba_noise_write_register3(gba_noise_t* noise,uint8_t value);
uint8_t gba_noise_read_register3(gba_noise_t* noise);

void gba_noise_write_register4(gba_noise_t* noise,uint8_t value);
uint8_t gba_noise_read_register4(gba_noise_t* noise);

void gba_apu_reset(gba_apu_t* apu);

#ifdef __cplusplus
}
#endif