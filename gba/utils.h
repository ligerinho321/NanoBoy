#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define gba_printf_errno(f) fprintf(stderr,"function: %s line: %d %s: %s\n",__func__,__LINE__,#f,strerror(errno))
#define gba_printf_error(e) fprintf(stderr,"function: %s line: %d error: %s\n",__func__,__LINE__,e)

#define gba_unused(x) ((void)x)

typedef struct _gba_t gba_t;

enum {
    gba_clock_rate = 0x1000000,

    gba_bus_length = 0x100000000,
    
    gba_boot_length = 0x4000,
    gba_boot_address_mask = 0x3FFF,
    
    gba_iwram_length = 0x8000,
    gba_iwram_address_mask = 0x7FFF,

    gba_ewram_length = 0x40000,
    gba_ewram_address_mask = 0x3FFFF,

    gba_cram_length = 0x400,
    gba_cram_address_mask = 0x3FF,

    gba_vram_length = 0x18000,
    gba_vram_address_mask = 0x1FFFF,

    gba_oam_length = 0x400,
    gba_oam_address_mask = 0x3FF,

    gba_screen_width = 240,
    gba_screen_height = 160,
    gba_screen_bytes_per_pixel = 3,
    gba_screen_pitch = gba_screen_width * gba_screen_bytes_per_pixel,
    gba_screen_length = gba_screen_pitch * gba_screen_height,

    gba_scanline_hblank_start_cycle = 1006,

    gba_scanline_cycles = 1232,

    gba_vblank_start_scanline = 160,
    
    gba_scanlines = 228,
    gba_frame_cycles = gba_scanlines * gba_scanline_cycles
};

typedef union _gba_halfword_t {
    uint8_t b[2];
    uint16_t v;
} gba_halfword_t;

typedef union _gba_register32_t {
    uint8_t b[4];
    uint32_t v;
} gba_word_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gba_save_file(const char* filename,void* data,size_t len);
bool gba_load_file(const char* filename,void** data,size_t* len);

uint32_t gba_crc32(const uint8_t* buffer,size_t len);

#ifdef __cplusplus
}
#endif