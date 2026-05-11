#pragma once

#include "./utils.h"

typedef struct _gb_t gb_t;

typedef struct _gb_apu_t {
    gb_t* gb;
} gb_apu_t;

void gb_apu_init(gb_apu_t* apu,gb_t* gb);

void gb_apu_frame_sequency_clock(gb_apu_t* apu);

void gb_apu_reset(gb_apu_t* apu);