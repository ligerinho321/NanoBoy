#pragma once

#include "../utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

typedef struct _gb_mmm01_t {
    bool ram_enabled;
    
    bool mapping_enabled;
    
    bool mbc1_mode_locked;
    bool mbc1_mode_select;

    bool multiplex_enabled;

    uint8_t ram_bank_mask;
    uint8_t rom_bank_mask;
    
    uint8_t rom_bank_low;
    uint8_t rom_bank_mid;
    uint8_t rom_bank_high;

    uint8_t ram_bank_low;
    uint8_t ram_bank_high;

    uint16_t rom_bank_0;
    uint16_t rom_bank_1;
    uint8_t ram_bank;
} gb_mmm01_t;


#ifdef __cplusplus
extern "C" {
#endif

bool gb_mmm01_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mmm01_write_register_0(void* data,uint8_t value,uint16_t address);
void gb_mmm01_write_register_1(void* data,uint8_t value,uint16_t address);

size_t gb_mmm01_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);
size_t gb_mmm01_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);

void gb_mmm01_reset(gb_cartridge_t* cartridge);

void gb_mmm01_save_state(gb_cartridge_t* cartridge,gb_state_t* state);
void gb_mmm01_load_state(gb_cartridge_t* cartridge,gb_state_t* state);

#ifdef __cplusplus
}
#endif