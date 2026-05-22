#include "apu.h"
#include "gb.h"

void gb_apu_init(gb_apu_t* apu,gb_t* gb){
    apu->gb = gb;

    apu->pcm12_register_handler = (gb_memory_handler_t){
        NULL,
        gb_apu_read_pcm12_register,
        apu
    };

    apu->pcm34_register_handler = (gb_memory_handler_t){
        NULL,
        gb_apu_read_pcm34_register,
        apu
    };
}

void gb_apu_frame_sequency_clock(gb_apu_t* apu){

}


uint8_t gb_apu_read_pcm12_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    return 0x00;
}

uint8_t gb_apu_read_pcm34_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    return 0x00;
}

void gb_apu_map_registers(gb_apu_t* apu){

}

void gb_apu_map_pcm_registers(gb_apu_t* apu){
    gb_memory_handler_t** bus = apu->gb->memory.bus;
    bus[0xFF76] = &apu->pcm12_register_handler;
    bus[0xFF77] = &apu->pcm34_register_handler;
}

void gb_apu_unmap_pcm_registers(gb_apu_t* apu){
    gb_memory_handler_t** bus = apu->gb->memory.bus;
    bus[0xFF76] = NULL;
    bus[0xFF77] = NULL;
}

void gb_apu_reset(gb_apu_t* apu){
    
}