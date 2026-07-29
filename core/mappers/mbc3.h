#pragma once

#include "../utils/utils.h"

typedef struct _gb_cartridge_t gb_cartridge_t;

#define gb_mbc3_rtc_clock_rate 32768

typedef struct _gb_mbc3_rtc_t {
    uint8_t reg[5];
    uint8_t latched_reg[5];
    bool latch;
    uint32_t cycles;
    uint64_t last_update_cycle;
} gb_mbc3_rtc_t;

typedef struct _gb_mbc3_t {
    bool has_rtc;

    uint8_t rom_bank;
    
    bool ram_or_rtc_enabled;
    uint8_t ram_or_rtc_bank;

    gb_mbc3_rtc_t rtc;
} gb_mbc3_t;

#ifdef __cplusplus
extern "C" {
#endif

bool gb_mbc3_init(gb_cartridge_t* cartridge,uint8_t flags);

void gb_mbc3_update_ram_or_rtc_mapping(gb_cartridge_t* cartridge);

void gb_mbc3_write_register_0(void* data,uint8_t value,uint16_t address);
void gb_mbc3_write_register_1(void* data,uint8_t value,uint16_t address);

void gb_mbc3_rtc_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_mbc3_rtc_read_register(void* data,uint16_t address);

void gb_mbc3_rtc_update_timer(gb_cartridge_t* cartridge);
void gb_mbc3_rtc_save(gb_cartridge_t* cartridge,const char* path);
void gb_mbc3_rtc_load(gb_cartridge_t* cartridge,const char* path);

void gb_mbc3_reset(gb_cartridge_t* cartridge);

void gb_mbc3_save_state(gb_cartridge_t* cartridge,gb_state_t* state);
void gb_mbc3_load_state(gb_cartridge_t* cartridge,gb_state_t* state);

#ifdef __cplusplus
}
#endif