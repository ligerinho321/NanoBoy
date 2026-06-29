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


void gb_ring_buffer_init(gb_ring_buffer_t* ring_buffer,size_t size){
    ring_buffer->data = (uint8_t*)malloc(size + 1);
    ring_buffer->size = size + 1;
    ring_buffer->write = 0;
    ring_buffer->read = 0;
}

size_t gb_ring_buffer_writeable(gb_ring_buffer_t* ring_buffer){
    size_t write = ring_buffer->write;
    size_t read = ring_buffer->read;
    return (read + ring_buffer->size - write - 1) % ring_buffer->size;
}

size_t gb_ring_buffer_readable(gb_ring_buffer_t* ring_buffer){
    size_t write = ring_buffer->write;
    size_t read = ring_buffer->read;
    return (write + ring_buffer->size - read) % ring_buffer->size;
}

size_t gb_ring_buffer_write(gb_ring_buffer_t* ring_buffer,const uint8_t* src,size_t len){
    
    size_t writable = gb_ring_buffer_writeable(ring_buffer);

    if(!writable || !len) return 0;

    len = gb_min(len,writable);

    size_t write = ring_buffer->write;

    if(write + len > ring_buffer->size){
        
        size_t first_len = ring_buffer->size - write;
        memcpy(ring_buffer->data + write,src,first_len);

        size_t second_len = len - first_len;
        memcpy(ring_buffer->data,src + first_len,second_len);
    }
    else{
        memcpy(ring_buffer->data + write,src,len);
    }

    ring_buffer->write = (write + len) % ring_buffer->size;

    return len;
}

size_t gb_ring_buffer_read(gb_ring_buffer_t* ring_buffer,uint8_t* dst,size_t len){
    
    size_t readable = gb_ring_buffer_readable(ring_buffer);

    if(!readable || !len) return 0;

    len = gb_min(len,readable);

    size_t read = ring_buffer->read;

    if(read + len > ring_buffer->size){
        
        size_t first_len = ring_buffer->size - read;
        memcpy(dst,ring_buffer->data + read,first_len);

        size_t second_len = len - first_len;
        memcpy(dst + first_len,ring_buffer->data,second_len);
    }
    else{
        memcpy(dst,ring_buffer->data + read,len);
    }

    ring_buffer->read = (read + len) % ring_buffer->size;

    return len;
}

void gb_ring_buffer_clear(gb_ring_buffer_t* ring_buffer){
    ring_buffer->read = 0;
    ring_buffer->write = 0;
}

void gb_ring_buffer_free(gb_ring_buffer_t* ring_buffer){
    free(ring_buffer->data);
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