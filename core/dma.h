#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gb_dma_t {
    gb_t* gb;

    uint8_t oam_state;
    bool oam_running;
    uint8_t oam_src;
    uint16_t oam_hi_addr;
    uint8_t oam_counter;
    uint8_t oam_byte;

    gb_memory_handler_t oam_register_handler;
    gb_memory_handler_t vram_register_handler;
} gb_dma_t;


void gb_dma_init(gb_dma_t* dma,gb_t* gb);

void gb_dma_oam_clock(gb_dma_t* dma);

bool gb_dma_oam_bus_conflict(gb_dma_t* dma,uint16_t address);

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