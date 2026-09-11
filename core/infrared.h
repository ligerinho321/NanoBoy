#pragma once

#include "utils.h"

typedef struct _gb_infrared_state_t {
    uint8_t read_enabled : 2;
    bool signal_received : 1;
    bool led_on : 1;
} gb_infrared_state_t;

typedef struct _gb_infrared_t {
    gb_t* gb;
    
    gb_infrared_state_t state;

    gb_memory_descriptor_t register_descriptor;
} gb_infrared_t;

#ifdef __cplusplus
extern "C" {
#endif

void gb_infrared_init(gb_infrared_t* infrared,gb_t* gb);

void gb_infrared_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_infrared_read_register(void* data,uint16_t address);

void gb_infrared_reset(gb_infrared_t* infrared);

void gb_infrared_save_state(gb_infrared_t* infrared,gb_snapshot_t* snapshot);
void gb_infrared_load_state(gb_infrared_t* infrared,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif
