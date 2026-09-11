#pragma once

#include "../utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

enum{
    gb_mbc6_flash_data_size = 0x100000,

    gb_mbc6_flash_bank_mask = (gb_mbc6_flash_data_size / 0x2000) - 0x01,
    
    gb_mbc6_flash_map_data_size = 0x100,

    gb_mbc6_flash_buffer_size = 0x80
};

typedef enum _gb_mbc6_flash_command_t {
    gb_mbc6_flash_reset_command = 0xF0,
    gb_mbc6_flash_read_id_command = 0x90,
    gb_mbc6_flash_program_flash_command = 0xA0,
    gb_mbc6_flash_erase_map_command = 0x04,
    gb_mbc6_flash_protect_sector_0_command = 0x20,
    gb_mbc6_flash_unprotect_sector_0_command = 0x40,
    gb_mbc6_flash_program_map_command = 0xE0,
    gb_mbc6_flash_read_map_command = 0x77,
    gb_mbc6_flash_mass_erase_flash_command = 0x10,
    gb_mbc6_flash_erase_flash_sector_command = 0x30
} gb_mbc6_flash_command_t;

typedef struct _gb_mbc6_state_t {
    bool ram_enabled;
    
    bool flash_enabled;
    bool flash_write_enabled;
    
    bool flash_bank_0_enabled;
    bool flash_bank_1_enabled;

    bool flash_protect_sector_0;

    uint8_t flash_state;
    uint8_t flash_pre_command;
    uint8_t flash_command;

    uint8_t rom_or_flash_bank_0;
    uint8_t rom_or_flash_bank_1;

    uint8_t ram_bank_0;
    uint8_t ram_bank_1;

    uint8_t flash_data[gb_mbc6_flash_data_size];
    
    uint8_t flash_map_data[gb_mbc6_flash_map_data_size];

    uint8_t flash_buffer[gb_mbc6_flash_buffer_size];
    uint8_t flash_buffer_last_write_address;
} gb_mbc6_state_t;

typedef struct _gb_mbc6_t {
    uint8_t* rom_or_flash_0_ptr;
    uint8_t* rom_or_flash_1_ptr;

    uint8_t* ram_0_ptr;
    uint8_t* ram_1_ptr;
    
    uint16_t rom_bank_mask;
    uint8_t ram_bank_mask;
    uint16_t ram_address_mask;

    gb_mbc6_state_t state;
} gb_mbc6_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gb_mbc6_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc6_write_register(void* data,uint8_t value,uint16_t address);

void gb_mbc6_write_flash(void* data,uint8_t value,uint16_t address);
uint8_t gb_mbc6_read_rom_or_flash(void* data,uint16_t address);

void gb_mbc6_write_ram(void* data,uint8_t value,uint16_t address);
uint8_t gb_mbc6_read_ram(void* data,uint16_t address);

size_t gb_mbc6_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);
size_t gb_mbc6_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);

void gb_mbc6_reset(gb_cartridge_t* cartridge);

void gb_mbc6_save_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot);
void gb_mbc6_load_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif