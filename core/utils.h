#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdatomic.h>

#ifdef _WIN32
#include <windows.h> // Sleep()
#else
#include <time.h> // nanosleep()
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define gb_printf_errno(f) fprintf(stderr,"function: %s line: %d %s: %s\n",__func__,__LINE__,#f,strerror(errno))
#define gb_printf_error(e) fprintf(stderr,"function: %s line: %d error: %s\n",__func__,__LINE__,e)

#define gb_min(x,y) ((x < y) ? x : y)

#define gb_clock_rate 4194304

#define gb_screen_width 160
#define gb_screen_height 144
#define gb_screen_bytes_per_pixel 3
#define gb_screen_pitch 480 // gb_screen_width * gb_screen_bytes_per_pixel
#define gb_screen_length 69120 // gb_screen_pitch * gb_screen_height

#define gb_vblank_scanline 144
#define gb_scanlines 154
#define gb_scanline_cycles 456
#define gb_frame_cycles 70224

#define gb_audio_sample_rate 44100
#define gb_audio_bytes_per_sample 2
#define gb_audio_channels 2
#define gb_audio_frame_samples 1293 // gb_frame_cycles / (gb_clock_rate / (gb_sample_rate * 1.75))

#define gb_audio_buffer_samples (gb_audio_frame_samples * gb_audio_channels)

#define gb_ring_buffer_frames 4
#define gb_ring_buffer_length (gb_audio_frame_samples * gb_audio_channels * gb_audio_bytes_per_sample * gb_ring_buffer_frames + 1)

typedef struct _gb_t gb_t;

typedef struct _gb_callback_handler_t {
    void (*callback)(void* data);
    void* data;
    uint8_t scanline;
    uint16_t cycle;
    struct _gb_callback_handler_t* next;
} gb_callback_handler_t;

typedef struct _gb_memory_handler_t {
    void (*write)(void*,uint8_t,uint16_t);
    uint8_t (*read)(void*,uint16_t);
    void* data;
} gb_memory_handler_t;


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
    uint8_t data[gb_ring_buffer_length];
    atomic_size_t read;
    atomic_size_t write;
} gb_ring_buffer_t;

size_t gb_ring_buffer_writeable(gb_ring_buffer_t* ring_buffer);

size_t gb_ring_buffer_readable(gb_ring_buffer_t* ring_buffer);

size_t gb_ring_buffer_write(gb_ring_buffer_t* ring_buffer,const uint8_t* src,size_t len);

size_t gb_ring_buffer_read(gb_ring_buffer_t* ring_buffer,uint8_t* dst,size_t len);

void gb_ring_buffer_clear(gb_ring_buffer_t* ring_buffer);

void gb_sleep(int ms);

#ifdef __cplusplus
}
#endif
