#include "mbc3.h"
#include "../gb.h"

void gb_mbc3_init(gb_cartridge_t* cartridge,uint8_t flags){

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->mbc3.has_rtc = flags & gb_cartridge_rtc;

    if(flags & gb_cartridge_ram){
        gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery);
    }

    cartridge->reset = gb_mbc3_reset;
}

void gb_mbc3_update_ram_and_rtc_mapping(gb_cartridge_t* cartridge){

    if(!cartridge->mbc3.ram_and_rtc_enabled) goto unmap;

    if(cartridge->mbc3.ram_and_rtc_bank <= 0x07){
        if(cartridge->ram_size){
            gb_cartridge_set_ram_bank(cartridge,cartridge->mbc3.ram_and_rtc_bank);
            cartridge->ram_handler.write = gb_cartridge_write_ram;
            cartridge->ram_handler.read = gb_cartridge_read_ram;
        }
        else{
            goto unmap;
        }
    }
    else if(cartridge->mbc3.ram_and_rtc_bank <= 0x0C){
        if(cartridge->mbc3.has_rtc){
            cartridge->ram_handler.write = gb_mbc3_write_rtc_register;
            cartridge->ram_handler.read = gb_mbc3_read_rtc_register;
        }
        else{
            goto unmap;
        }
    }
    else{
        goto unmap;
    }

    return;

    unmap:
    cartridge->ram_handler.write = NULL;
    cartridge->ram_handler.read = NULL;
}


void gb_mbc3_write_register0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    //0x0000-0x1FFF
    if(address <= 0x1FFF){
        if(!cartridge->ram_size && !cartridge->mbc3.has_rtc) return;

        cartridge->mbc3.ram_and_rtc_enabled = (value & 0x0F) == 0x0A;

        gb_mbc3_update_ram_and_rtc_mapping(cartridge);
    }
    //0x2000-0x3FFF
    else{
        cartridge->mbc3.rom_bank = value & 0x7F;

        if(!cartridge->mbc3.rom_bank){
            cartridge->mbc3.rom_bank = 0x01;
        }
        
        gb_cartridge_set_rom1_bank(cartridge,cartridge->mbc3.rom_bank);
    }
}

void gb_mbc3_write_register1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    //0x4000-0x5FFF
    if(address <= 0x5FFF){
        if(!cartridge->ram_size && !cartridge->mbc3.has_rtc) return;

        cartridge->mbc3.ram_and_rtc_bank = value & 0x0F;

        gb_mbc3_update_ram_and_rtc_mapping(cartridge);
    }
    //0x6000-0x7FFF
    else{
        if(!cartridge->mbc3.has_rtc) return;

        bool new_latch = value & 0x01;

        if(!cartridge->mbc3.rtc.latch && new_latch){
            memcpy(cartridge->mbc3.rtc.latched_reg,cartridge->mbc3.rtc.reg,sizeof(cartridge->mbc3.rtc.latched_reg));
        }

        cartridge->mbc3.rtc.latch = new_latch;
    }
}

void gb_mbc3_write_rtc_register(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    switch(cartridge->mbc3.ram_and_rtc_bank){
        case 0x08: cartridge->mbc3.rtc.reg[0x00] = value & 0x3F; break;
        case 0x09: cartridge->mbc3.rtc.reg[0x01] = value & 0x3F; break;
        case 0x0A: cartridge->mbc3.rtc.reg[0x02] = value & 0x1F; break;
        case 0x0B: cartridge->mbc3.rtc.reg[0x03] = value; break;
        case 0x0C: cartridge->mbc3.rtc.reg[0x04] = value & 0xC1; break;
    }
}

uint8_t gb_mbc3_read_rtc_register(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    uint8_t value = 0xFF;
    switch(cartridge->mbc3.ram_and_rtc_bank){
        case 0x08: value = cartridge->mbc3.rtc.latched_reg[0x00]; break;
        case 0x09: value = cartridge->mbc3.rtc.latched_reg[0x01]; break;
        case 0x0A: value = cartridge->mbc3.rtc.latched_reg[0x02]; break;
        case 0x0B: value = cartridge->mbc3.rtc.latched_reg[0x03]; break;
        case 0x0C: value = cartridge->mbc3.rtc.latched_reg[0x04]; break;
    }
    return value;
}

void gb_mbc3_rtc_clock(gb_cartridge_t* cartridge){
    //TODO
}

void gb_mbc3_reset(gb_cartridge_t* cartridge){
    cartridge->mbc3.rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,cartridge->mbc3.rom_bank);

    if(cartridge->ram_size || cartridge->mbc3.has_rtc){
        cartridge->mbc3.ram_and_rtc_enabled = false;
        cartridge->mbc3.ram_and_rtc_bank = 0x00;
        gb_mbc3_update_ram_and_rtc_mapping(cartridge);
    }

    if(cartridge->mbc3.has_rtc){
        memset(cartridge->mbc3.rtc.latched_reg,0,sizeof(cartridge->mbc3.rtc.latched_reg));
        cartridge->mbc3.rtc.latch = false;
    }
}