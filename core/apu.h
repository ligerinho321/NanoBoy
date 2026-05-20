#pragma once

#include "./utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_apu_t {
    gb_t* gb;

    gb_memory_handler_t pcm12_register_handler;
    gb_memory_handler_t pcm34_register_handler;
} gb_apu_t;

void gb_apu_init(gb_apu_t* apu,gb_t* gb);

void gb_apu_frame_sequency_clock(gb_apu_t* apu);

uint8_t gb_apu_read_pcm12_register(void* data,uint16_t address);

uint8_t gb_apu_read_pcm34_register(void* data,uint16_t address);

void gb_apu_map_registers(gb_apu_t* apu);

void gb_apu_map_pcm_registers(gb_apu_t* apu);

void gb_apu_reset(gb_apu_t* apu);

#ifdef __cplusplus
}
#endif