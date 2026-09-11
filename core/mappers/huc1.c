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
    cartridge->mapper.data_length = sizeof(gb_huc1_t);

    cartridge->mapper.rom_absolute_address = gb_huc1_rom_absolute_address;
    cartridge->mapper.ram_absolute_address = gb_huc1_ram_absolute_address;
    
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
        else if(cartridge->ram_length > 0){
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

        if(cartridge->ram_length > 0){
            gb_cartridge_set_ram_bank(cartridge,huc1->ram_bank);
        }
    }
}


void gb_huc1_write_ir_register(void* data,uint8_t value,uint16_t address){
    gb_unused(data);
    gb_unused(value);
    gb_unused(address);
    //Write to this region to control the IR transmitter. $01 turns it on, $00 turns it off.
}

uint8_t gb_huc1_read_ir_register(void* data,uint16_t address){
    gb_unused(data);
    gb_unused(address);
    // Read from this region to see either $C1 (saw light) or $C0 (did not see light).
    return 0xC0;
}


size_t gb_huc1_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    //$0000-$3FFF
    if((relative_address & 0x7FFF) < 0x4000){
        return relative_address & 0x3FFF;
    }
    //$4000-$7FFF
    else{
        return ((huc1->rom_bank & cartridge->rom_bank_mask) << 0x0E) | (relative_address & 0x3FFF);
    }
}

size_t gb_huc1_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    //$A000-$BFFF
    if(!huc1->ir_enabled && cartridge->ram_length > 0){
        return ((huc1->ram_bank & cartridge->ram_bank_mask) << 0x0D) | (relative_address & cartridge->ram_address_mask);
    }
    else{
        return (size_t)-1;
    }
}


void gb_huc1_reset(gb_cartridge_t* cartridge){
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    huc1->ir_enabled = false;

    huc1->rom_bank = 0x01;
    gb_cartridge_set_rom1_bank(cartridge,huc1->rom_bank);

    huc1->ram_bank = 0x00;

    if(cartridge->ram_length > 0){
        cartridge->ram_descriptor.write = gb_cartridge_write_ram;
        cartridge->ram_descriptor.read = gb_cartridge_read_ram;

        gb_cartridge_set_ram_bank(cartridge,huc1->ram_bank);
    }
    else{
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
    }
}


void gb_huc1_save_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot){
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    memcpy((uint8_t*)snapshot + sizeof(gb_snapshot_t) + cartridge->ram_length,huc1,sizeof(gb_huc1_t));
}

void gb_huc1_load_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot){
    gb_huc1_t* huc1 = (gb_huc1_t*)cartridge->mapper.data;

    memcpy(huc1,(uint8_t*)snapshot + sizeof(gb_snapshot_t) + cartridge->ram_length,sizeof(gb_huc1_t));

    if(huc1->ir_enabled){
        cartridge->ram_descriptor.write = gb_huc1_write_ir_register;
        cartridge->ram_descriptor.read = gb_huc1_read_ir_register;
    }
    else if(cartridge->ram_length > 0){
        cartridge->ram_descriptor.write = gb_cartridge_write_ram;
        cartridge->ram_descriptor.read = gb_cartridge_read_ram;
    }
    else{
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
    }

    gb_cartridge_set_rom1_bank(cartridge,huc1->rom_bank);

    if(cartridge->ram_length > 0){
        gb_cartridge_set_ram_bank(cartridge,huc1->ram_bank);
    }
}