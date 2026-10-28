#pragma once

#include "utils.h"

typedef struct _gba_boot_t {
    gba_t* gba;

    const char* path;
    
    bool inserted;
    
    uint8_t data[gba_boot_length];

    uint32_t crc32;

    bool skip_enabled;

    gba_word_t bus;
} gba_boot_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_boot_init(gba_boot_t* bios,gba_t* gba);

void gba_boot_set_path_reference(gba_t* gba,const char* path);
void gba_boot_remove_path_reference(gba_t* gba);

void gba_boot_enable_skip(gba_t* gba,bool enabled);

void gba_boot_update(gba_boot_t* boot);

gba_word_t gba_boot_read(gba_boot_t* boot,uint32_t address,uint8_t size);

void gba_boot_reset(gba_boot_t* boot);

#ifdef __cplusplus
}
#endif