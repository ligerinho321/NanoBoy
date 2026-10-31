#pragma once

#include "utils.h"

enum {
    gba_cartridge_rom_min_length    = 0x000000C0,
    gba_cartridge_rom_max_length    = 0x02000000,
    gba_cartridge_rom_address_mask  = 0x01FFFFFF,
    gba_cartridge_sram_address_mask = 0x0000FFFF
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
    size_t rom_length;

    uint8_t* sram;
    size_t sram_length;

    gba_cartridge_io_t io;
} gba_cartridge_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_cartridge_init(gba_cartridge_t* cartridge,gba_t* gba);

bool gba_cartridge_load(gba_cartridge_t* cartridge,uint8_t* rom,size_t length);
void gba_cartridge_unload(gba_cartridge_t* cartridge);

bool gba_cartridge_rom_is_valid(const uint8_t* rom,size_t length);
bool gba_cartridge_verify_header_nintendo_logo(const uint8_t* rom);
bool gba_cartridge_verify_header_checksum(const uint8_t* rom);


void gba_cartridge_write_rom_byte(gba_cartridge_t* cartridge,uint8_t value,uint32_t address,uint8_t access);
gba_word_t gba_cartridge_read_rom_byte(gba_cartridge_t* cartridge,uint32_t address,uint8_t access);

void gba_cartridge_write_rom_halfword(gba_cartridge_t* cartridge,gba_halfword_t value,uint32_t address,uint8_t access);
gba_word_t gba_cartridge_read_rom_halfword(gba_cartridge_t* cartridge,uint32_t address,uint8_t access);

void gba_cartridge_write_rom_word(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t access);
gba_word_t gba_cartridge_read_rom_word(gba_cartridge_t* cartridge,uint32_t address,uint8_t access);


void gba_cartridge_write_sram_byte(gba_cartridge_t* cartridge,uint8_t value,uint32_t address,uint8_t access);
gba_word_t gba_cartridge_read_sram_byte(gba_cartridge_t* cartridge,uint32_t address,uint8_t access);

void gba_cartridge_write_sram_halfword(gba_cartridge_t* cartridge,gba_halfword_t value,uint32_t address,uint8_t access);

void gba_cartridge_write_sram_word(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t access);


void gba_cartridge_reset(gba_cartridge_t* cartridge);

#ifdef __cplusplus
}
#endif