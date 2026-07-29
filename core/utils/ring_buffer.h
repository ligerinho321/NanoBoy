#pragma once

#include "utils.h"

typedef struct _gb_ring_buffer_t {
    uint8_t *data;
    size_t size;
    gb_atomic_size_t write;
    gb_atomic_size_t read;
} gb_ring_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

void gb_ring_buffer_init(gb_ring_buffer_t* rb,size_t size);

size_t gb_ring_buffer_writeable(gb_ring_buffer_t* rb);

size_t gb_ring_buffer_readable(gb_ring_buffer_t* rb);

size_t gb_ring_buffer_write(gb_ring_buffer_t* rb,const uint8_t* src,size_t len);

size_t gb_ring_buffer_read(gb_ring_buffer_t* rb,uint8_t* dst,size_t len);

void gb_ring_buffer_clear(gb_ring_buffer_t* rb);

void gb_ring_buffer_free(gb_ring_buffer_t* rb);

#ifdef __cplusplus
}
#endif