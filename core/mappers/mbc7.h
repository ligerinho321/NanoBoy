#pragma once

#include "../utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

enum{
    gb_eeprom93lc56_ram_length = 0x100,
    gb_eeprom93lc56_ram_bank_mask = 0x00,
    gb_eeprom93lc56_ram_address_mask = 0xFF
};

typedef enum _gb_eeprom93lc56_state_t {
    gb_eeprom93lc56_idle_state,
    gb_eeprom93lc56_command_state,
    gb_eeprom93lc56_write_state,
    gb_eeprom93lc56_write_all_state,
    gb_eeprom93lc56_read_state,
} gb_eeprom93lc56_state_t;

typedef struct _gb_eeprom93lc56_t {
    bool di;
    bool clk;
    bool cs;

    bool write_enabled;

    uint8_t state;
    
    uint16_t command;
    uint8_t command_bits;
    
    uint16_t read_data;
    int8_t read_count;

    uint16_t write_data;
    uint8_t write_count;

    uint8_t *ram;
} gb_eeprom93lc56_t;

typedef struct _gb_mbc7_t {
    bool ram_1_enabled;
    bool ram_2_enabled;

    uint8_t rom_bank;

    bool latch;
    uint16_t latched_accel_x;
    uint16_t latched_accel_y;

    gb_eeprom93lc56_t eeprom;
} gb_mbc7_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gb_mbc7_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc7_write_register_0(void* data,uint8_t value,uint16_t address);
void gb_mbc7_write_register_1(void* data,uint8_t value,uint16_t address);

void gb_mbc7_write_register_2(void* data,uint8_t value,uint16_t address);
uint8_t gb_mbc7_read_register_2(void* data,uint16_t address);

size_t gb_mbc7_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);

void gb_mbc7_reset(gb_cartridge_t* cartridge);

void gb_mbc7_save_state(gb_cartridge_t* cartridge,gb_state_t* state);
void gb_mbc7_load_state(gb_cartridge_t* cartridge,gb_state_t* state);

void gb_eeprom93lc56_write(gb_eeprom93lc56_t* eeprom,uint8_t value);
uint8_t gb_eeprom93lc56_read(gb_eeprom93lc56_t* eeprom);

void gb_eeprom93lc56_reset(gb_eeprom93lc56_t* eeprom);

void gb_eeprom93lc56_save_state(gb_eeprom93lc56_t* eeprom,gb_state_t* state);
void gb_eeprom93lc56_load_state(gb_eeprom93lc56_t* eeprom,gb_state_t* state);

#ifdef __cplusplus
}
#endif