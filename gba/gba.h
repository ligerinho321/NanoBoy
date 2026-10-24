#pragma once

#include "utils.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "dma.h"
#include "keypad.h"
#include "serial.h"
#include "cartridge.h"
#include "bios.h"

typedef struct _gba_t {

    bool cartridge_inserted;
    
    gba_cpu_t cpu;
    gba_ppu_t ppu;
    gba_apu_t apu;
    gba_interrupt_t interrupt;
    gba_timer_t timer;
    gba_memory_t memory;
    gba_dma_t dma;
    gba_keypad_t keypad;
    gba_serial_t serial;
    gba_cartridge_t cartridge;
    gba_bios_t bios;

    uint64_t cycles;
} gba_t;

#ifdef __cplusplus
extern "C" {
#endif

gba_t* gba_new();

bool gba_insert_cartridge(gba_t* gba,const char* path);
void gba_remove_cartridge(gba_t* gba);

void gba_clock(gba_t* gba,int clocks);

void gba_delete(gba_t* gba);

#ifdef __cplusplus
}
#endif