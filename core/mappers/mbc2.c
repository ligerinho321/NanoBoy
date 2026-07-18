#include "mbc2.h"
#include "../gb.h"

void gb_mbc2_init(gb_cartridge_t* cartridge,uint8_t flags){

    printf("Mapper: MBC2\n");
    
    cartridge->rom0_handler.write = gb_mbc2_write_register;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->ram_size = 0x200;
    cartridge->ram_bank_mask = 0x00;
    cartridge->ram_address_mask = 0x01FF;
    cartridge->ram_has_battery = flags & gb_cartridge_battery;
    
    gb_cartridge_set_ram_bank(cartridge,0x00);

    cartridge->reset = gb_mbc2_reset;
}

void gb_mbc2_write_register(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;

    if(address & 0x0100){
        cartridge->mbc2.rom_bank = value & 0x0F;
        if(!cartridge->mbc2.rom_bank){
            cartridge->mbc2.rom_bank = 0x01;
        }
        gb_cartridge_set_rom1_bank(cartridge,cartridge->mbc2.rom_bank);
    }
    else{
        cartridge->mbc2.ram_enabled = (value & 0x0F) == 0x0A;
        if(cartridge->mbc2.ram_enabled){
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
    cartridge->mbc2.ram_enabled = false;
    cartridge->ram_handler.write = gb_memory_write_empty;
    cartridge->ram_handler.read = gb_memory_read_empty;

    cartridge->mbc2.rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,cartridge->mbc2.rom_bank);
}