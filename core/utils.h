#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRINTF_ERRNO(f) fprintf(stderr,"function: %s line: %d %s: %s\n",__func__,__LINE__,#f,strerror(errno))
#define PRINTF_ERROR(e) fprintf(stderr,"function: %s line: %d error: %s\n",__func__,__LINE__,e)

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

#ifdef __cplusplus
}
#endif
