#include "./serial.h"
#include "./gb.h"

void gb_serial_init(gb_serial_t* serial,gb_t* gb){
    serial->gb = gb;

    serial->register_handler = (gb_memory_handler_t){
        gb_serial_write_register,
        gb_serial_read_register,
        serial
    };
}

void gb_serial_write_register(void* data,uint8_t value,uint16_t address){
    gb_serial_t* serial = (gb_serial_t*)data;

    switch(address){
        case 0xFF01: break;
        case 0xFF02: break;
    }
}

uint8_t gb_serial_read_register(void* data,uint16_t address){
    gb_serial_t* serial = (gb_serial_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF01: break;
        case 0xFF02: break;
    }

    return value;
}

void gb_serial_map_registers(gb_serial_t* serial){
    gb_memory_handler_t** bus = serial->gb->memory.bus;
    bus[0xFF01] = &serial->register_handler;
    bus[0xFF02] = &serial->register_handler;
}

void gb_serial_reset(gb_serial_t* serial){
    serial->sb = 0x00;
    serial->transfer_enabled = false;
    serial->clock_speed = false;
    serial->clock_select = false;
}