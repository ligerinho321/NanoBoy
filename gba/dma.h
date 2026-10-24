#pragma once

#include "utils.h"

typedef struct _gba_dma_control_t {
    uint8_t dst : 2;
    uint8_t src : 2;
    bool repeat : 1;
    bool word : 1;
    bool drq : 1;
    uint8_t start_timing : 2;
    bool irq_enabled : 1;
    bool enabled;
} gba_dma_control_t;

typedef struct _gba_dma_channel_t {
    gba_word_t src_address;
    gba_word_t dst_address;
    gba_halfword_t count;
    gba_dma_control_t control;
} gba_dma_channel_t;

typedef struct _gba_dma_t {
    gba_t* gba;

    gba_dma_channel_t channel[4];
} gba_dma_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_dma_init(gba_dma_t* dma,gba_t* gba);

void gba_dma_write_register_raw(gba_dma_t* dma,uint8_t value,uint32_t address);
uint8_t gba_dma_read_register_raw(gba_dma_t* dma,uint32_t address);

void gba_dma_reset(gba_dma_t* dma);

#ifdef __cplusplus
}
#endif