#pragma once

#include "./utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_boot_t {
    gb_t* gb;
    gb_memory_handler_t rom_handler;
    gb_memory_handler_t bank_register_handler;
    bool rom_mapped;
} gb_boot_t;

void gb_boot_init(gb_boot_t* boot,gb_t* gb);

void gb_boot_map(gb_boot_t* boot);

void gb_boot_write_bank_register(void* data,uint8_t value,uint16_t address);

uint8_t gb_boot_read_bank_register(void* data,uint16_t address);

uint8_t gb_boot_read_dmg_rom(void* data,uint16_t address);

uint8_t gb_boot_read_cgb_rom(void* data,uint16_t address);

#ifdef __cplusplus
}
#endif