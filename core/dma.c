#include "./dma.h"
#include "./gb.h"

void gb_dma_init(gb_dma_t* dma,gb_t* gb){
    dma->gb = gb;

    dma->oam_register_handler = (gb_memory_handler_t){
        gb_dma_oam_write_register,
        gb_dma_oam_read_register,
        dma
    };
}

void gb_dma_oam_clock(gb_dma_t* dma){
    if((!dma->oam_running && !dma->oam_setup_cycle) || dma->gb->cpu.halted) return;

    if(!dma->oam_setup_cycle){
        
        dma->oam_byte = gb_memory_read(&dma->gb->memory,(dma->oam_src_addr << 0x08) | dma->oam_index);
        
        dma->gb->memory.oam[dma->oam_index] = dma->oam_byte;

        if(++dma->oam_index >= 0xA0){
            dma->oam_running = false;
        }
    }
    else{
        dma->oam_setup_cycle = false;
        dma->oam_running = true;
    }
}

void gb_dma_oam_write_register(void* data,uint8_t value,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;
    dma->oam_setup_cycle = true;
    dma->oam_src_addr = value;
    dma->oam_index = 0x00;
}

uint8_t gb_dma_oam_read_register(void* data,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;
    return dma->oam_byte;
}

void gb_dma_oam_map(gb_dma_t* dma){
    gb_memory_handler_t** bus = dma->gb->memory.bus;
    bus[0xFF46] = &dma->oam_register_handler;
}

void gb_dma_reset(gb_dma_t* dma){
    dma->oam_running = false;
    dma->oam_setup_cycle = false;

    dma->oam_src_addr = 0x00;
    dma->oam_index = 0x00;
    dma->oam_byte = 0x00;
}