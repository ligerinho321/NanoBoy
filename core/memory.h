#pragma once

#include "utils/utils.h"

enum{
    gb_wram_length = 0x8000,
    gb_hram_length = 0x7F,
};

typedef struct _gb_memory_t {
    gb_t *gb;

    uint8_t wram[gb_wram_length];
    uint8_t* wram_bank_ptr;
    uint8_t wram_bank;
    gb_memory_descriptor_t wram0_descriptor;
    gb_memory_descriptor_t wram1_descriptor;
    gb_memory_descriptor_t wbk_register_descriptor;
        
    uint8_t hram[gb_hram_length];
    gb_memory_descriptor_t hram_descriptor;

    gb_memory_descriptor_t empty_descriptor;

    gb_memory_descriptor_t* bus[0x10000];
    gb_cheat_code_t* codes[0x10000];
} gb_memory_t;

#define gb_memory_map(m,h,a)\
    (m)->bus[a] = h;

#define gb_memory_unmap(m,a)\
    gb_memory_map(m,&(m)->empty_descriptor,a)

#define gb_memory_map_in_range(m,h,s,e){\
    gb_memory_descriptor_t** _s = (m)->bus + (s);\
    gb_memory_descriptor_t** _e = (m)->bus + (e);\
    gb_memory_descriptor_t* _h = h;\
    while(_s <= _e) *_s++ = _h;\
}

#define gb_memory_unmap_in_range(m,s,e)\
    gb_memory_map_in_range(m,&(m)->empty_descriptor,s,e)


#ifdef __cplusplus
extern "C" {
#endif

void gb_memory_init(gb_memory_t* memory,gb_t* gb);

void gb_memory_write_empty(void* data,uint8_t value,uint16_t address);
uint8_t gb_memory_read_empty(void* data,uint16_t address);

void gb_memory_add_cheat_code(gb_memory_t* memory,gb_cheat_code_t* code);
void gb_memory_remove_cheat_code(gb_memory_t* memory,gb_cheat_code_t* code);

void gb_memory_cpu_write(gb_memory_t* memory,uint8_t value,uint16_t address);
uint8_t gb_memory_cpu_read(gb_memory_t* memory,uint16_t address);

uint8_t gb_memory_oam_dma_read(gb_memory_t* memory,uint16_t address);

void gb_memory_vram_dma_write(gb_memory_t* memory,uint8_t value,uint16_t address);
uint8_t gb_memory_vram_dma_read(gb_memory_t* memory,uint16_t address);

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
void gb_memory_skip_boot(gb_memory_t* memory);

void gb_memory_save_state(gb_memory_t* memory,gb_state_t* state);
void gb_memory_load_state(gb_memory_t* memory,gb_state_t* state);

#ifdef __cplusplus
}
#endif