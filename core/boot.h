#pragma once

#include "utils.h"

enum{
    gb_boot_dmg_rom_size = 256,
    gb_boot_cgb_rom_size = 2048 + 256
};

typedef struct _gb_boot_state_t {
    bool mapped;
} gb_boot_state_t;

typedef struct _gb_boot_t {
    gb_t* gb;

    gb_boot_state_t state;
    
    const char* dmg_rom_path;
    bool cgb_rom_inserted;
    uint8_t cgb_rom[gb_boot_cgb_rom_size];
    uint32_t cgb_rom_crc32;

    const char* cgb_rom_path;
    bool dmg_rom_inserted;
    uint8_t dmg_rom[gb_boot_dmg_rom_size];
    uint32_t dmg_rom_crc32;

    bool skip_enabled;

    gb_memory_descriptor_t rom_descriptor;
    gb_memory_descriptor_t bank_register_descriptor;
} gb_boot_t;


#define gb_set_dmg_rom_path_reference(gb,path)\
    (gb)->boot.dmg_rom_path = path

#define gb_remove_dmg_rom_path_reference(gb)\
    (gb)->boot.dmg_rom_path = NULL

#define gb_set_cgb_rom_path_reference(gb,path)\
    (gb)->boot.cgb_rom_path = path

#define gb_remove_cgb_rom_path_reference(gb)\
    (gb)->boot.dmg_rom_path = NULL

#define gb_enable_skip_boot(gb,enabled)\
    (gb)->boot.skip_enabled = enabled


#ifdef __cplusplus
extern "C" {
#endif

void gb_boot_init(gb_boot_t* boot,gb_t* gb);

void gb_boot_update_roms(gb_boot_t* boot);

void gb_boot_map(gb_boot_t* boot);
void gb_boot_unmap(gb_boot_t* boot);

void gb_boot_write_bank_register(void* data,uint8_t value,uint16_t address);
uint8_t gb_boot_read_bank_register(void* data,uint16_t address);

uint8_t gb_boot_dmg_read_rom(void* data,uint16_t address);
uint8_t gb_boot_cgb_read_rom(void* data,uint16_t address);

void gb_boot_save_state(gb_boot_t* boot,gb_snapshot_t* snapshot);
void gb_boot_load_state(gb_boot_t* boot,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif