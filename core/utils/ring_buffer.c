#include "ring_buffer.h"

void gb_ring_buffer_init(gb_ring_buffer_t* rb,size_t size){
    rb->data = (uint8_t*)malloc(size + 1);
    rb->size = size + 1;
    rb->write = 0;
    rb->read = 0;
}

size_t gb_ring_buffer_writeable(gb_ring_buffer_t* rb){
    size_t write = gb_atomic_load_explicit(&rb->write,gb_memory_order_relaxed);
    size_t read = gb_atomic_load_explicit(&rb->read,gb_memory_order_acquire);
    return (read + rb->size - write - 1) % rb->size;    
}

size_t gb_ring_buffer_readable(gb_ring_buffer_t* rb){
    size_t write = gb_atomic_load_explicit(&rb->write,gb_memory_order_acquire);
    size_t read = gb_atomic_load_explicit(&rb->read,gb_memory_order_relaxed);
    return (write + rb->size - read) % rb->size;
}

size_t gb_ring_buffer_write(gb_ring_buffer_t* rb,const uint8_t* src,size_t len){
    
    size_t writable = gb_ring_buffer_writeable(rb);
    
    if(!writable || !len) return 0;

    len = gb_min(len,writable);

    size_t write = gb_atomic_load_explicit(&rb->write,gb_memory_order_relaxed);

    if(write + len > rb->size){
        
        size_t first_len = rb->size - write;
        memcpy(rb->data + write,src,first_len);

        size_t second_len = len - first_len;
        memcpy(rb->data,src + first_len,second_len);
    }
    else{
        memcpy(rb->data + write,src,len);
    }

    gb_atomic_store_explicit(&rb->write,(write + len) % rb->size,gb_memory_order_release);

    return len;
}

size_t gb_ring_buffer_read(gb_ring_buffer_t* rb,uint8_t* dst,size_t len){
    
    size_t readable = gb_ring_buffer_readable(rb);

    if(!readable || !len) return 0;

    len = gb_min(len,readable);

    size_t read = gb_atomic_load_explicit(&rb->read,gb_memory_order_relaxed);

    if(read + len > rb->size){
        
        size_t first_len = rb->size - read;
        memcpy(dst,rb->data + read,first_len);

        size_t second_len = len - first_len;
        memcpy(dst + first_len,rb->data,second_len);
    }
    else{
        memcpy(dst,rb->data + read,len);
    }

    gb_atomic_store_explicit(&rb->read,(read + len) % rb->size,gb_memory_order_release);

    return len;
}

void gb_ring_buffer_clear(gb_ring_buffer_t* rb){
    gb_atomic_store_explicit(&rb->write,0,gb_memory_order_relaxed);
    gb_atomic_store_explicit(&rb->read,0,gb_memory_order_relaxed);
}

void gb_ring_buffer_free(gb_ring_buffer_t* ring_buffer){
    free(ring_buffer->data);
}
