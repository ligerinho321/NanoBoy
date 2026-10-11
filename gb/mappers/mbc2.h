#pragma once

#include "../utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

enum{
    gb_mbc2_ram_length = 0x200,
    gb_mbc2_ram_bank_mask = 0x00,
    gb_mbc2_ram_address_mask = 0x1FF
};

typedef struct _gb_mbc2_t {
    bool ram_enabled;
    uint8_t rom_bank;
} gb_mbc2_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gb_mbc2_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc2_write_register(void* data,uint8_t value,uint16_t address);

void gb_mbc2_write_ram(void* data,uint8_t value,uint16_t address);
uint8_t gb_mbc2_read_ram(void* data,uint16_t address);

size_t gb_mbc2_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);
size_t gb_mbc2_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);

void gb_mbc2_reset(gb_cartridge_t* cartridge);

void gb_mbc2_save_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot);
void gb_mbc2_load_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif