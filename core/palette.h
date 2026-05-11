#pragma once

#include "./utils.h"

typedef struct _gb_t gb_t;

typedef struct gb_rgb_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} gb_rgb_t;

typedef struct _gb_palette_t {
    gb_t* gb;

    uint8_t bgp;
    uint8_t obp[2];

    gb_memory_handler_t dmg_register_handler;
    gb_memory_handler_t cgb_register_handler;
} gb_palette_t;

void gb_palette_init(gb_palette_t* palette,gb_t* gb);

void gb_palette_write_dmg_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_palette_read_dmg_register(void* data,uint16_t address);

void gb_palette_write_cgb_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_palette_read_cgb_register(void* data,uint16_t address);

void gb_palette_dmg_map(gb_palette_t* palette);

void gb_palette_cgb_map(gb_palette_t* palette);

void gb_palette_reset(gb_palette_t* palette);