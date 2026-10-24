#pragma once

#include "utils.h"

enum {
    gba_cartridge_rom_max_size = 0x2000000,
    gba_cartridge_rom_address_mask = 0x1FFFFFF
};

typedef enum _gba_cartridge_backup_t {
    gba_cartridge_none_backup,
    gba_cartridge_eeprom_backup,
    gba_cartridge_sram_backup,
    gba_cartridge_flash_backup,
    gba_cartridge_flash512_backup,
    gba_cartridge_flash1m_backup
} gba_cartridge_backup_t;

typedef struct _gba_cartridge_io_t {
    uint8_t data : 4;
    uint8_t direction : 4;
    bool read_enabled : 1;
} gba_cartridge_io_t;

typedef struct _gba_cartridge_t {
    gba_t* gba;

    uint8_t* rom;
    size_t rom_size;

    uint8_t* sram;
    size_t sram_size;

    gba_cartridge_io_t io;
} gba_cartridge_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_cartridge_init(gba_cartridge_t* cartridge,gba_t* gba);

bool gba_cartridge_load(gba_cartridge_t* cartridge,const char* path);
void gba_cartridge_unload(gba_cartridge_t* cartridge);

void gba_cartridge_write_rom(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t size,bool sequential);
gba_word_t gba_cartridge_read_rom(gba_cartridge_t* cartridge,uint32_t address,uint8_t size,bool sequential);

void gba_cartridge_write_sram(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t size,bool sequential);
gba_word_t gba_cartridge_read_sram(gba_cartridge_t* cartridge,uint32_t address,uint8_t size,bool sequential);

void gba_cartridge_reset(gba_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif