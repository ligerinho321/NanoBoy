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

typedef struct _gb_ppu_lcdc_t {
    bool tile_enabled;
    bool sprite_enabled;
    bool sprite_size;
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
    uint8_t attributes;
    uint8_t lo;
    uint8_t hi;
} gb_bg_fetcher_t;

typedef struct _gb_sprite_fetcher_t {
    uint8_t step;
    uint16_t tile_address;
    uint8_t lo;
    uint8_t hi;
} gb_sprite_fetcher_t;

typedef struct _gb_sprite_t {
    uint8_t y;
    uint8_t x;
    uint8_t tile_index;
    uint8_t attributes;
} gb_sprite_t;

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
    gb_sprite_fetcher_t sprite_fetcher;
    
    uint8_t sprite_found_index;
    bool fetch_window;
    uint8_t fetch_column;
    int drawn_pixels;
    bool fictitious_fetch;

    gb_sprite_t sprite_buffer[0x0A];
    uint8_t sprite_buffer_length;

    gb_pixel_fifo_t tile_fifo;
    gb_pixel_fifo_t sprite_fifo;

    uint8_t vram[0x4000];
    uint8_t* vram_bank_ptr;
    uint8_t vram_bank;
    bool vram_blocked;
    gb_memory_handler_t vram_handler;
    gb_memory_handler_t vbk_register_handler;

    uint8_t oam[0xA0];
    uint8_t oam_address;
    bool oam_blocked;
    gb_memory_handler_t oam_handler;

    bool status_irq_line;

    uint32_t off_cycle;

    uint64_t frame_count;
    bool first_frame;

    uint8_t screen[gb_screen_length];

    void (*clock)(struct _gb_ppu_t* ppu);

    gb_memory_handler_t register_handler;
} gb_ppu_t;

void gb_ppu_init(gb_ppu_t* ppu,gb_t* gb);

void gb_ppu_off_clock(gb_ppu_t* ppu);
void gb_ppu_on_clock(gb_ppu_t* ppu);

void gb_ppu_tile_fetcher_step(gb_ppu_t* ppu);
void gb_ppu_sprite_fetcher_step(gb_ppu_t* ppu);

void gb_ppu_render_pixel(gb_ppu_t* ppu);

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