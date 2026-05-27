#pragma once

#include "./utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gb_rgb_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} gb_rgb_t;

typedef struct _gb_palette_t {
    gb_t* gb;

    uint8_t bgp;
    uint8_t obp[0x02];

    uint8_t bcps;
    uint8_t ocps;

    uint8_t cgb_bg_cram[0x40];
    uint8_t cgb_obj_cram[0x40];

    gb_rgb_t cgb_bg_cram_converted[0x20];
    gb_rgb_t cgb_obj_cram_converted[0x20];

    gb_memory_handler_t dmg_register_handler;
    gb_memory_handler_t cgb_register_handler;
} gb_palette_t;

extern const gb_rgb_t dmg_palette[4];

void gb_palette_init(gb_palette_t* palette,gb_t* gb);

gb_rgb_t gb_palette_get_dmg_bgp_color(gb_palette_t* palette,uint8_t index);
gb_rgb_t gb_palette_get_dmg_obp_color(gb_palette_t* palette,uint8_t obp_index,uint8_t index);

gb_rgb_t gb_palette_get_cgb_bgp_color(gb_palette_t* palette,uint8_t palette_index,uint8_t color_index);
gb_rgb_t gb_palette_get_cgb_obp_color(gb_palette_t* palette,uint8_t palette_index,uint8_t color_index);

gb_rgb_t gb_palette_get_cgb_dmg_bgp_color(gb_palette_t* palette,uint8_t index);
gb_rgb_t gb_palette_get_cgb_dmg_obp_color(gb_palette_t* palette,uint8_t obp_index,uint8_t index);

gb_rgb_t gb_palette_rgb555_to_rgb888(uint16_t color);

void gb_palette_write_dmg_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_palette_read_dmg_register(void* data,uint16_t address);

void gb_palette_write_cgb_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_palette_read_cgb_register(void* data,uint16_t address);

void gb_palette_map_dmg_registers(gb_palette_t* palette);

void gb_palette_map_cgb_registers(gb_palette_t* palette);
void gb_palette_unmap_cgb_registers(gb_palette_t* palette);

void gb_palette_reset(gb_palette_t* palette);

#ifdef __cplusplus
}
#endif