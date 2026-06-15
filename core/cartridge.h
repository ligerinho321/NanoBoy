#pragma once

#include "utils.h"
#include "mappers/mbc1.h"
#include "mappers/mbc2.h"
#include "mappers/mbc3.h"
#include "mappers/mbc5.h"

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
    gb_cartridge_rtc = 0x04,
    gb_cartridge_rumble = 0x08,
    gb_cartridge_sensor = 0x10
} gb_cartridge_component_t;

typedef struct _gb_cartridge_t {
    gb_t* gb;

    uint8_t rom[GB_CARTRIDGE_ROM_MAX_SIZE];
    size_t rom_size;
    gb_memory_handler_t rom0_handler;
    gb_memory_handler_t rom1_handler;
    uint8_t* rom0_ptr;
    uint8_t* rom1_ptr;
    uint16_t rom_bank_mask;

    uint8_t ram[GB_CARTRIDGE_RAM_MAX_SIZE];
    size_t ram_size;
    gb_memory_handler_t ram_handler;
    uint8_t* ram_ptr;
    uint8_t ram_bank_mask;
    uint16_t ram_address_mask;
    bool ram_has_battery;

    void (*reset)(struct _gb_cartridge_t*);

    union{
        gb_mbc1_t mbc1;
        gb_mbc2_t mbc2;
        gb_mbc3_t mbc3;
        gb_mbc5_t mbc5;
    };
} gb_cartridge_t;

void gb_cartridge_init(gb_cartridge_t* cartridge,gb_t* gb);

bool gb_cartridge_load(gb_cartridge_t* cartridge,const char* path);

bool gb_cartridge_verify_header_checksum(gb_cartridge_t* cartridge);

bool gb_cartridge_verify_global_checksum(gb_cartridge_t* cartridge);

void gb_cartridge_init_mapper(gb_cartridge_t* cartridge);

void gb_no_mbc_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_cartridge_load_rom_size(gb_cartridge_t* cartridge);

void gb_cartridge_init_ram(gb_cartridge_t* cartridge,bool battery);

uint8_t gb_cartridge_read_rom0(void* data,uint16_t address);
uint8_t gb_cartridge_read_rom1(void* data,uint16_t address);

void gb_cartridge_write_ram(void* data,uint8_t value,uint16_t address);
uint8_t gb_cartridge_read_ram(void* data,uint16_t address);

void gb_cartridge_clear(gb_cartridge_t* cartridge);


inline void gb_cartridge_set_rom0_bank(gb_cartridge_t* cartridge,uint16_t bank){
    cartridge->rom0_ptr = cartridge->rom + ((bank & cartridge->rom_bank_mask) << 0x0E);
}

inline void gb_cartridge_set_rom1_bank(gb_cartridge_t* cartridge,uint16_t bank){
    cartridge->rom1_ptr = cartridge->rom + ((bank & cartridge->rom_bank_mask) << 0x0E);
}

inline void gb_cartridge_set_ram_bank(gb_cartridge_t* cartridge,uint8_t bank){
    cartridge->ram_ptr = cartridge->ram + ((bank & cartridge->ram_bank_mask) << 0x0D);
}

#ifdef __cplusplus
}
#endif