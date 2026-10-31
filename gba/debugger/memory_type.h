#pragma once

#include "../utils.h"

typedef enum _gba_memory_type_t {
    gba_memory_cpu_type,
    gba_memory_bios_type,
    gba_memory_ewram_type,
    gba_memory_iwram_type,
    gba_memory_cram_type,
    gba_memory_vram_type,
    gba_memory_oam_type,
    gba_memory_rom_type,
    gba_memory_sram_type,
    gba_memory_type_count,
} gba_memory_type_t;

extern const char* gba_memory_type_names[gba_memory_type_count];

#ifdef __cplusplus
extern "C" {
#endif

uint8_t gba_memory_type(uint32_t address);

size_t gba_memory_type_length(gba_t* gba,uint8_t memory_type);

size_t gba_memory_type_absolute_address(gba_t* gba,uint8_t memory_type,uint32_t relative_address);

void gba_memory_type_write_byte(gba_t* gba,uint8_t memory_type,uint8_t value,size_t address);
uint8_t gba_memory_type_read_byte(gba_t* gba,uint8_t memory_type,size_t address);

void gba_memory_type_write(gba_t* gba,uint8_t memory_type,size_t adderess,uint8_t* src,size_t length);
void gba_memory_type_read(gba_t* gba,uint8_t memory_type,size_t address,uint8_t* dst,size_t length);

void gba_memory_type_import(gba_t* gba,uint8_t memory_type,const char* path);
void gba_memory_type_export(gba_t* gba,uint8_t memory_type,const char* path);

#ifdef __cplusplus
}
#endif