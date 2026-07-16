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
#include "printer.h"

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
    
    bool multi_thread;

#ifdef _WIN32
    HANDLE thread_handle;
#else
    pthread_t thread_id;
#endif

    gb_atomic_bool_t thread_running;
    
    bool paused;
    
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
    gb_printer_t printer;
    gb_frame_timer_t frame_timer;
    
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

uint32_t gb_get_clock_rate(gb_t* gb);

bool gb_insert_cartridge(gb_t* gb,const char* path);
void gb_remove_cartridge(gb_t* gb);

void gb_thread_safe_connect_printer(gb_t* gb,gb_printer_callback_t callback,void* userdata);
void gb_thread_safe_disconnect_printer(gb_t* gb);

void gb_thread_safe_set_joypad_callback(gb_t* gb,gb_joypad_callback_t callback,void* data);
void gb_thread_safe_remove_joypad_callback(gb_t* gb);

void gb_thread_safe_set_apu_callback(gb_t* gb,gb_apu_callback_t callback,void* data);
void gb_thread_safe_remove_apu_callback(gb_t* gb);

void gb_thread_safe_add_ppu_handler(gb_t* gb,gb_ppu_handler_t* handler);
void gb_thread_safe_remove_ppu_handler(gb_t* gb,gb_ppu_handler_t* handler);

void gb_thread_safe_set_speed(gb_t* gb,float new_speed);
void gb_thread_safe_set_execution_mode(gb_t* gb,bool multi_thread);
void gb_thread_safe_set_paused(gb_t* gb,bool paused);

void gb_thread_safe_reset(gb_t *gb);

void gb_connect_printer(gb_t* gb,gb_printer_callback_t callback,void* userdata);
void gb_disconnect_printer(gb_t* gb);

void gb_half_machine_cycle(gb_t* gb);
void gb_machine_cycle(gb_t* gb);

void gb_execute_frame(gb_t* gb);

void gb_thread_stop(gb_t* gb);
void gb_thread_start(gb_t* gb);

void gb_switch_speed(gb_t* gb);

void gb_write_key0_register(void* data,uint8_t value,uint16_t address);

void gb_write_key1_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_read_key1_register(void* data,uint16_t address);

void gb_write_opri_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_read_opri_register(void* data,uint16_t address);

void gb_map_cgb_registers(gb_t* gb);
void gb_unmap_cgb_registers(gb_t* gb);

void gb_reset(gb_t* gb);

void gb_delete(gb_t* gb);

#define gb_save_ram(gb,path) gb_cartridge_save_ram(&(gb)->cartridge,path)
#define gb_load_ram(gb,path) gb_cartridge_load_ram(&(gb)->cartridge,path)

#define gb_save_rtc(gb,path) gb_cartridge_save_rtc(&(gb)->cartridge,path)
#define gb_load_rtc(gb,path) gb_cartridge_load_rtc(&(gb)->cartridge,path)

#define gb_set_apu_callback(gb,callback,data) gb_apu_set_callback(&(gb)->apu,callback,data)
#define gb_remove_apu_callback(gb) gb_apu_remove_callback(&(gb)->apu)

#define gb_add_ppu_handler(gb,handler) gb_ppu_add_handler(&(gb)->ppu,handler)
#define gb_remove_ppu_handler(gb,handler) gb_ppu_remove_handler(&(gb)->ppu,handler)

#define gb_add_cheat_code(gb,code) gb_memory_add_cheat_code(&(gb)->memory,code)
#define gb_remove_cheat_code(gb,code) gb_memory_remove_cheat_code(&(gb)->memory,code)

#define gb_set_printer_padding_enabled(gb,enabled) gb_printer_set_padding_enabled(&(gb)->printer,enabled)
#define gb_get_printer_padding_enabled(gb) gb_printer_get_padding_enabled(&(gb)->printer)

#define gb_accelerate_printer(gb) gb_printer_accelerate(&(gb)->printer)

#define gb_get_fps(gb) gb_frame_timer_get_fps(&(gb)->frame_timer)

#define gb_get_render_buffer(gb) gb_ppu_get_render_buffer(&(gb)->ppu)

#ifdef __cplusplus
}
#endif