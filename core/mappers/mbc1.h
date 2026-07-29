#pragma once

#include "../utils/utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

typedef struct _gb_mbc1_t {
    bool is_mbc1m;
    
    bool ram_enabled;
    bool mode;
    uint8_t bank0;
    uint8_t bank1;
} gb_mbc1_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gb_mbc1_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc1_write_register_0(void* data,uint8_t value,uint16_t address);

void gb_mbc1_write_register_1(void* data,uint8_t value,uint16_t address);

void gb_mbc1_reset(gb_cartridge_t* cartridge);

void gb_mbc1_save_state(gb_cartridge_t* cartridge,gb_state_t* state);
void gb_mbc1_load_state(gb_cartridge_t* cartridge,gb_state_t* state);

#ifdef __cplusplus
}
#endif