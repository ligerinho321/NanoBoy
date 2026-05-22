#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_serial_t {
    gb_t* gb;

    uint8_t sb;
    bool transfer_enabled;
    bool clock_speed;
    bool clock_select;

    gb_memory_handler_t register_handler;
} gb_serial_t;


void gb_serial_init(gb_serial_t* serial,gb_t* gb);

void gb_serial_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_serial_read_register(void* data,uint16_t address);

void gb_serial_map_registers(gb_serial_t* serial);

void gb_serial_reset(gb_serial_t* serial);

#ifdef __cplusplus
}
#endif