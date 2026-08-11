#include "huc1.h"
#include "../gb.h"

bool gb_huc1_init(gb_cartridge_t* cartridge,uint8_t flags){
    printf("Mapper: HuC1\n");

    gb_huc1_t* huc1 = (gb_huc1_t*)malloc(sizeof(gb_huc1_t));

    if(!huc1){
        gb_printf_errno(malloc);
        return false;
    }

    memset(huc1,0x00,sizeof(gb_huc1_t));

    cartridge->mapper.data = huc1;
    cartridge->mapper.reset = gb_huc1_reset;
    cartridge->mapper.save_state = gb_huc1_save_state;
    cartridge->mapper.load_state = gb_huc1_load_state;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->rom0_descriptor.write = gb_huc1_write_register_0;
    cartridge->rom1_descriptor.write = gb_huc1_write_register_1;

    if(flags & gb_cartridge_ram){
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }
    }

    return true;
}


void gb_huc1_write_register_0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    //0x0000-0x1FFF
    if(address < 0x2000){
        huc1->ir_enabled = value == 0x0E;

        if(huc1->ir_enabled){
            cartridge->ram_descriptor.write = gb_huc1_write_ir_register;
            cartridge->ram_descriptor.read = gb_huc1_read_ir_register;
        }
        else if(cartridge->ram_size > 0){
            cartridge->ram_descriptor.write = gb_cartridge_write_ram;
            cartridge->ram_descriptor.read = gb_cartridge_read_ram;
        }
        else{
            cartridge->ram_descriptor.write = gb_memory_write_empty;
            cartridge->ram_descriptor.read = gb_memory_read_empty;
        }
    }
    //0x2000-0x3FFF
    else{
        huc1->rom_bank = value & 0x3F;

        gb_cartridge_set_rom1_bank(cartridge,huc1->rom_bank);
    }
}

void gb_huc1_write_register_1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    //0x4000-0x5FFF
    if(address < 0x6000){
        huc1->ram_bank = value & 0x03;

        if(cartridge->ram_size > 0){
            gb_cartridge_set_ram_bank(cartridge,huc1->ram_bank);
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
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    huc1->ir_enabled = false;

    huc1->rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,huc1->rom_bank);

    huc1->ram_bank = 0x00;

    if(cartridge->ram_size > 0){
        cartridge->ram_descriptor.write = gb_cartridge_write_ram;
        cartridge->ram_descriptor.read = gb_cartridge_read_ram;

        gb_cartridge_set_ram_bank(cartridge,huc1->ram_bank);
    }
    else{
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
    }
}


void gb_huc1_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    gb_state_write(state,huc1->ir_enabled);
    gb_state_write(state,huc1->rom_bank);
    gb_state_write(state,huc1->ram_bank);
}

void gb_huc1_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    gb_state_read(state,huc1->ir_enabled);

    if(huc1->ir_enabled){
        cartridge->ram_descriptor.write = gb_huc1_write_ir_register;
        cartridge->ram_descriptor.read = gb_huc1_read_ir_register;
    }
    else if(cartridge->ram_size > 0){
        cartridge->ram_descriptor.write = gb_cartridge_write_ram;
        cartridge->ram_descriptor.read = gb_cartridge_read_ram;
    }
    else{
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
    }
    
    gb_state_read(state,huc1->rom_bank);

    gb_cartridge_set_rom1_bank(cartridge,huc1->rom_bank);

    gb_state_read(state,huc1->ram_bank);

    if(cartridge->ram_size > 0){
        gb_cartridge_set_ram_bank(cartridge,huc1->ram_bank);
    }
}