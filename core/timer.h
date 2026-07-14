#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_timer_t {
    gb_t* gb;

    uint16_t div;
    uint8_t tima;
    uint8_t tma;

    bool enabled;
    uint8_t clock_select;

    uint16_t div_bit;
    bool tima_reload_request;
    bool tima_reloaded;

    uint16_t apu_div_bit;
    
    gb_memory_handler_t register_handler;
} gb_timer_t;

void gb_timer_init(gb_timer_t* timer,gb_t* gb);

void gb_timer_set_div(gb_timer_t* timer,uint16_t new_div);

void gb_timer_clock(gb_timer_t* timer);

void gb_timer_write_register(void* data,uint8_t value,uint16_t address);

uint8_t gb_timer_read_register(void* data,uint16_t address);

void gb_timer_map_registers(gb_timer_t* timer);

void gb_timer_reset(gb_timer_t* timer);

#ifdef __cplusplus
}
#endif