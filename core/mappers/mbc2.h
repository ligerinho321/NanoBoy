#pragma once

#include "../utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_mbc2_t {
    bool ram_enabled;
    uint8_t rom_bank;
} gb_mbc2_t;

void gb_mbc2_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc2_write_register(void* data,uint8_t value,uint16_t address);

void gb_mbc2_write_ram(void* data,uint8_t value,uint16_t address);
uint8_t gb_mbc2_read_ram(void* data,uint16_t address);

void gb_mbc2_reset(gb_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif