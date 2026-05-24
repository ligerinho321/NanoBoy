#pragma once

#include "../utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_cartridge_t gb_cartridge_t;

typedef struct _gb_mbc5_t {
    bool ram_enabled;
    uint16_t rom_bank;
    uint8_t ram_bank;
} gb_mbc5_t;

void gb_mbc5_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc5_write_register0(void* data,uint8_t value,uint16_t address);

void gb_mbc5_write_register1(void* data,uint8_t value,uint16_t address);

void gb_mbc5_reset(gb_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif