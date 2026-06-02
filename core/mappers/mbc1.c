#include "mbc1.h"
#include "../gb.h"

void gb_mbc1_init(gb_cartridge_t* cartridge,uint8_t flags){

    printf("Mapper: MBC1\n");
    
    cartridge->rom0_handler.write = gb_mbc1_write_register0;
    cartridge->rom1_handler.write = gb_mbc1_write_register1;

    if(flags & gb_cartridge_ram){
        gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery);
    }

    //mortal kombat 1 and 2 room layout
    //menu    0x00000-0x3FFFF
    //rom1    0x40000-0x7FFFF
    //rom2    0x80000-0xBFFFF
    //padding 0xC0000-0xFFFFF
    cartridge->mbc1.is_mbc1m = cartridge->rom_size > 0x40133 && !memcmp(cartridge->rom + 0x00104,cartridge->rom + 0x40104,0x30);
    
    cartridge->reset = gb_mbc1_reset;
}

void gb_mbc1_update_mapping(gb_cartridge_t* cartridge){
    if(cartridge->mbc1.mode){
        gb_cartridge_set_rom0_bank(cartridge,cartridge->mbc1.bank[1] << (cartridge->mbc1.is_mbc1m ? 0x04 : 0x05));
        if(cartridge->ram_size){
            gb_cartridge_set_ram_bank(cartridge,cartridge->mbc1.bank[1]);
        }
    }
    else{
        gb_cartridge_set_rom0_bank(cartridge,0x00);
        if(cartridge->ram_size){
            gb_cartridge_set_ram_bank(cartridge,0x00);
        }
    }
    if(cartridge->mbc1.is_mbc1m){
        gb_cartridge_set_rom1_bank(cartridge,(cartridge->mbc1.bank[1] << 0x04) | (cartridge->mbc1.bank[0] & 0x0F));   
    }
    else{
        gb_cartridge_set_rom1_bank(cartridge,(cartridge->mbc1.bank[1] << 0x05) | cartridge->mbc1.bank[0]);
    }
}

void gb_mbc1_write_register0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    
    //0x0000-0x1FFF
    if(address < 0x2000){
        if(!cartridge->ram_size) return;

        cartridge->mbc1.ram_enabled = (value & 0x0F) == 0x0A;

        if(cartridge->mbc1.ram_enabled){
            cartridge->ram_handler.write = gb_cartridge_write_ram;
            cartridge->ram_handler.read = gb_cartridge_read_ram;
        }
        else{
            cartridge->ram_handler.write = NULL;
            cartridge->ram_handler.read = NULL;
        }
    }
    //0x2000-0x3FFF
    else{
        cartridge->mbc1.bank[0] = value & 0x1F;
        if(!cartridge->mbc1.bank[0]){
            cartridge->mbc1.bank[0] = 0x01;
        }
        gb_mbc1_update_mapping(cartridge);
    }
}

void gb_mbc1_write_register1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    
    //0x4000-0x5FFF
    if(address < 0x6000){
        cartridge->mbc1.bank[1] = value & 0x03;
    }
    //0x6000-0x7FFF
    else{
        cartridge->mbc1.mode = value & 0x01;
    }
    
    gb_mbc1_update_mapping(cartridge);
}

void gb_mbc1_reset(gb_cartridge_t* cartridge){
    if(cartridge->ram_size){
        cartridge->mbc1.ram_enabled = false;
        cartridge->ram_handler.write = NULL;
        cartridge->ram_handler.read = NULL;
    }
    cartridge->mbc1.bank[0] = 0x01;
    cartridge->mbc1.bank[1] = 0x00;
    cartridge->mbc1.mode = false;
    gb_mbc1_update_mapping(cartridge);
}