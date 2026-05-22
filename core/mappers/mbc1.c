#include "mbc1.h"
#include "../gb.h"

void gb_mbc1_init(gb_cartridge_t* cartridge,uint8_t flags){
    cartridge->rom0_handler.write = gb_mbc1_write_register0;
    cartridge->rom1_handler.write = gb_mbc1_write_register1;

    if(flags & gb_cartridge_ram){
        gb_cartridge_init_ram(cartridge,&cartridge->mbc1.ram_enabled,flags & gb_cartridge_battery);
    }

    //mortal kombat 1 and 2 room layout
    //menu    0x00000-0x3FFFF
    //rom1    0x40000-0x7FFFF
    //rom2    0x80000-0xBFFFF
    //padding 0xC0000-0xFFFFF
    cartridge->mbc1.is_mbc1m = cartridge->rom_size > 0x40133 && !memcmp(cartridge->rom + 0x00104,cartridge->rom + 0x40104,0x30);
}

void gb_mbc1_update_mapping(gb_cartridge_t* cartridge){
    if(cartridge->mbc1.mode){
        gb_cartridge_set_rom0_bank(cartridge,cartridge->mbc1.bank[1] << (cartridge->mbc1.is_mbc1m ? 0x04 : 0x05));
        if(cartridge->ram_size > 0){
            gb_cartridge_set_ram_size(cartridge,cartridge->mbc1.bank[1]);
        }
    }
    else{
        gb_cartridge_set_rom0_bank(cartridge,0x00);
        if(cartridge->ram_size > 0){
            gb_cartridge_set_ram_bank(cartridge,0x00);
        }
    }
    gb_cartridge_set_rom1_bank(cartridge,(cartridge->mbc1.bank[1] << (cartridge->mbc1.is_mbc1m ? 0x04 : 0x05)) | cartridge->mbc1.bank[0]);
}

void gb_mbc1_write_register0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    //0x2000-0x3FFF
    if(address >= 0x2000){
        cartridge->mbc1.bank[0] = cartridge->mbc1.is_mbc1m ? (value & 0x0F) : (value & 0x1F);
        if(!cartridge->mbc1.bank[0]){
            cartridge->mbc1.bank[0] = 0x01;
        }
        gb_mbc1_update_mapping(cartridge);
    }
    //0x0000-0x1FFF
    else{
        cartridge->mbc1.ram_enabled = (value & 0x0F) == 0x0A;
    }
}

void gb_mbc1_write_register1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    //0x6000-0x7FFF
    if(address >= 0x6000){
        cartridge->mbc1.mode = value & 0x01;
    }
    //0x4000-0x5FFF
    else{
        cartridge->mbc1.bank[1] = value & 0x03;
    }
    gb_mbc1_update_mapping(cartridge);
}

void gb_mbc1_reset(gb_cartridge_t* cartridge){
    cartridge->mbc1.ram_enabled = false;
    cartridge->mbc1.bank[0] = 0x01;
    cartridge->mbc1.bank[1] = 0x00;
    cartridge->mbc1.mode = false;
    gb_mbc1_update_mapping(cartridge);
}