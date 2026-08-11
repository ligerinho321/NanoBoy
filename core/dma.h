#pragma once

#include "utils/utils.h"

typedef enum _gb_oam_dma_state_t {
    gb_oam_dma_state_none,
    gb_oam_dma_state_delay,
    gb_oam_dma_state_setup,
    gb_oam_dma_state_transfer
} gb_oam_dma_state_t;

typedef struct _gb_dma_t {
    gb_t* gb;

    uint8_t oam_state;
    uint8_t oam_src;
    uint16_t oam_hi_addr;
    uint8_t oam_counter;
    uint8_t oam_byte;

    uint16_t vram_src;
    uint16_t vram_dst;
    uint8_t vram_length;
    bool vram_hblank_running;

    gb_memory_descriptor_t oam_register_descriptor;
    gb_memory_descriptor_t vram_register_descriptor;
} gb_dma_t;

#ifdef __cplusplus
extern "C" {
#endif

void gb_dma_init(gb_dma_t* dma,gb_t* gb);

void gb_oam_dma_clock(gb_dma_t* dma);

bool gb_oam_dma_bus_conflict(gb_dma_t* dma,uint16_t address);

void gb_oam_dma_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_oam_dma_read_register(void* data,uint16_t address);

void gb_vram_hblank_dma(gb_dma_t* dma);
void gb_vram_general_dma(gb_dma_t* dma);

void gb_vram_dma_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_vram_dma_read_register(void* data,uint16_t address);

void gb_dma_map(gb_dma_t* dma);

void gb_dma_reset(gb_dma_t* dma);
void gb_dma_skip_boot(gb_dma_t* dma);

void gb_dma_save_state(gb_dma_t* dma,gb_state_t* state);
void gb_dma_load_state(gb_dma_t* dma,gb_state_t* state);

#ifdef __cplusplus
}
#endif