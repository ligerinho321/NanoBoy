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

    infrared->state.read_enabled = (value & 0xC0) >> 0x06;
    infrared->state.led_on = value & 0x01;
}

uint8_t gb_infrared_read_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_infrared_t* infrared = (gb_infrared_t*)data;
    
    uint8_t value = ((infrared->state.read_enabled & 0x03) << 0x06) | 0x3C | (infrared->state.led_on ? 0x01 : 0x00);
    
    if(infrared->state.read_enabled == 0x03){
        value |= infrared->state.signal_received ? 0x02 : 0x00;
    }
    else{
        value |= 0x02;
    }

    return value;
}


void gb_infrared_reset(gb_infrared_t* infrared){
    memset(&infrared->state,0x00,sizeof(infrared->state));
}


void gb_infrared_save_state(gb_infrared_t* infrared,gb_snapshot_t* snapshot){
    snapshot->infrared = infrared->state;
}

void gb_infrared_load_state(gb_infrared_t* infrared,gb_snapshot_t* snapshot){
    infrared->state = snapshot->infrared;
}