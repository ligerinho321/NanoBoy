#include "serial.h"
#include "gb.h"

void gb_serial_init(gb_serial_t* serial,gb_t* gb){
    serial->gb = gb;

    serial->register_descriptor = (gb_memory_descriptor_t){
        gb_serial_write_register,
        gb_serial_read_register,
        serial
    };
}


void gb_serial_set_callback(gb_serial_t* serial,gb_serial_callback_t callback,void* data){
    serial->callback = callback;
    serial->data = data;
}

void gb_serial_remove_callback(gb_serial_t* serial){
    serial->callback = NULL;
    serial->data = NULL;
}


void gb_serial_clock(gb_serial_t* serial){

    gb_serial_state_t* state = &serial->state;

    if(state->internal_clock){
        
        state->timer -= 0x04;

        if(state->timer > 0x00) return;

        state->timer = state->clock_speed ? 16 : 512;

        bool send_bit = state->sb & 0x80;

        state->sb <<= 0x01;

        if(serial->callback != NULL){
            state->sb |= serial->callback(serial->data,send_bit);
        }
        else{
            state->sb |= 0x01;
        }

        if(++state->bits_received >= 0x08){
            state->transfer_enabled = false;
            serial->gb->interrupt.state.flag |= gb_interrupt_serial_flag;
        }
    }
}


void gb_serial_write_register(void* data,uint8_t value,uint16_t address){
    gb_serial_t* serial = (gb_serial_t*)data;
    gb_serial_state_t* state = &serial->state;

    switch(address){
        //Serial Byte
        case 0xFF01:
            state->sb = value;
            break;
        //Serial Control
        case 0xFF02:
            state->transfer_enabled = value & 0x80;

            if(serial->gb->state.cgb_mode){
                state->clock_speed = value & 0x02;
            }

            state->internal_clock = value & 0x01;
            
            /*
            speed   bit1   freq  Bits/s  Bytes/s
            single  clear  512   8192    1024
            single  set    16    262144  32679
            double  clear  256   16383   2048
            double  set    8     524288  65536
            */

            if(state->transfer_enabled){
                state->timer = state->clock_speed ? 16 : 512;
                state->bits_received = 0x00;
            }

            break;
    }
}

uint8_t gb_serial_read_register(void* data,uint16_t address){
    gb_serial_t* serial = (gb_serial_t*)data;
    gb_serial_state_t* state = &serial->state;

    uint8_t value = 0xFF;

    switch(address){
        //Serial Byte
        case 0xFF01:
            value = state->sb;
            break;
        //Serial Control
        case 0xFF02:
            value = state->transfer_enabled ? 0x80 : 0x00;
            
            if(serial->gb->state.cgb_mode){
                
                value |= 0x7C;
                
                value |= state->clock_speed ? 0x02 : 0x00;
            }
            else{
                value |= 0x7E;
            }
            
            value |= state->internal_clock ? 0x01 : 0x00;

            break;
    }

    return value;
}


void gb_serial_map_registers(gb_serial_t* serial){
    gb_memory_map_in_range(&serial->gb->memory,&serial->register_descriptor,0xFF01,0xFF02);
}


void gb_serial_reset(gb_serial_t* serial){
    memset(&serial->state,0x00,sizeof(serial->state));
}


void gb_serial_save_state(gb_serial_t* serial,gb_snapshot_t* snapshot){
    snapshot->serial = serial->state;
}

void gb_serial_load_state(gb_serial_t* serial,gb_snapshot_t* snapshot){
    serial->state = snapshot->serial;
}