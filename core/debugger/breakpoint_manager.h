#pragma once

#include "../utils.h"

typedef void (*gb_breakpoint_manager_callback_t)(void* userdata);

typedef struct _gb_breakpoint_t {
    bool enabled;
    uint8_t memory_type;
    size_t address;
    struct _gb_breakpoint_t* next;
} gb_breakpoint_t;

typedef struct _gb_breakpoint_manager_t {
    gb_t* gb;
    
    bool enabled;
    uint32_t last_check_address;
    
    gb_breakpoint_t* breakpoints;
} gb_breakpoint_manager_t;

#ifdef __cplusplus
extern "C" {
#endif

void gb_breakpoint_manager_init(gb_breakpoint_manager_t* breakpoint_manager,gb_t* gb);

bool gb_breakpoint_manager_check(gb_breakpoint_manager_t* breakpoint_manager,uint16_t address);

void gb_breakpoint_manager_enable(gb_t* gb,bool enabled);

void gb_breakpoint_manager_add(gb_t* gb,gb_breakpoint_t* breakpoint);
void gb_breakpoint_manager_remove(gb_t* gb,gb_breakpoint_t* breakpoint);
void gb_breakpoint_manager_clear(gb_t* gb);

void gb_breakpoint_manager_reset(gb_breakpoint_manager_t* breakpoint_manager);

#ifdef __cplusplus
}
#endif