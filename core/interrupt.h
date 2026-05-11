#pragma once

#include "./utils.h"

typedef struct _gb_t gb_t;

typedef enum _gb_interrupt_flag_t {
    gb_interrupt_vblank_flag = 0x01,
    gb_interrupt_lcd_flag = 0x02,
    gb_interrupt_timer_flag = 0x04,
    gb_interrupt_serial_flag = 0x08,
    gb_interrupt_joypad_flag = 0x10
} gb_interrupt_flag_t;

typedef enum _gb_interrupt_vector_t {
    gb_interrupt_vblank_vector = 0x40,
    gb_interrupt_lcd_vector = 0x48,
    gb_interrupt_timer_vector = 0x50,
    gb_interrupt_serial_vector = 0x58,
    gb_interrupt_joypad_vector = 0x60
} gb_interrupt_source_t;

typedef struct _gb_interrupt_t {
    gb_t* gb;

    uint8_t enable;
    uint8_t flag;

    gb_memory_handler_t enable_register_handler;
    gb_memory_handler_t flag_register_handler;
} gb_interrupt_t;

void gb_interrupt_init(gb_interrupt_t* interrupt,gb_t* gb);

void gb_interrupt_write_flag_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_interrupt_read_flag_register(void* data,uint16_t address);

void gb_interrupt_write_enable_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_interrupt_read_enable_register(void* data,uint16_t address);

uint8_t gb_interrupt_get_vector(gb_interrupt_t* interrupt);

void gb_interrupt_map(gb_interrupt_t* interrupt);

void gb_interrupt_reset(gb_interrupt_t* interrupt);