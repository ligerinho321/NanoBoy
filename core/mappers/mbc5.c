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
    cartridge->mapper.rom_absolute_address = gb_mbc5_rom_absolute_address;
    cartridge->mapper.ram_absolute_address = gb_mbc5_ram_absolute_address;
    cartridge->mapper.reset = gb_mbc5_reset;
    cartridge->mapper.save_state = gb_mbc5_save_state;
    cartridge->mapper.load_state = gb_mbc5_load_state;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->rom0_descriptor.write = gb_mbc5_write_register_0;
    cartridge->rom1_descriptor.write = gb_mbc5_write_register_1;

    if(flags & gb_cartridge_ram){
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }
    }

    return true;
}


void gb_mbc5_write_register_0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    //0x0000-0x1FFF
    if(address < 0x2000){
        if(cartridge->ram_length > 0x00){

            mbc5->ram_enabled = (value & 0x0F) == 0x0A;

            if(mbc5->ram_enabled){
                cartridge->ram_descriptor.write = gb_cartridge_write_ram;
                cartridge->ram_descriptor.read = gb_cartridge_read_ram;
            }
            else{
                cartridge->ram_descriptor.write = gb_memory_write_empty;
                cartridge->ram_descriptor.read = gb_memory_read_empty;
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

void gb_mbc5_write_register_1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    //0x4000-0x5FFF
    if(address < 0x6000 && cartridge->ram_length > 0x00){
        mbc5->ram_bank = value & 0x0F;
        gb_cartridge_set_ram_bank(cartridge,mbc5->ram_bank);
    }
}


size_t gb_mbc5_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    //$0000-$3FFF
    if((relative_address & 0x7FFF) < 0x4000){
        return relative_address & 0x3FFF;
    }
    //$4000-$7FFF
    else{
        return ((mbc5->rom_bank & cartridge->rom_bank_mask) << 0x0E) | (relative_address & 0x3FFF);
    }
}

size_t gb_mbc5_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    //$A000-$BFFF
    if(mbc5->ram_enabled && cartridge->ram_length > 0){
        return ((mbc5->ram_bank & cartridge->ram_bank_mask) << 0x0D) | (relative_address & cartridge->ram_address_mask);
    }
    else{
        return (size_t)-1;
    }
}


void gb_mbc5_reset(gb_cartridge_t* cartridge){
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    mbc5->rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,mbc5->rom_bank);

    if(cartridge->ram_length > 0x00){
        mbc5->ram_enabled = false;

        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;

        mbc5->ram_bank = 0x00;
        gb_cartridge_set_ram_bank(cartridge,mbc5->ram_bank);
    }
}


void gb_mbc5_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    gb_state_write(state,mbc5->rom_bank);

    if(cartridge->ram_length > 0x00){
        gb_state_write(state,mbc5->ram_enabled);
        gb_state_write(state,mbc5->ram_bank);
    }
}

void gb_mbc5_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc5_t* mbc5 = (gb_mbc5_t*)cartridge->mapper.data;

    gb_state_read(state,mbc5->rom_bank);
    gb_cartridge_set_rom1_bank(cartridge,mbc5->rom_bank);
    
    if(cartridge->ram_length > 0x00){
        gb_state_read(state,mbc5->ram_enabled);

        if(mbc5->ram_enabled){
            cartridge->ram_descriptor.write = gb_cartridge_write_ram;
            cartridge->ram_descriptor.read = gb_cartridge_read_ram;
        }
        else{
            cartridge->ram_descriptor.write = gb_memory_write_empty;
            cartridge->ram_descriptor.read = gb_memory_read_empty;
        }

        gb_state_read(state,mbc5->ram_bank);
        gb_cartridge_set_ram_bank(cartridge,mbc5->ram_bank);
    }
}