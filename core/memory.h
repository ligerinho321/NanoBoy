#pragma once

#include "./utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_memory_t {
    gb_t *gb;

    gb_memory_handler_t boot_handler;
    gb_memory_handler_t bank_register_handler;

    gb_memory_handler_t rom0_handler;
    gb_memory_handler_t rom1_handler;

    gb_memory_handler_t ram_handler;

    uint8_t vram[2][0x2000];
    uint8_t vram_bank;
    gb_memory_handler_t vram_handler;
    gb_memory_handler_t vbk_register_handler;

    uint8_t wram[8][0x1000];
    uint8_t wram_bank;
    gb_memory_handler_t wram0_handler;
    gb_memory_handler_t wram1_handler;
    gb_memory_handler_t wbk_register_handler;
    
    uint8_t oam[0xA0];
    gb_memory_handler_t oam_handler;
    
    uint8_t hram[0x7F];
    gb_memory_handler_t hram_handler;

    gb_memory_handler_t* bus[0x10000];
} gb_memory_t;

#define gb_memory_map(memory,handler,start,end) for(uint16_t i = start; i <= end; ++i) (memory)->bus[i] = &(handler)

void gb_memory_init(gb_memory_t* memory,gb_t* gb);

void gb_memory_write(gb_memory_t* memory,uint8_t value,uint16_t address);
uint8_t gb_memory_read(gb_memory_t* memory,uint16_t address);

void gb_memory_write_vram(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_vram(void* data,uint16_t address);
void gb_memory_map_vram(gb_memory_t* memory);

void gb_memory_write_wram0(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_wram0(void* data,uint16_t address);
void gb_memory_write_wram1(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_wram1(void* data,uint16_t address);
void gb_memory_map_wram(gb_memory_t* memory);

void gb_memory_write_oam(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_oam(void* data,uint16_t address);
void gb_memory_map_oam(gb_memory_t* memory_manager);

void gb_memory_write_hram(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_hram(void* data,uint16_t address);
void gb_memory_map_hram(gb_memory_t* memory);


void gb_memory_write_bank_register(void* data,uint8_t value,uint16_t address);

void gb_memory_write_vbk_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_vbk_register(void* data,uint16_t address);

void gb_memory_write_wbk_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_wbk_register(void* data,uint16_t address);

#ifdef __cplusplus
}
#endif