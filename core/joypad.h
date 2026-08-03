#pragma once

#include "utils/utils.h"

typedef enum _gb_joypad_button_t {
    gb_button_down,
    gb_button_up,
    gb_button_left,
    gb_button_right,
    gb_button_start,
    gb_button_select,
    gb_button_b,
    gb_button_a,
    gb_button_count
} gb_joypad_button_t;

typedef struct gb_joypad_state_t {
    bool down : 1;
    bool up : 1;
    bool left : 1;
    bool right : 1;

    bool start : 1;
    bool select : 1;
    bool b : 1;
    bool a : 1;
} gb_joypad_state_t;

typedef void (*gb_joypad_callback_t)(void* data,gb_joypad_state_t* state);

typedef struct _gb_joypad_t {
    gb_t* gb;
    
    bool select_buttons;
    bool select_directions;

    gb_joypad_state_t state;

    bool current_edge;

    gb_joypad_callback_t callback;
    void* callback_data;

    gb_memory_handler_t register_handler;
} gb_joypad_t;


#ifdef __cplusplus
extern "C" {
#endif

void gb_joypad_init(gb_joypad_t* joypad,gb_t* gb);

const char* gb_joypad_get_button_name(int button);

void gb_joypad_set_callback(gb_joypad_t* joypad,gb_joypad_callback_t callback,void* data);
void gb_joypad_remove_callback(gb_joypad_t* joypad);

void gb_joypad_update(gb_joypad_t* joypad);

void gb_joypad_write_register(void* data,uint8_t value,uint16_t address);

uint8_t gb_joypad_read_register(void* data,uint16_t address);

bool gb_joypad_is_any_button_pressed(gb_joypad_t* joypad);

void gb_joypad_map_registers(gb_joypad_t* joypad);

void gb_joypad_reset(gb_joypad_t* joypad);

void gb_joypad_save_state(gb_joypad_t* joypad,gb_state_t* state);
void gb_joypad_load_state(gb_joypad_t* joypad,gb_state_t* state);

#ifdef __cplusplus
}
#endif

