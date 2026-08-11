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

    if(!serial->transfer_enabled) return;

    if(serial->internal_clock){
        
        serial->timer -= 0x04;

        if(serial->timer > 0x00) return;

        serial->timer = serial->clock_speed ? 16 : 512;

        bool send_bit = serial->sb & 0x80;

        serial->sb <<= 0x01;

        if(serial->callback != NULL){
            serial->sb |= serial->callback(serial->data,send_bit);
        }
        else{
            serial->sb |= 0x01;
        }

        if(++serial->bits_received >= 0x08){
            serial->transfer_enabled = false;
            serial->gb->interrupt.flag |= gb_interrupt_serial_flag;
        }
    }
}


void gb_serial_write_register(void* data,uint8_t value,uint16_t address){
    gb_serial_t* serial = (gb_serial_t*)data;

    switch(address){
        //Serial Byte
        case 0xFF01:
            serial->sb = value;
            break;
        //Serial Control
        case 0xFF02:
            serial->transfer_enabled = value & 0x80;

            if(serial->gb->cgb_mode){
                serial->clock_speed = value & 0x02;
            }

            serial->internal_clock = value & 0x01;
            
            /*
            speed   bit1   freq  Bits/s  Bytes/s
            single  clear  512   8192    1024
            single  set    16    262144  32679
            double  clear  256   16383   2048
            double  set    8     524288  65536
            */

            if(serial->transfer_enabled){
                serial->timer = serial->clock_speed ? 16 : 512;
                serial->bits_received = 0x00;
            }

            break;
    }
}

uint8_t gb_serial_read_register(void* data,uint16_t address){
    gb_serial_t* serial = (gb_serial_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        //Serial Byte
        case 0xFF01:
            value = serial->sb;
            break;
        //Serial Control
        case 0xFF02:
            value = serial->transfer_enabled ? 0x80 : 0x00;
            
            if(serial->gb->cgb_mode){
                
                value |= 0x7C;
                
                value |= serial->clock_speed ? 0x02 : 0x00;
            }
            else{
                value |= 0x7E;
            }
            
            value |= serial->internal_clock ? 0x01 : 0x00;

            break;
    }

    return value;
}


void gb_serial_map_registers(gb_serial_t* serial){
    gb_memory_map_in_range(&serial->gb->memory,&serial->register_descriptor,0xFF01,0xFF02);
}


void gb_serial_reset(gb_serial_t* serial){
    serial->sb = 0x00;
    
    serial->transfer_enabled = false;
    serial->clock_speed = false;
    serial->internal_clock = false;

    serial->bits_received = 0x00;
    serial->timer = 0x00;
}


void gb_serial_save_state(gb_serial_t* serial,gb_state_t* state){
    gb_state_write(state,serial->sb);
    gb_state_write(state,serial->bits_received);

    gb_state_write(state,serial->transfer_enabled);
    gb_state_write(state,serial->clock_speed);
    gb_state_write(state,serial->internal_clock);

    gb_state_write(state,serial->timer);
}

void gb_serial_load_state(gb_serial_t* serial,gb_state_t* state){
    gb_state_read(state,serial->sb);
    gb_state_read(state,serial->bits_received);

    gb_state_read(state,serial->transfer_enabled);
    gb_state_read(state,serial->clock_speed);
    gb_state_read(state,serial->internal_clock);

    gb_state_read(state,serial->timer);
}