#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_dma_t {
    gb_t* gb;

    bool oam_running;
    bool oam_setup_cycle;
    uint8_t oam_src_addr;
    uint8_t oam_index;
    uint8_t oam_byte;

    gb_memory_handler_t oam_register_handler;
    gb_memory_handler_t vram_register_handler;
} gb_dma_t;


void gb_dma_init(gb_dma_t* dma,gb_t* gb);

void gb_dma_oam_clock(gb_dma_t* dma);

void gb_dma_oam_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_dma_oam_read_register(void* data,uint16_t address);

void gb_dma_vram_write_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_dma_vram_read_register(void* data,uint16_t address);

void gb_dma_oam_map_registers(gb_dma_t* dma);

void gb_dma_vram_map_registers(gb_dma_t* dma);
void gb_dma_vram_unmap_registers(gb_dma_t* dma);

void gb_dma_reset(gb_dma_t* dma);

#ifdef __cplusplus
}
#endif