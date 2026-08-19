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
    cartridge->mapper.rom_absolute_address = gb_mmm01_rom_absolute_address;
    cartridge->mapper.ram_absolute_address = gb_mmm01_ram_absolute_address;
    cartridge->mapper.reset = gb_mmm01_reset;
    cartridge->mapper.save_state = gb_mmm01_save_state;
    cartridge->mapper.load_state = gb_mmm01_load_state;

    cartridge->rom0_descriptor.write = gb_mmm01_write_register_0;
    cartridge->rom1_descriptor.write = gb_mmm01_write_register_1;

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

        uint8_t rom_bank_mid_0 = 0x00;

        uint8_t rom_bank_low_1 = 0x00;
        uint8_t rom_bank_mid_1 = 0x00;

        if(!(mmm01->rom_bank_low & ~mmm01->rom_bank_mask)){
            rom_bank_low_1 = mmm01->rom_bank_low | 0x01;
        }
        else{
            rom_bank_low_1 = mmm01->rom_bank_low;
        }

        if(mmm01->multiplex_enabled){

            if(mmm01->mbc1_mode_select){
                rom_bank_mid_0 = mmm01->ram_bank_low << 0x05;
            }
            else{
                rom_bank_mid_0 = (mmm01->ram_bank_low & mmm01->ram_bank_mask) << 0x05;
            }

            rom_bank_mid_1 = mmm01->ram_bank_low << 0x05;
        }
        else{
            rom_bank_mid_0 = mmm01->rom_bank_mid << 0x05;

            rom_bank_mid_1 = mmm01->rom_bank_mid << 0x05;
        }

        mmm01->rom_bank_0 = (mmm01->rom_bank_high << 0x07) | rom_bank_mid_0 | (mmm01->rom_bank_low & mmm01->rom_bank_mask);
        mmm01->rom_bank_1 = (mmm01->rom_bank_high << 0x07) | rom_bank_mid_1 | rom_bank_low_1;
    }
    else{
        mmm01->rom_bank_0 = cartridge->rom_bank_mask - 0x01;
        mmm01->rom_bank_1 = cartridge->rom_bank_mask - 0x00;
    }

    gb_cartridge_set_rom0_bank(cartridge,mmm01->rom_bank_0);
    gb_cartridge_set_rom1_bank(cartridge,mmm01->rom_bank_1);


    if(cartridge->ram_length > 0x00){

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

        mmm01->ram_bank = (mmm01->ram_bank_high << 0x02) | ram_bank_low;

        gb_cartridge_set_ram_bank(cartridge,mmm01->ram_bank);
    }
}


void gb_mmm01_write_register_0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    //0x0000-0x1FFF
    if(address < 0x2000){

        mmm01->ram_enabled = (value & 0x0F) == 0x0A;

        if(cartridge->ram_length > 0x00){
            if(mmm01->ram_enabled){
                cartridge->ram_descriptor.write = gb_cartridge_write_ram;
                cartridge->ram_descriptor.read = gb_cartridge_read_ram;
            }
            else{
                cartridge->ram_descriptor.write = gb_memory_write_empty;
                cartridge->ram_descriptor.read = gb_memory_read_empty;
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


size_t gb_mmm01_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    //$0000-$3FFF
    if((relative_address & 0x7FFF) < 0x4000){
        return ((mmm01->rom_bank_0 & cartridge->rom_bank_mask) << 0x0E) | (relative_address & 0x3FFF);
    }
    //$4000-$7FFF
    else{
        return ((mmm01->rom_bank_1 & cartridge->rom_bank_mask) << 0x0E) | (relative_address & 0x3FFF);
    }
}

size_t gb_mmm01_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    //$A000-$BFFF
    if(mmm01->ram_enabled && cartridge->ram_length > 0){
        return ((mmm01->ram_bank & cartridge->ram_bank_mask) << 0x0D) | (relative_address & cartridge->ram_address_mask);
    }
    else{
        return (size_t)-1;
    }
}


void gb_mmm01_reset(gb_cartridge_t* cartridge){

    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    mmm01->ram_enabled = false;

    if(cartridge->ram_length > 0x00){
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
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


void gb_mmm01_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    gb_state_write(state,mmm01->ram_enabled);
    
    gb_state_write(state,mmm01->mapping_enabled);

    gb_state_write(state,mmm01->mbc1_mode_locked);
    gb_state_write(state,mmm01->mbc1_mode_select);

    gb_state_write(state,mmm01->multiplex_enabled);

    gb_state_write(state,mmm01->ram_bank_mask);
    gb_state_write(state,mmm01->rom_bank_mask);

    gb_state_write(state,mmm01->rom_bank_low);
    gb_state_write(state,mmm01->rom_bank_mid);
    gb_state_write(state,mmm01->rom_bank_high);

    gb_state_write(state,mmm01->ram_bank_low);
    gb_state_write(state,mmm01->ram_bank_high);
}

void gb_mmm01_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mmm01_t* mmm01 = (gb_mmm01_t*)cartridge->mapper.data;

    gb_state_read(state,mmm01->ram_enabled);

    if(cartridge->ram_length > 0x00){
        if(mmm01->ram_enabled){
            cartridge->ram_descriptor.write = gb_cartridge_write_ram;
            cartridge->ram_descriptor.read = gb_cartridge_read_ram;
        }
        else{
            cartridge->ram_descriptor.write = gb_memory_write_empty;
            cartridge->ram_descriptor.read = gb_memory_read_empty;
        }
    }

    gb_state_read(state,mmm01->mapping_enabled);

    gb_state_read(state,mmm01->mbc1_mode_locked);
    gb_state_read(state,mmm01->mbc1_mode_select);

    gb_state_read(state,mmm01->multiplex_enabled);

    gb_state_read(state,mmm01->ram_bank_mask);
    gb_state_read(state,mmm01->rom_bank_mask);

    gb_state_read(state,mmm01->rom_bank_low);
    gb_state_read(state,mmm01->rom_bank_mid);
    gb_state_read(state,mmm01->rom_bank_high);

    gb_state_read(state,mmm01->ram_bank_low);
    gb_state_read(state,mmm01->ram_bank_high);

    gb_mmm01_update_mapping(cartridge);
}