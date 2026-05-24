#include "mbc5.h"
#include "../gb.h"

void gb_mbc5_init(gb_cartridge_t* cartridge,uint8_t flags){
    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->rom0_handler.write = gb_mbc5_write_register0;
    cartridge->rom1_handler.write = gb_mbc5_write_register1;

    if(flags & gb_cartridge_ram){
        gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery);
    }

    cartridge->reset = gb_mbc2_reset;
}

void gb_mbc5_write_register0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    //0x3000-0x3FFF
    if(address >= 0x3000){
        cartridge->mbc5.rom_bank = ((value & 0x01) ? 0x100 : 0x00) | (cartridge->mbc5.rom_bank & 0xFF);
        gb_cartridge_set_rom1_bank(cartridge,cartridge->mbc5.rom_bank);
    }
    //0x2000-0x2FFF
    else if(address >= 0x2000){
        cartridge->mbc5.rom_bank = (cartridge->mbc5.rom_bank & 0x100) | value;
        gb_cartridge_set_rom1_bank(cartridge,cartridge->mbc5.rom_bank);
    }
    //0x0000-0x1FFF
    else if(cartridge->ram_size){
        cartridge->mbc5.ram_enabled = (value & 0x0F) == 0x0A;
        if(cartridge->mbc5.ram_enabled){
            cartridge->ram_handler.write = gb_cartridge_write_ram;
            cartridge->ram_handler.read = gb_cartridge_read_ram;
        }
        else{
            cartridge->ram_handler.write = NULL;
            cartridge->ram_handler.read = NULL;
        }
    }
}

void gb_mbc5_write_register1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    //0x4000-0x5FFF
    if(address < 0x6000 && cartridge->ram_size){
        cartridge->mbc5.ram_bank = value & 0x0F;
        gb_cartridge_set_ram_bank(cartridge,cartridge->mbc5.ram_bank);
    }
}

void gb_mbc5_reset(gb_cartridge_t* cartridge){
    cartridge->mbc5.rom_bank = 0x00;
    gb_cartridge_set_rom1_bank(cartridge,cartridge->mbc5.rom_bank);

    if(cartridge->ram_size){
        cartridge->mbc5.ram_enabled = false;
        cartridge->ram_handler.write = NULL;
        cartridge->ram_handler.read = NULL;

        cartridge->mbc5.ram_bank = 0x00;
        gb_cartridge_set_ram_bank(cartridge,cartridge->mbc5.ram_bank);
    }
}