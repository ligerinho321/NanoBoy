#include "mbc2.h"
#include "../gb.h"

bool gb_mbc2_init(gb_cartridge_t* cartridge,uint8_t flags){

    printf("Mapper: MBC2\n");
    
    gb_mbc2_t* mbc2 = malloc(sizeof(gb_mbc2_t));

    if(!mbc2){
        gb_printf_errno(malloc);
        return false;
    }

    memset(mbc2,0x00,sizeof(gb_mbc2_t));

    cartridge->mapper.data = mbc2;
    cartridge->mapper.reset = gb_mbc2_reset;
    cartridge->mapper.save_state = gb_mbc2_save_state;
    cartridge->mapper.load_state = gb_mbc2_load_state;

    cartridge->rom0_handler.write = gb_mbc2_write_register;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->ram = (uint8_t*)malloc(gb_mbc2_ram_size);
    
    if(!cartridge->ram){
        gb_printf_errno(malloc);
        return false;
    }

    cartridge->ram_size = gb_mbc2_ram_size;
    cartridge->ram_bank_mask = gb_mbc2_ram_bank_mask;
    cartridge->ram_address_mask = gb_mbc2_ram_address_mask;
    cartridge->ram_has_battery = flags & gb_cartridge_battery;
    
    gb_cartridge_set_ram_bank(cartridge,0x00);

    return true;
}

void gb_mbc2_write_register(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc2_t* mbc2 = (gb_mbc2_t*)cartridge->mapper.data;

    if(address & 0x0100){
        mbc2->rom_bank = gb_max(0x01,value & 0x0F);
        gb_cartridge_set_rom1_bank(cartridge,mbc2->rom_bank);
    }
    else{
        mbc2->ram_enabled = (value & 0x0F) == 0x0A;
        if(mbc2->ram_enabled){
            cartridge->ram_handler.write = gb_mbc2_write_ram;
            cartridge->ram_handler.read = gb_mbc2_read_ram;
        }
        else{
            cartridge->ram_handler.write = gb_memory_write_empty;
            cartridge->ram_handler.read = gb_memory_read_empty;
        }
    }
}

void gb_mbc2_write_ram(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    cartridge->ram_ptr[address & cartridge->ram_address_mask] = value & 0x0F;
}

uint8_t gb_mbc2_read_ram(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    return 0xF0 | (cartridge->ram_ptr[address & cartridge->ram_address_mask] & 0x0F);
}

void gb_mbc2_reset(gb_cartridge_t* cartridge){
    gb_mbc2_t* mbc2 = (gb_mbc2_t*)cartridge->mapper.data;

    mbc2->ram_enabled = false;
    cartridge->ram_handler.write = gb_memory_write_empty;
    cartridge->ram_handler.read = gb_memory_read_empty;

    mbc2->rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,mbc2->rom_bank);
}

void gb_mbc2_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc2_t* mbc2 = (gb_mbc2_t*)cartridge->mapper.data;

    gb_state_write(state,mbc2->ram_enabled);
    gb_state_write(state,mbc2->rom_bank);
}

void gb_mbc2_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc2_t* mbc2 = (gb_mbc2_t*)cartridge->mapper.data;

    gb_state_read(state,mbc2->ram_enabled);

    if(mbc2->ram_enabled){
        cartridge->ram_handler.write = gb_mbc2_write_ram;
        cartridge->ram_handler.read = gb_mbc2_read_ram;
    }
    else{
        cartridge->ram_handler.write = gb_memory_write_empty;
        cartridge->ram_handler.read = gb_memory_read_empty;
    }

    gb_state_read(state,mbc2->rom_bank);
    
    gb_cartridge_set_rom1_bank(cartridge,mbc2->rom_bank);
}