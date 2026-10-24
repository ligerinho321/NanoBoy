#pragma once

#include "utils.h"

typedef struct _gba_timer_control_t{
    uint8_t prescaler;
    bool countup;
    bool irq_enabled;
    bool enabled;
} gba_timer_control_t;

typedef struct _gba_timer_counter_t {
    uint32_t accumulate;
    uint16_t value;
    gba_halfword_t reload_value;
    gba_timer_control_t control;
} gba_timer_counter_t;

typedef struct _gba_timer_t {
    gba_t* gba;

    gba_timer_counter_t counter[4];

    uint64_t last_schedule_event;
    uint64_t next_schedule_event;
} gba_timer_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_timer_init(gba_timer_t* timer,gba_t* gba);

void gba_timer_schedule_next_event(gba_timer_t* timer);
void gba_timer_update(gba_timer_t* timer);

void gba_timer_write_register_raw(gba_timer_t* timer,uint8_t value,uint32_t address);
uint8_t gba_timer_read_register_raw(gba_timer_t* timer,uint32_t address);

void gba_timer_reset(gba_timer_t* timer);

#ifdef __cplusplus
}
#endif