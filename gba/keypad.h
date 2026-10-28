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

typedef void (*gba_keypad_callback_t)(void* data,uint8_t* status);

typedef struct _gba_keypad_t {
    gba_t* gba;

    uint8_t status[gba_button_count];
    uint8_t select[gba_button_count];
    bool irq_enabled;
    bool irq_condition;

    gba_keypad_callback_t callback;
    void* callback_data;
} gba_keypad_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_keypad_init(gba_keypad_t* keypad,gba_t* gba);

void gba_keypad_set_callback(gba_t* gba,gba_keypad_callback_t callback,void* data);
void gba_keypad_remove_callback(gba_t* gba);

void gba_keypad_update(gba_keypad_t* keypad);

void gba_keypad_write_register_raw(gba_keypad_t* keypad,uint8_t value,uint32_t address);
uint8_t gba_keypad_read_register_raw(gba_keypad_t* keypad,uint32_t address);

void gba_keypad_reset(gba_keypad_t* keypad);

#ifdef __cplusplus
}
#endif