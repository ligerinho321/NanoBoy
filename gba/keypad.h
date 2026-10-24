#pragma once

#include "utils.h"

typedef enum _gba_keypad_button_t {
    gba_button_a,
    gba_button_b,
    gba_button_select,
    gba_button_start,
    gba_button_right,
    gba_button_left,
    gba_button_up,
    gba_button_down,
    gba_button_r,
    gba_button_l,
    gba_button_count
} gba_keypad_button_t;

extern const char* gba_keypad_button_names[gba_button_count];


typedef struct _gba_keypad_status_t {
    bool a : 1;
    bool b : 1;
    bool select : 1;
    bool start : 1;
    bool right : 1;
    bool left : 1;
    bool up : 1;
    bool down : 1;
    bool r : 1;
    bool l : 1;
} gba_keypad_status_t;

typedef struct _gba_keybpad_control_t {
    bool a : 1;
    bool b : 1;
    bool select : 1;
    bool start : 1;
    bool right : 1;
    bool left : 1;
    bool up : 1;
    bool down : 1;
    bool r : 1;
    bool l : 1;
    bool irq_enabled : 1;
    bool irq_condition : 1;
} gba_keypad_control_t;

typedef struct _gba_keypad_t {
    gba_t* gba;

    gba_keypad_status_t status;
    gba_keypad_control_t control;

} gba_keypad_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_keypad_init(gba_keypad_t* keypad,gba_t* gba);

void gba_keypad_write_register_raw(gba_keypad_t* keypad,uint8_t value,uint32_t address);
uint8_t gba_keypad_read_register_raw(gba_keypad_t* keypad,uint32_t address);

void gba_keypad_reset(gba_keypad_t* keypad);

#ifdef __cplusplus
}
#endif