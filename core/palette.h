#pragma once

#include "utils.h"

enum{
    gb_dmg_bg_palettes = 1,
    gb_dmg_obj_palettes = 2,

    gb_cgb_palettes = 8,
    
    gb_palette_colors = 4,
    
    gb_dmg_colors = gb_palette_colors,
    gb_cgb_colors = gb_cgb_palettes * gb_palette_colors,

    gb_cgb_bytes_per_color = 2,
    gb_cgb_cram_length = gb_cgb_colors * gb_cgb_bytes_per_color,
};

typedef struct _gb_palette_t {
    gb_t* gb;

    uint8_t bgp;
    uint8_t obp[0x02];

    uint8_t bgpi;
    uint8_t obpi;

    uint8_t bg_cram[gb_cgb_cram_length];
    uint8_t obj_cram[gb_cgb_cram_length];

    gb_rgb_t bg_cram_converted[gb_cgb_colors];
    gb_rgb_t obj_cram_converted[gb_cgb_colors];

    gb_memory_descriptor_t dmg_register_descriptor;
    gb_memory_descriptor_t cgb_register_descriptor;
} gb_palette_t;

extern const gb_rgb_t dmg_colors[gb_dmg_colors];


#define gb_palette_get_dmg_bgp_color(palette,palette_index)\
    dmg_colors[((palette)->bgp >> (((palette_index) & 0x03) << 0x01)) & 0x03]

#define gb_palette_get_dmg_obp_color(palette,palette_index,color_index)\
    dmg_colors[((palette)->obp[(palette_index) & 0x01] >> (((color_index) & 0x03) << 0x01)) & 0x03]


#define gb_palette_get_cgb_bgp_color(palette,palette_index,color_index)\
    (palette)->bg_cram_converted[(((palette_index) & 0x07) << 0x02) | ((color_index) & 0x03)]

#define gb_palette_get_cgb_obp_color(palette,palette_index,color_index)\
    (palette)->obj_cram_converted[(((palette_index) & 0x07) << 0x02) | ((color_index) & 0x03)]


#define gb_palette_get_cgb_dmg_bgp_color(palette,palette_index)\
    (palette)->bg_cram_converted[((palette)->bgp >> (((palette_index) & 0x03) << 0x01)) & 0x03]

#define gb_palette_get_cgb_dmg_obp_color(palette,palette_index,color_index)\
    (palette)->obj_cram_converted[(((palette_index) & 0x01) << 0x02) | (((palette)->obp[(palette_index) & 0x01] >> (((color_index) & 0x03) << 0x01)) & 0x03)]

    
#ifdef __cplusplus
extern "C" {
#endif

void gb_palette_init(gb_palette_t* palette,gb_t* gb);

void gb_palette_cgb_dmg_colorization(gb_palette_t* palette);

gb_rgb_t gb_palette_rgb555_to_rgb888(uint16_t color);

void gb_palette_write_dmg_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_palette_read_dmg_register(void* data,uint16_t address);

void gb_palette_write_cgb_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_palette_read_cgb_register(void* data,uint16_t address);

void gb_palette_map(gb_palette_t* palette);

void gb_palette_reset(gb_palette_t* palette);
void gb_palette_skip_boot(gb_palette_t* palette);

void gb_palette_save_state(gb_palette_t* palette,gb_state_t* state);
void gb_palette_load_state(gb_palette_t* palette,gb_state_t* state);

#ifdef __cplusplus
}
#endif