#pragma once

#include "./utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GB_CARTRIDGE_ROM_MIN_SIZE 0x8000
#define GB_CARTRIDGE_ROM_MAX_SIZE 0x800000

#define GB_CARTRIDGE_RAM_MIN_SIZE 0x2000
#define GB_CARTRIDGE_RAM_MAX_SIZE 0x20000

typedef enum _gb_cartridge_component_t {
    gb_cartridge_ram = 0x01,
    gb_cartridge_battery = 0x02,
    gb_cartridge_timer = 0x04,
    gb_cartridge_rumble = 0x08,
    gb_cartridge_sensor = 0x10
} gb_cartridge_component_t;

typedef struct _gb_cartridge_t {
    uint8_t rom[GB_CARTRIDGE_ROM_MAX_SIZE];
    size_t rom_size;

    uint8_t ram[GB_CARTRIDGE_RAM_MAX_SIZE];
    size_t ram_size;
} gb_cartridge_t;

bool gb_cartridge_load(gb_cartridge_t* cartridge,const char* path);

bool gb_cartridge_verify_header_checksum(gb_cartridge_t* cartridge);

bool gb_cartridge_verify_global_checksum(gb_cartridge_t* cartridge);

uint8_t gb_cartridge_mapper(gb_cartridge_t* cartridge);

size_t gb_cartridge_rom_size(gb_cartridge_t* cartridge);

size_t gb_cartridge_ram_size(gb_cartridge_t* cartridge);

void gb_cartridge_clear(gb_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif