#pragma once

#include "../utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

typedef struct _gb_mbc5_t {
    uint16_t rom_bank;
    bool ram_enabled;
    uint8_t ram_bank;
} gb_mbc5_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gb_mbc5_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc5_write_register_0(void* data,uint8_t value,uint16_t address);
void gb_mbc5_write_register_1(void* data,uint8_t value,uint16_t address);

size_t gb_mbc5_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);
size_t gb_mbc5_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);

void gb_mbc5_reset(gb_cartridge_t* cartridge);

void gb_mbc5_save_state(gb_cartridge_t* cartridge,gb_state_t* state);
void gb_mbc5_load_state(gb_cartridge_t* cartridge,gb_state_t* state);

#ifdef __cplusplus
}
#endif