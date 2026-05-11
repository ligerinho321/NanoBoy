#pragma once

#include "./utils.h"

typedef struct _gb_t gb_t;

typedef struct gb_joypad_key_t {
    bool down;
    bool up;
    bool left;
    bool right;

    bool start;
    bool select;
    bool b;
    bool a;
} gb_joypad_key_t;

typedef void (*gb_joypad_callback_t)(void* data,gb_joypad_key_t* key);

typedef struct _gb_joypad_t {
    gb_t* gb;
    
    bool select_buttons;
    bool select_directions;

    gb_joypad_key_t key;

    bool current_edge;

    gb_joypad_callback_t callback;
    void* data;

    gb_memory_handler_t register_handler;
} gb_joypad_t;

void gb_joypad_init(gb_joypad_t* joypad,gb_t* gb);

void gb_joypad_set_callback(gb_joypad_t* joypad,gb_joypad_callback_t callback,void* data);

void gb_joypad_update(gb_joypad_t* joypad);

void gb_joypad_write_register(void* data,uint8_t value,uint16_t address);

uint8_t gb_joypad_read_register(void* data,uint16_t address);

void gb_joypad_map(gb_joypad_t* joypad);

void gb_joypad_reset(gb_joypad_t* joypad);

