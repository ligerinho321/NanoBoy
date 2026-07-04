#include "utils.h"

void gb_frame_timer_init(gb_frame_timer_t* frame_timer){
#ifdef _WIN32
    if(!QueryPerformanceFrequency(&frame_timer->frequency)){
        printf("QueryPerformanceFrequency failed");
    }
#endif
}

void gb_frame_timer_clock(gb_frame_timer_t* frame_timer){
    
    ++frame_timer->frame_count;

#ifdef _WIN32
    LARGE_INTERGER current = {0};

    if(!QueryPerformanceCounter(&frame_timer->last)){
        gb_printf_error("QueryPerformanceCounter failed");
    }

    float elapsed = (float)(current.QuadPart - frame_timer->last.QuadPart) / (float)frame_timer->freq.QuadPart;
#else
    struct timespec current = {0};
        
    if(clock_gettime(CLOCK_MONOTONIC,&current) < 0){
        gb_printf_errno(clock_gettime);
    }

    float elapsed = (current.tv_sec - frame_timer->last.tv_sec) + ((current.tv_nsec - frame_timer->last.tv_nsec) / 1e+9);
#endif

    if(elapsed > 1.0f){
        frame_timer->last = current;
        atomic_store_explicit(&frame_timer->fps,frame_timer->frame_count / elapsed,memory_order_release);
        frame_timer->frame_count = 0;
    }
}

void gb_frame_timer_start(gb_frame_timer_t* frame_timer){
    frame_timer->frame_count = 0;
    frame_timer->cycles = 0;
#ifdef _WIN32
    if(!QueryPerformanceCounter(&frame_timer->last)){
        gb_printf_error("QueryPerformanceCounter failed");
    }
#else
    if(clock_gettime(CLOCK_MONOTONIC,&frame_timer->last) < 0){
        gb_printf_errno(clock_gettime);
    }
#endif
}

void gb_frame_timer_stop(gb_frame_timer_t* frame_timer){
    atomic_store_explicit(&frame_timer->fps,0.0f,memory_order_release);
}

float gb_frame_timer_get_fps(gb_frame_timer_t* frame_timer){
    return atomic_load_explicit(&frame_timer->fps,memory_order_acquire);
}


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


void gb_ring_buffer_init(gb_ring_buffer_t* rb,long size){
    rb->data = (uint8_t*)malloc(size + 1);
    rb->size = size + 1;
    rb->write = 0;
    rb->read = 0;
}

long gb_ring_buffer_writeable(gb_ring_buffer_t* rb){
#ifdef _WIN32
    return (rb->read + rb->size - rb->write - 1) % rb->size;
#else
    long write = atomic_load_explicit(&rb->write,memory_order_relaxed);
    long read = atomic_load_explicit(&rb->read,memory_order_acquire);
    return (read + rb->size - write - 1) % rb->size;
#endif
    
}

long gb_ring_buffer_readable(gb_ring_buffer_t* rb){
#ifdef _WIN32
    return (rb->write + rb->size - rb->read) % rb->size;
#else
    long write = atomic_load_explicit(&rb->write, memory_order_acquire);
    long read = atomic_load_explicit(&rb->read, memory_order_relaxed);
    return (write + rb->size - read) % rb->size;
#endif
}

long gb_ring_buffer_write(gb_ring_buffer_t* rb,const uint8_t* src,long len){
    
    long writable = gb_ring_buffer_writeable(rb);
    
    if(!writable || !len) return 0;

    len = gb_min(len,writable);

#ifdef _WIN32
    long write = rb->write;
#else
    long write = atomic_load_explicit(&rb->write,memory_order_relaxed);
#endif

    if(write + len > rb->size){
        
        long first_len = rb->size - write;
        memcpy(rb->data + write,src,first_len);

        long second_len = len - first_len;
        memcpy(rb->data,src + first_len,second_len);
    }
    else{
        memcpy(rb->data + write,src,len);
    }

#ifdef _WIN32
    InterlockedExchange(&rb->write,(write + len) % rb->size);
#else
    atomic_store_explicit(&rb->write,(write + len) % rb->size,memory_order_release);
#endif

    return len;
}

long gb_ring_buffer_read(gb_ring_buffer_t* rb,uint8_t* dst,long len){
    
    long readable = gb_ring_buffer_readable(rb);

    if(!readable || !len) return 0;

    len = gb_min(len,readable);

#ifdef _WIN32
    long read = rb->read;
#else
    long read = atomic_load_explicit(&rb->read,memory_order_relaxed);
#endif

    if(read + len > rb->size){
        
        long first_len = rb->size - read;
        memcpy(dst,rb->data + read,first_len);

        long second_len = len - first_len;
        memcpy(dst + first_len,rb->data,second_len);
    }
    else{
        memcpy(dst,rb->data + read,len);
    }

#ifdef _WIN32
    InterlockedExchange(&rb->read,(read + len) % rb->size);
#else
    atomic_store_explicit(&rb->read,(read + len) % rb->size,memory_order_release);
#endif

    return len;
}

void gb_ring_buffer_clear(gb_ring_buffer_t* rb){
#ifdef _WIN32
    InterlockedExchange(&rb->write,0);
    InterlockedExchange(&rb->read,0);
#else
    atomic_store_explicit(&rb->write,0,memory_order_relaxed);
    atomic_store_explicit(&rb->read,0,memory_order_relaxed);
#endif
}

void gb_ring_buffer_free(gb_ring_buffer_t* ring_buffer){
    free(ring_buffer->data);
}


bool gb_save_file(const char* path,void* data,size_t len){
    FILE* file = fopen(path,"wb");
    if(!file){
        gb_printf_errno(fopen);
        return false;
    }

    fwrite(data,1,len,file);

    fclose(file);
    return true;
}

bool gb_load_file(const char* path,void** data,size_t* len){

    *data = NULL;
    *len = 0;

    size_t read_bytes;

    FILE* file = fopen(path,"rb");

    if(!file){
        gb_printf_errno(fopen);
        goto fail;
    }

    fseek(file,0,SEEK_END);
    *len = ftell(file);
    fseek(file,0,SEEK_SET);

    if(!*len) goto fail;

    *data = malloc(*len);
    if(!*data){
        gb_printf_errno(malloc);
        goto fail;
    }

    read_bytes = fread(*data,1,*len,file);
    if(read_bytes != *len){
        gb_printf_error("fread failed");
        goto fail;
    }
    
    fclose(file);
    return true;

    fail:
    if(file != NULL) fclose(file);
    
    if(*data != NULL){
        free(*data);
        *data = NULL;
    }
    
    *len = 0;

    return false;
}