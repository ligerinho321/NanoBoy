#pragma once

#include "./utils.h"
#include "./cpu.h"
#include "./ppu.h"
#include "./apu.h"
#include "./joypad.h"
#include "./interrupt.h"
#include "./timer.h"
#include "./dma.h"
#include "./palette.h"
#include "./memory.h"
#include "./cartridge.h"

typedef enum _gb_type_t {
    gb_dmg = 0x00,
    gb_cgb = 0x01
} gb_type_t;

typedef struct _gb_t {
    gb_type_t type;
    
    gb_cpu_t cpu;
    gb_ppu_t ppu;
    gb_apu_t apu;
    gb_joypad_t joypad;
    gb_interrupt_t interrupt;
    gb_timer_t timer;
    gb_dma_t dma;
    gb_palette_t palette;
    gb_memory_t memory;
    
    bool double_speed;
    bool speed_switch_needed;
    
    gb_memory_handler_t key1_register_handler;

    uint64_t cycles;
} gb_t;

void gb_init(gb_t* gb);

void gb_master_clock(gb_t* gb);

void gb_write_key1_register(void* data,uint8_t value,uint16_t address);

uint8_t gb_read_key1_register(void* data,uint16_t address);

void gb_reset(gb_t* gb);