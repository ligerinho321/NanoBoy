#include "utils.h"

void gb_pixel_fifo_pop(gb_pixel_fifo_t* fifo){
    if(fifo->length == 0x00) return;

    gb_pixel_fifo_entry_t* entry = fifo->data + fifo->front;
    entry->palette_index = 0;
    entry->color_index = 0;
    entry->priority = false;
    entry->index = 0;
    
    fifo->front = (fifo->front + 0x01) & 0x07;

    fifo->length--;
}


size_t gb_ring_buffer_writeable(gb_ring_buffer_t* ring_buffer){
    size_t write = atomic_load_explicit(&ring_buffer->write,memory_order_relaxed);
    size_t read = atomic_load_explicit(&ring_buffer->read,memory_order_acquire);
    return (read + sizeof(ring_buffer->data) - write - 1) % sizeof(ring_buffer->data);
}

size_t gb_ring_buffer_readable(gb_ring_buffer_t* ring_buffer){
    size_t write = atomic_load_explicit(&ring_buffer->write,memory_order_acquire);
    size_t read = atomic_load_explicit(&ring_buffer->read,memory_order_relaxed);
    return (write + sizeof(ring_buffer->data) - read) % sizeof(ring_buffer->data);
}

size_t gb_ring_buffer_write(gb_ring_buffer_t* ring_buffer,const uint8_t* src,size_t len){
    
    size_t writable = gb_ring_buffer_writeable(ring_buffer);

    if(!writable || !len) return 0;

    len = gb_min(len,writable);

    size_t write = atomic_load_explicit(&ring_buffer->write,memory_order_relaxed);

    if(write + len > sizeof(ring_buffer->data)){
        
        size_t first_len = sizeof(ring_buffer->data) - write;
        memcpy(ring_buffer->data + write,src,first_len);

        size_t second_len = len - first_len;
        memcpy(ring_buffer->data,src + first_len,second_len);
    }
    else{
        memcpy(ring_buffer->data + write,src,len);
    }

    atomic_store_explicit(&ring_buffer->write,(write + len) % sizeof(ring_buffer->data),memory_order_release);

    return len;
}

size_t gb_ring_buffer_read(gb_ring_buffer_t* ring_buffer,uint8_t* dst,size_t len){
    
    size_t readable = gb_ring_buffer_readable(ring_buffer);

    if(!readable || !len) return 0;

    len = gb_min(len,readable);

    size_t read = atomic_load_explicit(&ring_buffer->read,memory_order_relaxed);

    if(read + len > sizeof(ring_buffer->data)){
        
        size_t first_len = sizeof(ring_buffer->data) - read;
        memcpy(dst,ring_buffer->data + read,first_len);

        size_t second_len = len - first_len;
        memcpy(dst + first_len,ring_buffer->data,second_len);
    }
    else{
        memcpy(dst,ring_buffer->data + read,len);
    }

    atomic_store_explicit(&ring_buffer->read,(read + len) % sizeof(ring_buffer->data),memory_order_release);

    return len;
}

void gb_ring_buffer_clear(gb_ring_buffer_t* ring_buffer){
    atomic_store_explicit(&ring_buffer->read,0,memory_order_relaxed);
    atomic_store_explicit(&ring_buffer->write,0,memory_order_relaxed);
}


void gb_sleep(int ms){
    #ifdef _WIN32
    Sleep(1);
    #else
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000 * ms;
    nanosleep(&ts,NULL);
    #endif
}