#pragma once

#include "utils.h"

typedef struct _gba_bios_t {
    gba_t* gba;

    const char* path;
    
    bool inserted;
    
    uint8_t data[gba_bios_length];

    uint32_t crc32;

    gba_word_t bus;
} gba_bios_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_bios_init(gba_bios_t* bios,gba_t* gba);

void gba_bios_set_path_reference(gba_t* gba,const char* path);

void gba_bios_update(gba_bios_t* bios);

gba_word_t gba_bios_read(gba_bios_t* bios,uint32_t address,uint8_t size);

void gba_bios_reset(gba_bios_t* bios);

#ifdef __cplusplus
}
#endif