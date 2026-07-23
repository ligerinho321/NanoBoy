#include "mbc5.h"
#include "../gb.h"

bool gb_mbc5_init(gb_cartridge_t* cartridge,uint8_t flags){

    printf("Mapper: MBC5\n");
    
    gb_mbc5_t* mbc5 = malloc(sizeof(gb_mbc5_t));

    if(!mbc5){
        gb_printf_errno(malloc);
        return false;
    }

    memset(mbc5,0x00,sizeof(gb_mbc5_t));

    cartridge->mapper.data = mbc5;
    cartridge->mapper.reset = gb_mbc5_reset;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->rom0_handler.write = gb_mbc5_write_register0;
    cartridge->rom1_handler.write = gb_mbc5_write_register1;

    if(flags & gb_cartridge_ram){
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }
    }

    return true;
}

void gb_mbc5_write_register0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    //0x0000-0x1FFF
    if(address < 0x2000){

        mbc5->ram_enabled = (value & 0x0F) == 0x0A;
        
        if(cartridge->ram_size > 0x00){
            if(mbc5->ram_enabled){
                cartridge->ram_handler.write = gb_cartridge_write_ram;
                cartridge->ram_handler.read = gb_cartridge_read_ram;
            }
            else{
                cartridge->ram_handler.write = gb_memory_write_empty;
                cartridge->ram_handler.read = gb_memory_read_empty;
            }
        }
    }
    //0x2000-0x2FFF
    else if(address < 0x3000){
        mbc5->rom_bank = (mbc5->rom_bank & 0x100) | value;
        gb_cartridge_set_rom1_bank(cartridge,mbc5->rom_bank);
    }
    //0x3000-0x3FFF
    else{
        mbc5->rom_bank = ((value & 0x01) ? 0x100 : 0x00) | (mbc5->rom_bank & 0xFF);
        gb_cartridge_set_rom1_bank(cartridge,mbc5->rom_bank);
    }
}

void gb_mbc5_write_register1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;
    //0x4000-0x5FFF
    if(address < 0x6000){
        mbc5->ram_bank = value & 0x0F;
        if(cartridge->ram_size > 0x00){
            gb_cartridge_set_ram_bank(cartridge,mbc5->ram_bank);
        }
    }
}

void gb_mbc5_reset(gb_cartridge_t* cartridge){
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    mbc5->rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,mbc5->rom_bank);

    mbc5->ram_enabled = false;
    
    mbc5->ram_bank = 0x00;

    if(cartridge->ram_size > 0x00){
        cartridge->ram_handler.write = gb_memory_write_empty;
        cartridge->ram_handler.read = gb_memory_read_empty;

        gb_cartridge_set_ram_bank(cartridge,mbc5->ram_bank);
    }
}