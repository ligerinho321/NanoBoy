#include "huc1.h"
#include "../gb.h"

void gb_huc1_init(gb_cartridge_t* cartridge,uint8_t flags){
    printf("Mapper: HuC1\n");

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->rom0_handler.write = gb_huc1_write_register0;
    cartridge->rom1_handler.write = gb_huc1_write_register1;

    if(flags & gb_cartridge_ram){
        gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery);
    }

    cartridge->reset = gb_huc1_reset;
}


void gb_huc1_write_register0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    
    //0x0000-0x1FFF
    if(address < 0x2000){
        cartridge->huc1.ir_enabled = value == 0x0E;

        if(cartridge->huc1.ir_enabled){
            cartridge->ram_handler.write = gb_huc1_write_ir_register;
            cartridge->ram_handler.read = gb_huc1_read_ir_register;
        }
        else if(cartridge->ram_size > 0 && !cartridge->huc1.ir_enabled){
            cartridge->ram_handler.write = gb_cartridge_write_ram;
            cartridge->ram_handler.read = gb_cartridge_read_ram;
        }
    }
    //0x2000-0x3FFF
    else{
        cartridge->huc1.rom_bank = value & 0x3F;

        gb_cartridge_set_rom1_bank(cartridge,cartridge->huc1.rom_bank);
    }
}

void gb_huc1_write_register1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;

    //0x4000-0x5FFF
    if(address < 0x6000){
        cartridge->huc1.ram_bank = value & 0x03;

        if(cartridge->ram_size > 0){
            gb_cartridge_set_ram_bank(cartridge,cartridge->huc1.ram_bank);
        }
    }
}


void gb_huc1_write_ir_register(void* data,uint8_t value,uint16_t address){
    //Write to this region to control the IR transmitter. $01 turns it on, $00 turns it off.
}

uint8_t gb_huc1_read_ir_register(void* data,uint16_t address){
    // Read from this region to see either $C1 (saw light) or $C0 (did not see light).
    return 0xC0;
}


void gb_huc1_reset(gb_cartridge_t* cartridge){
    cartridge->huc1.ir_enabled = false;

    cartridge->huc1.rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,cartridge->huc1.rom_bank);

    cartridge->huc1.ram_bank = 0x00;

    if(cartridge->ram_size > 0){
        cartridge->ram_handler.write = gb_cartridge_write_ram;
        cartridge->ram_handler.read = gb_cartridge_read_ram;

        gb_cartridge_set_ram_bank(cartridge,cartridge->huc1.ram_bank);
    }
}