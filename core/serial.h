#pragma once

#include "utils.h"

typedef bool (*gb_serial_callback_t)(void* data,bool bit);

typedef struct _gb_serial_state_t {
    uint8_t sb;
    uint8_t bits_received;

    bool transfer_enabled : 1;
    bool clock_speed : 1;
    bool internal_clock : 1;

    int timer;
} gb_serial_state_t;

typedef struct _gb_serial_t {
    gb_t* gb;

    gb_serial_state_t state;
    
    gb_memory_descriptor_t register_descriptor;

    gb_serial_callback_t callback;
    void* data;
} gb_serial_t;


#ifdef __cplusplus
extern "C" {
#endif

void gb_serial_init(gb_serial_t* serial,gb_t* gb);

void gb_serial_set_callback(gb_serial_t* serial,gb_serial_callback_t callback,void* data);
void gb_serial_remove_callback(gb_serial_t* serial);

void gb_serial_clock(gb_serial_t* serial);

void gb_serial_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_serial_read_register(void* data,uint16_t address);

void gb_serial_map_registers(gb_serial_t* serial);

void gb_serial_reset(gb_serial_t* serial);

void gb_serial_save_state(gb_serial_t* serial,gb_snapshot_t* snapshot);
void gb_serial_load_state(gb_serial_t* serial,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif