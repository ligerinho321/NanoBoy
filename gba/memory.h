#pragma once

#include "utils.h"

typedef enum _gba_memory_access_t {
    gba_nonsequential_access = 0x00,
    gba_sequential_access    = 0x01,

    gba_memory_byte_access          = 0x00,
    gba_memory_halfword_access      = 0x01,
    gba_memory_word_access          = 0x03,
} gba_memory_access_t;

typedef struct _gba_waitstate_control_t {
    uint8_t ws_sram : 2;
    
    uint8_t ws0_first_access : 2;
    uint8_t ws0_second_access : 1;
    
    uint8_t ws1_first_access : 2;
    uint8_t ws1_second_access : 1;

    uint8_t ws2_first_access : 2;
    uint8_t ws2_second_access : 1;

    uint8_t phi : 2;

    bool prefetch_enabled : 1;
} gba_waitstate_control_t;

typedef struct _gba_memory_control_t {
    bool wram_disabled : 1;
    uint8_t unknown0 : 2;
    bool cgb_bootrom_disabled : 1;
    bool ewram_enabled : 1;
    uint8_t wram_waitstate : 4;
    uint8_t unknown1 : 4;
} gba_memory_control_t;

typedef struct _gba_memory_t {
    gba_t* gba;

    uint8_t ewram[gba_ewram_length];
    uint8_t iwram[gba_iwram_length];

    gba_waitstate_control_t waitstate_control;
    uint8_t waitstate_table[8][2];

    gba_memory_control_t control;

    uint16_t prefetch_buffer[0x08];

    gba_word_t bus;
} gba_memory_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_memory_init(gba_memory_t* memory,gba_t* gba);

void gba_memory_write(gba_memory_t* memory,uint32_t value,uint32_t address,uint8_t size,uint8_t access);
uint32_t gba_memory_read(gba_memory_t* memory,uint32_t address,uint8_t size,uint8_t access);

gba_word_t gba_memory_read_unused(gba_memory_t* memory,uint32_t address,uint8_t size);

void gba_memory_update_waitstate_table(gba_memory_t* memory);

void gba_memory_write_register(gba_memory_t* memory,gba_word_t value,uint32_t address,uint8_t size);
gba_word_t gba_memory_read_register(gba_memory_t* memory,uint32_t address,uint8_t size);

void gba_memory_write_ewram(gba_memory_t* memory,gba_word_t value,uint32_t address,uint8_t size);
gba_word_t gba_memory_read_ewram(gba_memory_t* memory,uint32_t address,uint8_t size);

void gba_memory_write_iwram(gba_memory_t* memory,gba_word_t value,uint32_t address,uint8_t size);
gba_word_t gba_memory_read_iwram(gba_memory_t* memory,uint32_t address,uint8_t size);

void gba_memory_reset(gba_memory_t* memory);

#ifdef __cplusplus
}
#endif