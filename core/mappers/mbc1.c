#include "mbc1.h"
#include "../gb.h"

bool gb_mbc1_init(gb_cartridge_t* cartridge,uint8_t flags){

    printf("Mapper: MBC1\n");
    
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)malloc(sizeof(gb_mbc1_t));

    if(!mbc1){
        gb_printf_errno(malloc);
        return false;
    }

    memset(mbc1,0x00,sizeof(gb_mbc1_t));

    //mortal kombat 1 and 2 room layout
    //menu    0x00000-0x3FFFF
    //rom1    0x40000-0x7FFFF
    //rom2    0x80000-0xBFFFF
    //padding 0xC0000-0xFFFFF
    mbc1->is_mbc1m = cartridge->rom_length > 0x40133 && !memcmp(cartridge->rom + 0x00104,cartridge->rom + 0x40104,0x30);

    cartridge->mapper.data = mbc1;
    cartridge->mapper.rom_absolute_address = gb_mbc1_rom_absolute_address;
    cartridge->mapper.ram_absolute_address = gb_mbc1_ram_absolute_address;
    cartridge->mapper.reset = gb_mbc1_reset;
    cartridge->mapper.save_state = gb_mbc1_save_state;
    cartridge->mapper.load_state = gb_mbc1_load_state;

    cartridge->rom0_descriptor.write = gb_mbc1_write_register_0;
    cartridge->rom1_descriptor.write = gb_mbc1_write_register_1;

    if(flags & gb_cartridge_ram){
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }
    }
    
    return true;
}


static inline void gb_mbc1_update_mapping(gb_cartridge_t* cartridge){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;

    if(mbc1->mode){
        mbc1->rom_bank_0 = mbc1->bank_1 << (mbc1->is_mbc1m ? 0x04 : 0x05);

        gb_cartridge_set_rom0_bank(cartridge,mbc1->rom_bank_0);
        
        if(cartridge->ram_length > 0x00){

            mbc1->ram_bank = mbc1->bank_1;

            gb_cartridge_set_ram_bank(cartridge,mbc1->ram_bank);
        }
    }
    else{
        mbc1->rom_bank_0 = 0x00;

        gb_cartridge_set_rom0_bank(cartridge,mbc1->rom_bank_0);
        
        if(cartridge->ram_length > 0x00){

            mbc1->ram_bank = 0x00;

            gb_cartridge_set_ram_bank(cartridge,mbc1->ram_bank);
        }
    }

    if(mbc1->is_mbc1m){
        mbc1->rom_bank_1 = (mbc1->bank_1 << 0x04) | (mbc1->bank_0 & 0x0F);  
    }
    else{
        mbc1->rom_bank_1 = (mbc1->bank_1 << 0x05) | (mbc1->bank_0 & 0x1F);
    }

    gb_cartridge_set_rom1_bank(cartridge,mbc1->rom_bank_1);
}


void gb_mbc1_write_register_0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;
    //0x0000-0x1FFF
    if(address < 0x2000){

        mbc1->ram_enabled = (value & 0x0F) == 0x0A;

        if(cartridge->ram_length > 0x00){
            if(mbc1->ram_enabled){
                cartridge->ram_descriptor.write = gb_cartridge_write_ram;
                cartridge->ram_descriptor.read = gb_cartridge_read_ram;
            }
            else{
                cartridge->ram_descriptor.write = gb_memory_write_empty;
                cartridge->ram_descriptor.read = gb_memory_read_empty;
            }
        }
    }
    //0x2000-0x3FFF
    else{
        mbc1->bank_0 = gb_max(0x01,value & 0x1F);
        gb_mbc1_update_mapping(cartridge);
    }
}

void gb_mbc1_write_register_1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;
    //0x4000-0x5FFF
    if(address < 0x6000){
        mbc1->bank_1 = value & 0x03;
    }
    //0x6000-0x7FFF
    else{
        mbc1->mode = value & 0x01;
    }
    
    gb_mbc1_update_mapping(cartridge);
}


size_t gb_mbc1_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;

    //$0000-$3FFF
    if((relative_address & 0x7FFF) < 0x4000){
        return ((mbc1->rom_bank_0 & cartridge->rom_bank_mask) << 0x0E) | (relative_address & 0x3FFF);
    }
    //$4000-$7FFF
    else{
        return ((mbc1->rom_bank_1 & cartridge->rom_bank_mask) << 0x0E) | (relative_address & 0x3FFF);
    }
}

size_t gb_mbc1_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;

    //$A000-$BFFF
    if(mbc1->ram_enabled && cartridge->ram_length > 0){
        return ((mbc1->ram_bank & cartridge->ram_bank_mask) << 0x0D) | (relative_address & cartridge->ram_address_mask);
    }
    else{
        return (size_t)-1;
    }
}


void gb_mbc1_reset(gb_cartridge_t* cartridge){
    
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;

    mbc1->ram_enabled = false;

    if(cartridge->ram_length > 0x00){
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
    }

    mbc1->mode = false;

    mbc1->bank_0 = 0x01;
    mbc1->bank_1 = 0x00;

    gb_mbc1_update_mapping(cartridge);
}


void gb_mbc1_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;

    gb_state_write(state,mbc1->ram_enabled);
    gb_state_write(state,mbc1->mode);
    gb_state_write(state,mbc1->bank_0);
    gb_state_write(state,mbc1->bank_1);
}

void gb_mbc1_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc1_t* mbc1 = (gb_mbc1_t*)cartridge->mapper.data;

    gb_state_read(state,mbc1->ram_enabled);
    gb_state_read(state,mbc1->mode);
    gb_state_read(state,mbc1->bank_0);
    gb_state_read(state,mbc1->bank_1);

    gb_mbc1_update_mapping(cartridge);
}