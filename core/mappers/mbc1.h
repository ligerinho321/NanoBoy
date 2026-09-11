#pragma once

#include "../utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

typedef struct _gb_mbc1_t {
    bool is_mbc1m;
    
    bool ram_enabled;
    bool mode;
    uint8_t bank_0;
    uint8_t bank_1;

    uint16_t rom_bank_0;
    uint16_t rom_bank_1;
    uint8_t ram_bank;
} gb_mbc1_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gb_mbc1_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc1_write_register_0(void* data,uint8_t value,uint16_t address);

void gb_mbc1_write_register_1(void* data,uint8_t value,uint16_t address);

size_t gb_mbc1_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);
size_t gb_mbc1_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);

void gb_mbc1_reset(gb_cartridge_t* cartridge);

void gb_mbc1_save_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot);
void gb_mbc1_load_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif