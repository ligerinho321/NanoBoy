#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <threads.h>

#ifdef _WIN32
#include <windows.h> // Sleep(), QueryPerformanceFrequency(), QueryPerformanceCounter()
#else
#include <stdatomic.h>
#include <time.h> // nanosleep(), clock_gettime()
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define gb_printf_errno(f) fprintf(stderr,"function: %s line: %d %s: %s\n",__func__,__LINE__,#f,strerror(errno))
#define gb_printf_error(e) fprintf(stderr,"function: %s line: %d error: %s\n",__func__,__LINE__,e)

#define gb_min(x,y) ((x < y) ? x : y)

#define gb_speed_step 0.25f
#define gb_speed_min 1.0f
#define gb_speed_max 10.0f

#define gb_high_pass_factor 0.996013f

enum {
    gb_clock_rate = 4194304,

    gb_vram_length = 0x4000,
    gb_wram_length = 0x8000,
    gb_oam_length = 0xA0,
    gb_hram_length = 0x7F,

    gb_oam_objects = gb_oam_length / 4,
    
    gb_object_width = 8,
    gb_object_min_height = 8,
    gb_object_max_height = 16,
    
    gb_tile_size = 8,

    gb_tilemap_columns = 32,
    gb_tilemap_rows = 32,

    gb_screen_columns = 20,
    gb_screen_rows = 18,

    gb_screen_width = gb_screen_columns * gb_tile_size,
    gb_screen_height = gb_screen_rows * gb_tile_size,
    gb_screen_bytes_per_pixel = 3,
    gb_screen_pitch = gb_screen_width * gb_screen_bytes_per_pixel,
    gb_screen_length = gb_screen_pitch * gb_screen_height,

    gb_vblank_scanline = 144,
    gb_scanlines = 154,
    gb_scanline_cycles = 456,
    gb_frame_cycles = 70224,

    gb_audio_sample_rate = 44100,
    gb_audio_bytes_per_sample = sizeof(int16_t),
    gb_audio_channels = 2,
    gb_audio_frame_samples = 739, // gb_frame_cycles / (gb_clock_rate / gb_sample_rate)

    gb_audio_channel_volume_shift = 6,
    gb_audio_channel_min_output = -(8 << gb_audio_channel_volume_shift),
    gb_audio_channel_max_output = +(7 << gb_audio_channel_volume_shift),

    gb_audio_mixer_buffer_samples = gb_audio_frame_samples * gb_audio_channels,

    gb_ring_buffer_frames = 2,
    gb_ring_buffer_size = gb_audio_mixer_buffer_samples * gb_audio_bytes_per_sample * gb_ring_buffer_frames
};

typedef struct _gb_t gb_t;

typedef struct _gb_frame_timer_t {
    uint32_t frame_count;
    uint32_t cycles;
    _Atomic(float) fps;
#ifdef _WIN32
    LARGE_INTERGER freq;
    LARGE_INTERGER last;
#else
    struct timespec last;
#endif
} gb_frame_timer_t;

void gb_frame_timer_init(gb_frame_timer_t* frame_timer);

void gb_frame_timer_clock(gb_frame_timer_t* frame_timer);

void gb_frame_timer_start(gb_frame_timer_t* frame_timer);

void gb_frame_timer_stop(gb_frame_timer_t* frame_timer);

float gb_frame_timer_get_fps(gb_frame_timer_t* frame_timer);


typedef struct _gb_ppu_handler_t {
    void (*callback)(void* data);
    void* data;
    uint8_t scanline;
    uint16_t cycle;
    struct _gb_ppu_handler_t* next;
} gb_ppu_handler_t;

typedef struct _gb_memory_handler_t {
    void (*write)(void*,uint8_t,uint16_t);
    uint8_t (*read)(void*,uint16_t);
    void* data;
} gb_memory_handler_t;

typedef struct _gb_cheat_code_t {
    uint8_t new_value;
    int16_t old_value;
    uint16_t address;
    bool* enabled;
    struct _gb_cheat_code_t* next;
} gb_cheat_code_t;


typedef struct _gb_pixel_fifo_entry_t {
    uint8_t palette_index;
    uint8_t color_index;
    bool priority;
    uint8_t index;
} gb_pixel_fifo_entry_t;

typedef struct _gb_pixel_fifo_t {
    gb_pixel_fifo_entry_t data[0x08];
    uint8_t front;
    uint8_t length;
} gb_pixel_fifo_t;

void gb_pixel_fifo_pop(gb_pixel_fifo_t* fifo);


typedef struct _gb_ring_buffer_t {
    uint8_t *data;
    long size;
#ifdef _WIN32
    volatile long write;
    volatile long read;
#else
    _Atomic(long) write;
    _Atomic(long) read;
#endif
} gb_ring_buffer_t;

void gb_ring_buffer_init(gb_ring_buffer_t* rb,long size);

long gb_ring_buffer_writeable(gb_ring_buffer_t* rb);

long gb_ring_buffer_readable(gb_ring_buffer_t* rb);

long gb_ring_buffer_write(gb_ring_buffer_t* rb,const uint8_t* src,long len);

long gb_ring_buffer_read(gb_ring_buffer_t* rb,uint8_t* dst,long len);

void gb_ring_buffer_clear(gb_ring_buffer_t* rb);

void gb_ring_buffer_free(gb_ring_buffer_t* rb);


bool gb_save_file(const char* path,void* data,size_t len);
bool gb_load_file(const char* path,void** data,size_t* len);

#ifdef __cplusplus
}
#endif
