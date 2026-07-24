#include "mmm01.h"
#include "../gb.h"

bool gb_mmm01_init(gb_cartridge_t* cartridge,uint8_t flags){
    printf("Mapper: MMM01\n");  

    gb_mmm01_t* mmm01 = (gb_mmm01_t*)malloc(sizeof(gb_mmm01_t));

    if(!mmm01){
        gb_printf_errno(malloc);
        return false;
    }

    memset(mmm01,0x00,sizeof(gb_mmm01_t));

    cartridge->mapper.data = mmm01;
    cartridge->mapper.reset = gb_mmm01_reset;

    cartridge->rom0_handler.write = gb_mmm01_write_register_0;
    cartridge->rom1_handler.write = gb_mmm01_write_register_1;

    if(flags & gb_cartridge_ram){
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }
    }

    return true;
}

static void gb_mmm01_update_mapping(gb_cartridge_t* cartridge){
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    if(mmm01->mapping_enabled){

        uint8_t rom_bank_mid0 = 0x00;

        uint8_t rom_bank_low1 = 0x00;
        uint8_t rom_bank_mid1 = 0x00;

        if(!(mmm01->rom_bank_low & ~mmm01->rom_bank_mask)){
            rom_bank_low1 = mmm01->rom_bank_low | 0x01;
        }
        else{
            rom_bank_low1 = mmm01->rom_bank_low;
        }

        if(mmm01->multiplex_enabled){

            if(mmm01->mbc1_mode_select){
                rom_bank_mid0 = mmm01->ram_bank_low << 0x05;
            }
            else{
                rom_bank_mid0 = (mmm01->ram_bank_low & mmm01->ram_bank_mask) << 0x05;
            }

            rom_bank_mid1 = mmm01->ram_bank_low << 0x05;
        }
        else{
            rom_bank_mid0 = mmm01->rom_bank_mid << 0x05;

            rom_bank_mid1 = mmm01->rom_bank_mid << 0x05;
        }

        uint16_t rom_bank0 = (mmm01->rom_bank_high << 0x07) | rom_bank_mid0 | (mmm01->rom_bank_low & mmm01->rom_bank_mask);
        uint16_t rom_bank1 = (mmm01->rom_bank_high << 0x07) | rom_bank_mid1 | rom_bank_low1;

        gb_cartridge_set_rom0_bank(cartridge,rom_bank0);
        gb_cartridge_set_rom1_bank(cartridge,rom_bank1);
    }
    else{
        gb_cartridge_set_rom0_bank(cartridge,cartridge->rom_bank_mask - 0x01);
        gb_cartridge_set_rom1_bank(cartridge,cartridge->rom_bank_mask - 0x00);
    }


    if(cartridge->ram_size > 0){

        uint8_t ram_bank_low = 0x00;

        if(mmm01->multiplex_enabled){
            ram_bank_low = mmm01->rom_bank_mid;
        }
        else{
            if(mmm01->mbc1_mode_select){
                ram_bank_low = mmm01->ram_bank_low;
            }
            else{
                ram_bank_low = mmm01->ram_bank_low & mmm01->ram_bank_mask;
            }
        }

        uint16_t ram_bank = (mmm01->ram_bank_high << 0x02) | ram_bank_low;

        gb_cartridge_set_ram_bank(cartridge,ram_bank);
    }
}

void gb_mmm01_write_register_0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    //0x0000-0x1FFF
    if(address < 0x2000){

        mmm01->ram_enabled = (value & 0x0F) == 0x0A;

        if(cartridge->ram_size > 0){
            if(mmm01->ram_enabled){
                cartridge->ram_handler.write = gb_cartridge_write_ram;
                cartridge->ram_handler.read = gb_cartridge_read_ram;
            }
            else{
                cartridge->ram_handler.write = gb_memory_write_empty;
                cartridge->ram_handler.read = gb_memory_read_empty;
            }
        }

        if(!mmm01->mapping_enabled){
            
            mmm01->ram_bank_mask = (value & 0x30) >> 0x04;

            mmm01->mapping_enabled = value & 0x40;
        }
    }
    //0x2000-0x3FFF
    else{
        mmm01->rom_bank_low = (mmm01->rom_bank_low & mmm01->rom_bank_mask) | ((value & 0x1F) & ~mmm01->rom_bank_mask);

        if(!mmm01->mapping_enabled){

            mmm01->rom_bank_mid = (value & 0x60) >> 0x05;
        }
    }

    gb_mmm01_update_mapping(cartridge);
}

void gb_mmm01_write_register_1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    //0x4000-0x5FFF
    if(address < 0x6000){
        mmm01->ram_bank_low = (mmm01->ram_bank_low & mmm01->ram_bank_mask) | ((value & 0x03) & ~mmm01->ram_bank_mask);
        
        if(!mmm01->mapping_enabled){
            
            mmm01->ram_bank_high = (value & 0x0C) >> 0x02;
            
            mmm01->rom_bank_high = (value & 0x30) >> 0x04;

            mmm01->mbc1_mode_locked = value & 0x40;
        }
    }
    //0x6000-0x7FFF
    else{
        if(!mmm01->mbc1_mode_locked){

            mmm01->mbc1_mode_select = value & 0x01;
        }
        if(!mmm01->mapping_enabled){
            
            mmm01->rom_bank_mask = (value & 0x3C) >> 0x01;

            mmm01->multiplex_enabled = value & 0x40;
        }
    }

    gb_mmm01_update_mapping(cartridge);

}


void gb_mmm01_reset(gb_cartridge_t* cartridge){

    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    mmm01->ram_enabled = false;

    if(cartridge->ram_size > 0){
        cartridge->ram_handler.write = gb_memory_write_empty;
        cartridge->ram_handler.read = gb_memory_read_empty;
    }

    mmm01->mapping_enabled = false;
    
    mmm01->mbc1_mode_locked = false;
    mmm01->mbc1_mode_select = false;

    mmm01->multiplex_enabled = false;

    mmm01->rom_bank_mask = 0x00;
    mmm01->ram_bank_mask = 0x00;

    mmm01->rom_bank_low = 0x00;
    mmm01->rom_bank_mid = 0x00;
    mmm01->rom_bank_high = 0x00;

    mmm01->ram_bank_low = 0x00;
    mmm01->ram_bank_high = 0x00;

    gb_mmm01_update_mapping(cartridge);
}