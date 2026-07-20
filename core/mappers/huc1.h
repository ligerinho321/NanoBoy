#pragma once

#include "../utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_cartridge_t gb_cartridge_t;

typedef struct _gb_huc1_t {
    bool ir_enabled : 1;
    uint8_t rom_bank : 6;
    uint8_t ram_bank : 2;
} gb_huc1_t;

void gb_huc1_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_huc1_write_register0(void* data,uint8_t value,uint16_t address);
void gb_huc1_write_register1(void* data,uint8_t value,uint16_t address);

void gb_huc1_write_ir_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_huc1_read_ir_register(void* data,uint16_t address);

void gb_huc1_reset(gb_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif