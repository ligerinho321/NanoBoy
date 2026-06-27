#pragma once

#include "utils.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "joypad.h"
#include "interrupt.h"
#include "timer.h"
#include "dma.h"
#include "palette.h"
#include "serial.h"
#include "boot.h"
#include "memory.h"
#include "cartridge.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _gb_type_t {
    gb_dmg = 0x00,
    gb_cgb = 0x01
} gb_type_t;

typedef struct _gb_t {
    gb_type_t type;
    gb_type_t type_pending;
    float speed;
    bool cartridge_inserted;
    
    gb_cpu_t cpu;
    gb_ppu_t ppu;
    gb_apu_t apu;
    gb_joypad_t joypad;
    gb_interrupt_t interrupt;
    gb_timer_t timer;
    gb_dma_t dma;
    gb_palette_t palette;
    gb_serial_t serial;
    gb_boot_t boot;
    gb_memory_t memory;
    gb_cartridge_t cartridge;
    
    bool cgb_mode;
    bool double_speed;
    bool speed_switch_needed;
    bool obj_priority_mode;

    uint64_t cycle;

    gb_memory_handler_t key0_register_handler;
    gb_memory_handler_t key1_register_handler;
    gb_memory_handler_t opri_register_handler;
} gb_t;

gb_t* gb_new();

bool gb_insert_cartridge(gb_t* gb,const char* path);
void gb_remove_cartridge(gb_t* gb);

void gb_set_joypad_callback(gb_t* gb,gb_joypad_callback_t callback,void* data);

void gb_set_speed(gb_t* gb,float new_speed);

void gb_half_machine_cycle(gb_t* gb);
void gb_machine_cycle(gb_t* gb);

void gb_write_key0_register(void* data,uint8_t value,uint16_t address);

void gb_write_key1_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_read_key1_register(void* data,uint16_t address);

void gb_write_opri_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_read_opri_register(void* data,uint16_t address);

void gb_map_cgb_registers(gb_t* gb);
void gb_unmap_cgb_registers(gb_t* gb);

void gb_reset(gb_t* gb);

void gb_delete(gb_t* gb);


inline void gb_save_ram(gb_t* gb,const char* path){
    gb_cartridge_save_ram(&gb->cartridge,path);
}

inline void gb_load_ram(gb_t* gb,const char* path){
    gb_cartridge_load_ram(&gb->cartridge,path);
}


inline void gb_set_apu_callback(gb_t* gb,gb_apu_callback_t callback,void* data){
    gb_apu_set_callback(&gb->apu,callback,data);
}

inline void gb_remove_apu_callback(gb_t* gb){
    gb_apu_remove_callback(&gb->apu);
}


inline void gb_add_ppu_handler(gb_t* gb,gb_ppu_handler_t* handler){
    gb_ppu_add_handler(&gb->ppu,handler);
}

inline void gb_remove_ppu_handler(gb_t* gb,gb_ppu_handler_t* handler){
    gb_ppu_remove_handler(&gb->ppu,handler);
}


inline void gb_add_cheat_code(gb_t* gb,gb_cheat_code_t* code){
    gb_memory_add_cheat_code(&gb->memory,code);
}

inline void gb_remove_cheat_code(gb_t* gb,gb_cheat_code_t* code){
    gb_memory_remove_cheat_code(&gb->memory,code);
}


#ifdef __cplusplus
}
#endif