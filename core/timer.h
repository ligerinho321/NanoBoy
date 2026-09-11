#pragma once

#include "utils.h"

typedef struct _gb_timer_state_t {
    uint16_t div;
    uint8_t tima;
    uint8_t tma;

    uint8_t clock_select : 2;
    bool enabled : 1;

    bool tima_reload_request : 1;
    bool tima_reloaded : 1;
    
    uint64_t last_schedule_event;
    uint64_t next_schedule_event;
} gb_timer_state_t;

typedef struct _gb_timer_t {
    gb_t* gb;

    gb_timer_state_t state;

    gb_memory_descriptor_t register_descriptor;
} gb_timer_t;


#ifdef __cplusplus
extern "C" {
#endif

void gb_timer_init(gb_timer_t* timer,gb_t* gb);

void gb_timer_schedule_next_event(gb_timer_t* timer);

void gb_timer_set_div(gb_timer_t* timer,uint16_t new_div);

void gb_timer_update(gb_timer_t* timer);

void gb_timer_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_timer_read_register(void* data,uint16_t address);

void gb_timer_map_registers(gb_timer_t* timer);

void gb_timer_reset(gb_timer_t* timer);
void gb_timer_skip_boot(gb_timer_t* timer);

void gb_timer_save_state(gb_timer_t* timer,gb_snapshot_t* snapshot);
void gb_timer_load_state(gb_timer_t* timer,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif