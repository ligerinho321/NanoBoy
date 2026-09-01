#include "infrared.h"
#include "gb.h"

void gb_infrared_init(gb_infrared_t* infrared,gb_t* gb){
    infrared->gb = gb;

    infrared->register_descriptor = (gb_memory_descriptor_t){
        gb_infrared_write_register,
        gb_infrared_read_register,
        infrared
    };
}


void gb_infrared_write_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_infrared_t* infrared = (gb_infrared_t*)data;

    infrared->read_enabled = (value & 0xC0) >> 0x06;
    infrared->led_on = value & 0x01;
}

uint8_t gb_infrared_read_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_infrared_t* infrared = (gb_infrared_t*)data;
    
    uint8_t value = ((infrared->read_enabled & 0x03) << 0x06) | 0x3C | (infrared->led_on ? 0x01 : 0x00);
    
    if(infrared->read_enabled == 0x03){
        value |= infrared->signal_received ? 0x02 : 0x00;
    }
    else{
        value |= 0x02;
    }

    return value;
}


void gb_infrared_reset(gb_infrared_t* infrared){
    infrared->read_enabled = false;
    infrared->signal_received = false;
    infrared->led_on = false;
}


void gb_infrared_save_state(gb_infrared_t* infrared,gb_state_t* state){
    gb_state_write(state,infrared->read_enabled);
    gb_state_write(state,infrared->signal_received);
    gb_state_write(state,infrared->led_on);
}

void gb_infrared_load_state(gb_infrared_t* infrared,gb_state_t* state){
    gb_state_read(state,infrared->read_enabled);
    gb_state_read(state,infrared->signal_received);
    gb_state_read(state,infrared->led_on);
}