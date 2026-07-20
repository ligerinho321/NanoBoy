#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _gb_ppu_mode_t {
    gb_ppu_hblank_mode = 0x00,
    gb_ppu_vblank_mode = 0x01,
    gb_ppu_oam_mode = 0x02,
    gb_ppu_drawing_mode = 0x03,
} gb_ppu_mode_t;

typedef enum _gb_tilemap_attribute_mask_t {
    gb_tilemap_palette_mask = 0x07,
    gb_tilemap_tile_bank_mask = 0x08,
    gb_tilemap_horizontal_flip_mask = 0x20,
    gb_tilemap_vertical_flip_mask = 0x40,
    gb_tilemap_priority_mask = 0x80
} gb_tilemap_attribute_mask_t;

typedef enum _gb_object_attribute_mask_t {
    gb_object_cgb_palette_mask = 0x07,
    gb_object_tile_bank_mask = 0x08,
    gb_object_dmg_palette_mask = 0x10,
    gb_object_horizontal_flip_mask = 0x20,
    gb_object_vertical_flip_mask = 0x40,
    gb_object_priority_mask = 0x80
} gb_object_attribute_mask_t;

typedef struct _gb_ppu_lcdc_t {
    bool tile_enabled;
    bool object_enabled;
    bool object_size;
    bool bg_tilemap_area;
    bool tiledata_area;
    bool window_enabled;
    bool window_tilemap_area;
    bool lcd_enabled;
} gb_ppu_lcdc_t;

typedef struct _gb_ppu_status_t {
    uint8_t mode;
    bool lcy_equals_ly;
    bool hblank_enabled;
    bool vblank_enabled;
    bool oam_enabled;
    bool lyc_enabled;
} gb_ppu_status_t;

typedef struct _gb_bg_fetcher_t {
    uint8_t step;
    uint16_t tile_address;
    uint8_t attribute;
    uint8_t lo;
    uint8_t hi;
} gb_bg_fetcher_t;

typedef struct _gb_object_fetcher_t {
    uint8_t step;
    uint16_t tile_address;
    uint8_t lo;
    uint8_t hi;
} gb_object_fetcher_t;

typedef struct _gb_object_t {
    uint8_t y;
    uint8_t x;
    uint8_t tile_index;
    uint8_t attribute;
} gb_object_t;

typedef struct _gb_ppu_t {
    gb_t* gb;

    gb_ppu_lcdc_t lcdc;
    gb_ppu_status_t status;
    
    uint8_t scy;
    uint8_t scx;
    
    uint8_t ly;
    uint8_t _ly;
    uint16_t cycle;

    uint8_t lyc;
    uint16_t _lyc;

    uint8_t wy;
    uint8_t wx;
    bool wy_enabled;
    bool wx_enabled;
    uint8_t window_ly;

    gb_bg_fetcher_t tile_fetcher;
    gb_object_fetcher_t object_fetcher;
    
    uint8_t object_found_index;
    bool fetch_window;
    uint8_t fetch_column;
    int drawn_pixels;
    bool fictitious_fetch;

    gb_object_t object_buffer[0x0A];
    uint8_t object_buffer_length;

    gb_pixel_fifo_t tile_fifo;
    gb_pixel_fifo_t object_fifo;

    uint8_t vram[gb_vram_length];
    uint8_t* vram_bank_ptr;
    uint8_t vram_bank;
    bool vram_blocked;
    gb_memory_handler_t vram_handler;
    gb_memory_handler_t vbk_register_handler;

    uint8_t oam[gb_oam_length];
    uint8_t oam_address;
    bool oam_blocked;
    gb_memory_handler_t oam_handler;

    bool status_irq_line;

    uint32_t off_cycle;

    uint64_t frame_count;
    bool first_frame;

    gb_atomic_bool_t screen_index;
    uint8_t screen[2][gb_screen_length];
    uint8_t* pixel_ptr;

    gb_memory_handler_t register_handler;

    gb_ppu_handler_t* handlers;
} gb_ppu_t;

void gb_ppu_init(gb_ppu_t* ppu,gb_t* gb);

void gb_ppu_clock(gb_ppu_t* ppu,int cycles);

const uint8_t* gb_ppu_get_render_buffer(gb_ppu_t* ppu);

void gb_ppu_add_handler(gb_ppu_t* ppu,gb_ppu_handler_t* handler);
void gb_ppu_remove_handler(gb_ppu_t* ppu,gb_ppu_handler_t* handler);

void gb_ppu_write_vram(void* data,uint8_t value,uint16_t address);
uint8_t gb_ppu_read_vram(void* data,uint16_t address);

void gb_ppu_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_ppu_read_register(void* data,uint16_t address);

void gb_ppu_write_vbk_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_ppu_read_vbk_register(void* data,uint16_t address);

void gb_ppu_write_oam(void* data,uint8_t value,uint16_t address);
uint8_t gb_ppu_read_oam(void* data,uint16_t address);

void gb_ppu_map_vram(gb_ppu_t* ppu);
void gb_ppu_map_registers(gb_ppu_t* ppu);
void gb_ppu_map_oam(gb_ppu_t* ppu);

void gb_ppu_reset(gb_ppu_t* ppu);

#ifdef __cplusplus
}
#endif