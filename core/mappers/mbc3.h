#pragma once

#include "../utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_cartridge_t gb_cartridge_t;

typedef struct _gb_mbc3_rtc_t {
    uint8_t reg[5];
    uint8_t latched_reg[5];
    bool latch;
} gb_mbc3_rtc_t;

typedef struct _gb_mbc3_t {
    bool has_rtc;
    bool ram_and_rtc_enabled;
    uint8_t rom_bank;
    uint8_t ram_and_rtc_bank;
    gb_mbc3_rtc_t rtc;
} gb_mbc3_t;


void gb_mbc3_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc3_update_ram_and_rtc_mapping(gb_cartridge_t* cartridge);

void gb_mbc3_write_register0(void* data,uint8_t value,uint16_t address);

void gb_mbc3_write_register1(void* data,uint8_t value,uint16_t address);

void gb_mbc3_write_rtc_register(void* data,uint8_t value,uint16_t address);

uint8_t gb_mbc3_read_rtc_register(void* data,uint16_t address);

void gb_mbc3_rtc_clock(gb_cartridge_t* cartridge);

void gb_mbc3_reset(gb_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif