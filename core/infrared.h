#pragma once

#include "utils.h"

typedef struct _gb_infrared_t {
    gb_t* gb;
    uint8_t read_enabled;
    bool signal_received;
    bool led_on;
    gb_memory_descriptor_t register_descriptor;
} gb_infrared_t;

#ifdef __cplusplus
extern "C" {
#endif

void gb_infrared_init(gb_infrared_t* infrared,gb_t* gb);

void gb_infrared_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_infrared_read_register(void* data,uint16_t address);

void gb_infrared_map(gb_infrared_t* infrared);

void gb_infrared_reset(gb_infrared_t* infrared);

void gb_infrared_save_state(gb_infrared_t* infrared,gb_state_t* state);
void gb_infrared_load_state(gb_infrared_t* infrared,gb_state_t* state);

#ifdef __cplusplus
}
#endif
