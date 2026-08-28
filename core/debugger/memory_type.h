#pragma once

#include "../utils.h"

typedef enum gb_memory_type_t {
    gb_memory_cpu_type,
    gb_memory_rom_type,
    gb_memory_vram_type,
    gb_memory_ram_type,
    gb_memory_wram_type,
    gb_memory_oam_type,
    gb_memory_hram_type,
    gb_memory_type_count,
} gb_memory_type_t;

extern const char* gb_memory_type_names[7];

#ifdef __cplusplus
extern "C" {
#endif

uint8_t gb_memory_type(gb_t* gb,uint16_t address);

size_t gb_memory_type_length(gb_t* gb,uint8_t memory_type);

size_t gb_memory_type_absolute_address(gb_t* gb,uint8_t memory_type,uint16_t relative_address);

void gb_memory_type_write_byte(gb_t* gb,uint8_t memory_type,uint8_t value,size_t address);
uint8_t gb_memory_type_read_byte(gb_t* gb,uint8_t memory_type,size_t address);

void gb_memory_type_write(gb_t* gb,uint8_t memory_type,size_t address,uint8_t* src,size_t len);
void gb_memory_type_read(gb_t* gb,uint8_t memory_type,size_t address,uint8_t* dst,size_t len);

void gb_memory_type_import(gb_t* gb,uint8_t memory_type,const char* filename);
void gb_memory_type_export(gb_t* gb,uint8_t memory_type,const char* filename);

#ifdef __cplusplus
}
#endif