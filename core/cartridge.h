#pragma once

#include "utils.h"
#include "mappers/mbc1.h"
#include "mappers/mbc2.h"
#include "mappers/mbc3.h"
#include "mappers/mbc5.h"
#include "mappers/mbc6.h"
#include "mappers/mbc7.h"
#include "mappers/mmm01.h"
#include "mappers/huc1.h"

enum{
    gb_cartridge_rom_min_size = 0x8000,
    gb_cartridge_rom_max_size = 0x800000,

    gb_cartridge_ram_min_size = 0x2000,
    gb_cartridge_ram_max_size = 0x20000,
};

typedef enum _gb_cartridge_component_t {
    gb_cartridge_ram = 0x01,
    gb_cartridge_battery = 0x02,
    gb_cartridge_rtc = 0x04,
    gb_cartridge_rumble = 0x08,
    gb_cartridge_sensor = 0x10
} gb_cartridge_component_t;

typedef struct _gb_cartridge_t {
    gb_t* gb;

    uint8_t* header;

    uint8_t* rom;
    size_t rom_length;
    uint32_t rom_crc32;
    gb_memory_descriptor_t rom0_descriptor;
    gb_memory_descriptor_t rom1_descriptor;
    uint8_t* rom0_ptr;
    uint8_t* rom1_ptr;
    uint16_t rom_bank_mask;

    uint8_t* ram;
    size_t ram_length;
    gb_memory_descriptor_t ram_descriptor;
    uint8_t* ram_ptr;
    uint8_t ram_bank_mask;
    uint16_t ram_address_mask;
    bool ram_has_battery;

    struct{
        void* data;
        size_t (*rom_absolute_address)(struct _gb_cartridge_t*,uint16_t);
        size_t (*ram_absolute_address)(struct _gb_cartridge_t*,uint16_t);
        void (*rtc_update_timer)(struct _gb_cartridge_t*);
        void (*rtc_save)(struct _gb_cartridge_t*,const char*);
        void (*rtc_load)(struct _gb_cartridge_t*,const char*);
        void (*reset)(struct _gb_cartridge_t*);
        void (*save_state)(struct _gb_cartridge_t*,gb_state_t*);
        void (*load_state)(struct _gb_cartridge_t*,gb_state_t*);
    } mapper;
} gb_cartridge_t;


#define gb_cartridge_nintendo_logo(cartridge)\
    ((cartridge)->header + 0x04)

#define gb_cartridge_title(cartridge)\
    ((cartridge)->header + 0x34)
    
#define gb_cartridge_cgb_flag(cartridge)\
    ((cartridge)->header[0x43] & 0x80)

#define gb_cartridge_old_licensee_code(cartridge)\
    (cartridge)->header[0x4B]

#define gb_cartridge_new_licensee_code(cartridge)\
    ((cartridge)->header + 0x44)


#define gb_cartridge_set_rom0_bank(cartridge,bank)\
    (cartridge)->rom0_ptr = (cartridge)->rom + (((bank) & (cartridge)->rom_bank_mask) << 0x0E)

#define gb_cartridge_set_rom1_bank(cartridge,bank)\
    (cartridge)->rom1_ptr = (cartridge)->rom + (((bank) & (cartridge)->rom_bank_mask) << 0x0E)

#define gb_cartridge_set_ram_bank(cartridge,bank)\
    (cartridge)->ram_ptr = (cartridge)->ram + (((bank) & (cartridge)->ram_bank_mask) << 0x0D)

    
#ifdef __cplusplus
extern "C" {
#endif

void gb_cartridge_init(gb_cartridge_t* cartridge,gb_t* gb);

bool gb_cartridge_load(gb_cartridge_t* cartridge,const char* path);
void gb_cartridge_remove(gb_cartridge_t* cartridge);

void gb_cartridge_save_ram(gb_t* gb,const char* path);
void gb_cartridge_load_ram(gb_t* gb,const char* path);

bool gb_cartridge_verify_nintendo_logo(uint8_t* header);
bool gb_cartridge_verify_header_checksum(uint8_t* header);

size_t gb_cartridge_get_rom_length(uint8_t* header);

bool gb_cartridge_init_ram(gb_cartridge_t* cartridge,bool battery);
bool gb_cartridge_init_mapper(gb_cartridge_t* cartridge);

bool gb_no_mbc_init(gb_cartridge_t* cartridge,uint8_t flags);

uint8_t gb_cartridge_read_rom0(void* data,uint16_t address);
uint8_t gb_cartridge_read_rom1(void* data,uint16_t address);

void gb_cartridge_write_ram(void* data,uint8_t value,uint16_t address);
uint8_t gb_cartridge_read_ram(void* data,uint16_t address);

void gb_cartridge_map(gb_cartridge_t* cartridge);

size_t gb_cartridge_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);
size_t gb_cartridge_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address);

void gb_cartridge_update_rtc_timer(gb_cartridge_t* cartridge);

void gb_cartridge_save_rtc(gb_t* gb,const char* path);
void gb_cartridge_load_rtc(gb_t* gb,const char* path);

void gb_cartridge_reset(gb_cartridge_t* cartridge);

void gb_cartridge_save_state(gb_cartridge_t* cartridge,gb_state_t* state);
void gb_cartridge_load_state(gb_cartridge_t* cartridge,gb_state_t* state);

#ifdef __cplusplus
}
#endif