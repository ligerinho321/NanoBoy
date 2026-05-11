#pragma once

#include "./utils.h"

typedef struct _gb_mbc1_t {
    bool ram_enabled;
    uint8_t bank[2];
    bool mode;
} gb_mbc1_t;


void gb_mbc1_init(){

}

void gb_mbc1_write_enable_ram_register(void* data,uint8_t value,uint16_t address){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)data;
    mbc1->ram_enabled = ((value & 0x0F) == 0x0A) ? true : false;
}

void gb_mbc1_write_bank0_register(void* data,uint8_t value,uint16_t address){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)data;
    mbc1->bank[0] = value & 0x1F;
}

void gb_mbc1_write_bank1_register(void* data,uint8_t value,uint16_t address){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)data;
    mbc1->bank[1] = value & 0x03;
}

void gb_mbc1_write_mode_register(void* data,uint8_t value,uint16_t address){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)data;
    mbc1->mode = value & 0x01;
}
