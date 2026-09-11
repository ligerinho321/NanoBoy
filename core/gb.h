#pragma once

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "joypad.h"
#include "interrupt.h"
#include "timer.h"
#include "dma.h"
#include "palette.h"
#include "serial.h"
#include "infrared.h"
#include "boot.h"
#include "memory.h"
#include "cartridge.h"
#include "printer.h"
#include "rewind.h"
#include "savestate.h"

#include "debugger/breakpoint_manager.h"
#include "debugger/disassembler.h"
#include "debugger/event_manager.h"
#include "debugger/memory_type.h"
#include "debugger/registers_label.h"

typedef struct _gb_state_t {
    bool is_cgb;
    bool cgb_mode;
    bool obj_priority_mode;
    bool double_speed;
    bool speed_switch_needed;
    uint8_t undocumented_registers[0x04];
    uint64_t cycle;
} gb_state_t;

typedef struct _gb_snapshot_t {
    gb_state_t gb;
    gb_cpu_state_t cpu;
    gb_ppu_state_t ppu;
    gb_apu_state_t apu;
    gb_square_state_t square1;
    gb_square_state_t square2;
    gb_wave_state_t wave;
    gb_noise_state_t noise;
    gb_joypad_state_t joypad;
    gb_interrupt_state_t interrupt;
    gb_timer_state_t timer;
    gb_dma_state_t dma;
    gb_palette_state_t palette;
    gb_serial_state_t serial;
    gb_infrared_state_t infrared;
    gb_boot_state_t boot;
    gb_memory_state_t memory;
} gb_snapshot_t;

typedef struct _gb_t {

    bool is_cgb_pending;
    float speed;
    bool cartridge_inserted;
    bool paused;
    
    gb_state_t state;
    
    gb_cpu_t cpu;
    gb_ppu_t ppu;
    gb_apu_t apu;
    gb_joypad_t joypad;
    gb_interrupt_t interrupt;
    gb_timer_t timer;
    gb_dma_t dma;
    gb_palette_t palette;
    gb_serial_t serial;
    gb_infrared_t infrared;
    gb_boot_t boot;
    gb_memory_t memory;
    gb_cartridge_t cartridge;
    
    gb_rewind_t rewind;
    gb_printer_t printer;
    gb_frame_timer_t frame_timer;
    gb_breakpoint_manager_t breakpoint_manager;
    gb_event_manager_t event_manager;
    
    gb_memory_descriptor_t key0_register_descriptor;
    gb_memory_descriptor_t key1_register_descriptor;
    gb_memory_descriptor_t opri_register_descriptor;
    gb_memory_descriptor_t undocumented_register_descriptor;
} gb_t;


#ifdef __cplusplus
extern "C" {
#endif

gb_t* gb_new();

uint32_t gb_get_clock_rate(gb_t* gb);

bool gb_insert_cartridge(gb_t* gb,const char* path);
void gb_remove_cartridge(gb_t* gb);

void gb_set_speed(gb_t* gb,float new_speed);

void gb_pause(gb_t* gb,bool paused);

void gb_connect_printer(gb_t* gb,gb_printer_callback_t callback,void* userdata);
void gb_disconnect_printer(gb_t* gb);

void gb_half_machine_cycle(gb_t* gb);
void gb_machine_cycle(gb_t* gb);

void gb_execute_frame(gb_t* gb);

void gb_execute_step(gb_t* gb);

void gb_switch_speed(gb_t* gb);

void gb_write_key0_register(void* data,uint8_t value,uint16_t address);

void gb_write_key1_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_read_key1_register(void* data,uint16_t address);

void gb_write_opri_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_read_opri_register(void* data,uint16_t address);

void gb_write_undocumented_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_read_undocumented_register(void* data,uint16_t address);

void gb_map(gb_t* gb);
void gb_update_mapping(gb_t* gb);

void gb_reset(gb_t* gb);

void gb_save_state(gb_t* gb,gb_snapshot_t* snapshot);
void gb_load_state(gb_t* gb,gb_snapshot_t* snapshot);

void gb_save_snapshot(gb_t* gb,gb_snapshot_t* snapshot);
void gb_load_snapshot(gb_t* gb,gb_snapshot_t* snapshot);

void gb_delete(gb_t* gb);

#ifdef __cplusplus
}
#endif