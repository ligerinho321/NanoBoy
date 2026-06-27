#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_memory_t {
    gb_t *gb;

    uint8_t wram[gb_wram_length];
    uint8_t* wram_bank_ptr;
    uint8_t wram_bank;
    gb_memory_handler_t wram0_handler;
    gb_memory_handler_t wram1_handler;
    gb_memory_handler_t wbk_register_handler;
        
    uint8_t hram[gb_hram_length];
    gb_memory_handler_t hram_handler;

    gb_memory_handler_t* bus[0x10000];
    gb_cheat_code_t* codes[0x10000];
} gb_memory_t;

void gb_memory_init(gb_memory_t* memory,gb_t* gb);

void gb_memory_add_cheat_code(gb_memory_t* memory,gb_cheat_code_t* code);
void gb_memory_remove_cheat_code(gb_memory_t* memory,gb_cheat_code_t* code);

void gb_memory_cpu_write(gb_memory_t* memory,uint8_t value,uint16_t address);
uint8_t gb_memory_cpu_read(gb_memory_t* memory,uint16_t address);

uint8_t gb_memory_oam_dma_read(gb_memory_t* memory,uint16_t address);

void gb_memory_vram_dma_write(gb_memory_t* memory,uint8_t value,uint16_t address);
uint8_t gb_memory_vram_dma_read(gb_memory_t* memory,uint16_t address);

void gb_memory_map(gb_memory_t* memory,gb_memory_handler_t* handler,uint32_t start,uint32_t end);

void gb_memory_write_wram0(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_wram0(void* data,uint16_t address);
void gb_memory_write_wram1(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_wram1(void* data,uint16_t address);
void gb_memory_map_wram(gb_memory_t* memory);

void gb_memory_write_hram(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_hram(void* data,uint16_t address);
void gb_memory_map_hram(gb_memory_t* memory);

void gb_memory_write_wbk_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_wbk_register(void* data,uint16_t address);

void gb_memory_reset(gb_memory_t* memory);

#ifdef __cplusplus
}
#endif