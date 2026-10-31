#pragma once

#include "utils.h"

typedef struct _gba_ppu_lcd_control_t {
    uint8_t bg_mode : 3;
    bool frame_select : 1;
    bool hblank_interval_free : 1;
    bool obj_vram_mapping : 1;
    bool forced_blank : 1;
    bool bg0_enabled : 1;
    bool bg1_enabled : 1;
    bool bg2_enabled : 1;
    bool bg3_enabled : 1;
    bool obj_enabled : 1;
    bool window0_enabled : 1;
    bool window1_enabled : 1;
    bool obj_window_enabled : 1;
} gba_ppu_lcd_control_t;

typedef struct _gba_ppu_lcd_status_t {
    bool vblank_flag : 1;
    bool hblank_flag : 1;
    bool lyc_flag : 1;
    bool vblank_irq_enabled : 1;
    bool hblank_irq_enabled : 1;
    bool lyc_irq_enabled : 1;
    uint8_t lyc : 8;
} gba_ppu_lcd_status_t;

typedef struct _gba_ppu_bg_control_t {
    uint8_t priority : 2;
    uint8_t char_base_block : 2;
    bool mosaic_enabled : 1;
    bool color : 1;
    uint8_t screen_base_block : 5;
    bool wraparound : 1;
    uint8_t screen_size : 2;
} gba_ppu_bg_control_t;

typedef struct _gba_ppu_bg_scroll_t {
    gba_halfword_t x;
    gba_halfword_t y;
} gba_ppu_bg_scroll_t;

typedef struct _gba_ppu_bg_affine_t {
    gba_halfword_t dx;
    gba_halfword_t dmx;
    gba_halfword_t dy;
    gba_halfword_t dmy;
    gba_word_t x;
    gba_word_t y;
} gba_ppu_bg_affine_t;

typedef struct _gba_ppu_win_size_t {
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
} gba_ppu_win_size_t;

typedef struct _gba_ppu_win_control_t {
    bool bg0_enabled : 1;
    bool bg1_enabled : 1;
    bool bg2_enabled : 1;
    bool bg3_enabled : 1;
    bool obj_enabled : 1;
    bool special_effect : 1;
} gba_ppu_win_control_t;

typedef struct _gba_ppu_mosaic_size_t {
    uint8_t width : 4;
    uint8_t height : 4;
} gba_ppu_mosaic_size_t;

typedef struct _gba_ppu_special_effect_t {
    bool bg0_first_target : 1;
    bool bg1_first_target : 1;
    bool bg2_first_target : 1;
    bool bg3_first_target : 1;
    bool obj_first_target : 1;
    bool bd_first_target : 1;

    uint8_t effect : 2;

    bool bg0_second_target : 1;
    bool bg1_second_target : 1;
    bool bg2_second_target : 1;
    bool bg3_second_target : 1;
    bool obj_second_target : 1;
    bool bd_second_target : 1;
} gba_ppu_special_effect_t;

typedef struct _gba_ppu_ev_t {
    uint8_t a : 5;
    uint8_t b : 5;
    uint8_t y : 5;
} gba_ppu_ev_t;

typedef struct _gba_ppu_t {
    gba_t* gba;

    gba_ppu_lcd_control_t lcd_control;
    
    bool green_swap;
    
    gba_ppu_lcd_status_t lcd_status;
    
    uint8_t scanline;
    uint16_t cycle;
    
    gba_ppu_bg_control_t bg_control[4];
    
    gba_ppu_bg_scroll_t bg_scroll[4];
    
    gba_ppu_bg_affine_t bg2_affine;
    gba_ppu_bg_affine_t bg3_affine;

    gba_ppu_win_size_t win_size[2];

    gba_ppu_win_control_t win0_control;
    gba_ppu_win_control_t win1_control;
    gba_ppu_win_control_t obj_win_control;
    gba_ppu_win_control_t out_win_control;

    gba_ppu_mosaic_size_t bg_mosaic_size;
    gba_ppu_mosaic_size_t obj_mosaic_size;

    gba_ppu_special_effect_t special_effect;
    gba_ppu_ev_t ev;

    uint8_t cram[gba_cram_length];
    uint8_t vram[gba_vram_length];
    uint8_t oam[gba_oam_length];

    uint64_t frame_count;
    
    uint64_t last_schedule_event;
    uint64_t next_schedule_event;

    gba_ppu_handler_t* handlers;
} gba_ppu_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_ppu_init(gba_ppu_t* ppu,gba_t* gb);

void gba_ppu_add_handler(gba_t* gba,gba_ppu_handler_t* handler);
void gba_ppu_remove_handler(gba_t* gba,gba_ppu_handler_t* handler);

void gba_ppu_schedule_next_event(gba_ppu_t* ppu);

void gba_ppu_update(gba_ppu_t* ppu);


void gba_ppu_write_register_raw(gba_ppu_t* ppu,uint8_t value,uint32_t address);
uint8_t gba_ppu_read_register_raw(gba_ppu_t* ppu,uint32_t address);


void gba_ppu_write_cram_byte(gba_ppu_t* ppu,uint8_t value,uint32_t address);
gba_word_t gba_ppu_read_cram_byte(gba_ppu_t* ppu,uint32_t address);

void gba_ppu_write_cram_halfword(gba_ppu_t* ppu,gba_halfword_t value,uint32_t address);
gba_word_t gba_ppu_read_cram_halfword(gba_ppu_t* ppu,uint32_t address);

void gba_ppu_write_cram_word(gba_ppu_t* ppu,gba_word_t value,uint32_t address);
gba_word_t gba_ppu_read_cram_word(gba_ppu_t* ppu,uint32_t address);


void gba_ppu_write_vram_byte(gba_ppu_t* ppu,uint8_t value,uint32_t address);
gba_word_t gba_ppu_read_vram_byte(gba_ppu_t* ppu,uint32_t address);

void gba_ppu_write_vram_halfword(gba_ppu_t* ppu,gba_halfword_t value,uint32_t address);
gba_word_t gba_ppu_read_vram_halfword(gba_ppu_t* ppu,uint32_t address);

void gba_ppu_write_vram_word(gba_ppu_t* ppu,gba_word_t value,uint32_t address);
gba_word_t gba_ppu_read_vram_word(gba_ppu_t* ppu,uint32_t address);


void gba_ppu_write_oam_byte(gba_ppu_t* ppu,uint8_t value,uint32_t address);
gba_word_t gba_ppu_read_oam_byte(gba_ppu_t* ppu,uint32_t address);

void gba_ppu_write_oam_halfword(gba_ppu_t* ppu,gba_halfword_t value,uint32_t address);
gba_word_t gba_ppu_read_oam_halfword(gba_ppu_t* ppu,uint32_t address);

void gba_ppu_write_oam_word(gba_ppu_t* ppu,gba_word_t value,uint32_t address);
gba_word_t gba_ppu_read_oam_word(gba_ppu_t* ppu,uint32_t address);


void gba_ppu_reset(gba_ppu_t* ppu);

#ifdef __cplusplus
}
#endif