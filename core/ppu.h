#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_ppu_lcdc_t {
    union{
        bool bg_and_window_enabled;
        bool bg_and_window_priority;
    };
    bool obj_enabled;
    bool obj_size;
    bool bg_tilemap_area;
    bool bg_and_window_tiledata_area;
    bool window_enabled;
    bool window_tilemap_area;
    bool lcd_enabled;
} gb_ppu_lcdc_t;

typedef struct _gb_ppu_status_t {
    uint8_t mode;
    bool lcy_equals_ly;
    bool mode0_select;
    bool mode1_select;
    bool mode2_select;
    bool lyc_select;
} gb_ppu_status_t;

typedef struct _gb_ppu_t {
    gb_t* gb;

    gb_ppu_lcdc_t lcdc;
    gb_ppu_status_t status;
    uint8_t scy;
    uint8_t scx;
    uint8_t ly;
    uint8_t lyc;
    uint8_t wy;
    uint8_t wx;

    gb_memory_handler_t register_handler;
} gb_ppu_t;

void gb_ppu_init(gb_ppu_t* ppu,gb_t* gb);

void gb_ppu_clock(gb_ppu_t* ppu);

void gb_ppu_write_register(void* data,uint8_t value,uint16_t address);

uint8_t gb_ppu_read_register(void* data,uint16_t address);

void gb_ppu_map_registers(gb_ppu_t* ppu);

void gb_ppu_reset(gb_ppu_t* ppu);

#ifdef __cplusplus
}
#endif