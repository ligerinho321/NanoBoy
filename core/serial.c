#include "serial.h"
#include "gb.h"

void gb_serial_init(gb_serial_t* serial,gb_t* gb){
    serial->gb = gb;

    gb_serial_map_registers(serial);
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

        serial->timer = serial->freq;

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
            
            serial->clock_speed = value & 0x02;

            serial->internal_clock = value & 0x01;
            
            /*
            speed   bit1   freq  Bits/s  Bytes/s
            single  clear  512   8192    1024
            single  set    16    262144  32679
            double  clear  256   16383   2048
            double  set    8     524288  65536
            */

            serial->freq = serial->clock_speed ? 16 : 512;

            serial->timer = serial->freq;
            
            serial->bits_received = 0x00;

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
            value = (serial->transfer_enabled ? 0x80 : 0x00) | 0x7C | (serial->clock_speed ? 0x02 : 0x00) | (serial->internal_clock ? 0x01 : 0x00);
            break;
    }

    return value;
}


void gb_serial_map_registers(gb_serial_t* serial){
    
    serial->register_handler = (gb_memory_handler_t){
        gb_serial_write_register,
        gb_serial_read_register,
        serial
    };

    gb_memory_handler_t** bus = serial->gb->memory.bus;

    bus[0xFF01] = &serial->register_handler;
    bus[0xFF02] = &serial->register_handler;
}

void gb_serial_reset(gb_serial_t* serial){
    serial->sb = 0x00;
    serial->bits_received = 0x00;

    serial->transfer_enabled = false;
    serial->clock_speed = false;
    serial->internal_clock = false;

    serial->freq = 0x00;
    serial->timer = 0x00;
}