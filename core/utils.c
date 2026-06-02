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