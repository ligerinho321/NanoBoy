#pragma once

#include "../utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_mbc1_t {
    bool ram_enabled;
    uint8_t bank[2];
    bool mode;
    bool is_mbc1m;
} gb_mbc1_t;

void gb_mbc1_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc1_update_mapping(gb_cartridge_t* cartridge);

void gb_mbc1_write_register0(void* data,uint8_t value,uint16_t address);

void gb_mbc1_write_register1(void* data,uint8_t value,uint16_t address);

void gb_mbc1_reset(gb_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif