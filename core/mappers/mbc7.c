#include "mbc7.h"
#include "../gb.h"

bool gb_mbc7_init(gb_cartridge_t* cartridge,uint8_t flags){
    printf("Mapper: MBC7\n");

    gb_mbc7_t* mbc7 = (gb_mbc7_t*)malloc(sizeof(gb_mbc7_t));

    if(!mbc7){
        gb_printf_errno(malloc);
        return false;
    }

    memset(mbc7,0x00,sizeof(gb_mbc7_t));

    cartridge->mapper.data = mbc7;
    cartridge->mapper.reset = gb_mbc7_reset;
    cartridge->mapper.save_state = gb_mbc7_save_state;
    cartridge->mapper.load_state = gb_mbc7_load_state;

    uint8_t* ram = (uint8_t*)malloc(gb_eeprom93lc56_ram_size);
    
    if(!ram){
        gb_printf_errno(malloc);
        return false;
    }

    memset(ram,0x00,gb_eeprom93lc56_ram_size);

    mbc7->eeprom.ram = ram;

    cartridge->ram = ram;
    cartridge->ram_size = gb_eeprom93lc56_ram_size;
    cartridge->ram_bank_mask = gb_eeprom93lc56_ram_bank_mask;
    cartridge->ram_address_mask = gb_eeprom93lc56_ram_address_mask;
    cartridge->ram_has_battery = flags & gb_cartridge_battery;

    cartridge->rom0_handler.write = gb_mbc7_write_register_0;
    cartridge->rom1_handler.write = gb_mbc7_write_register_1;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    return true;
}


void gb_mbc7_write_register_0(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc7_t* mbc7 = (gb_mbc7_t*)cartridge->mapper.data;

    //0x0000-0x1FFF
    if(address < 0x2000){
        mbc7->ram_1_enabled = (value & 0x0F) == 0x0A;

        if(mbc7->ram_1_enabled && mbc7->ram_2_enabled){
            cartridge->ram_handler.write = gb_mbc7_write_register_2;
            cartridge->ram_handler.read = gb_mbc7_read_register_2;
        }
        else{
            cartridge->ram_handler.write = gb_memory_write_empty;
            cartridge->ram_handler.read = gb_memory_read_empty;
        }
    }
    //0x2000-0x3FFF
    else{
        mbc7->rom_bank = value & 0x7F;
        gb_cartridge_set_rom1_bank(cartridge,mbc7->rom_bank);
    }
}

void gb_mbc7_write_register_1(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc7_t* mbc7 = (gb_mbc7_t*)cartridge->mapper.data;

    //0x4000-0x5FFF
    if(address < 0x6000){
        mbc7->ram_2_enabled = value == 0x40;

        if(mbc7->ram_1_enabled && mbc7->ram_2_enabled){
            cartridge->ram_handler.write = gb_mbc7_write_register_2;
            cartridge->ram_handler.read = gb_mbc7_read_register_2;
        }
        else{
            cartridge->ram_handler.write = gb_memory_write_empty;
            cartridge->ram_handler.read = gb_memory_read_empty;
        }
    }
}

void gb_mbc7_write_register_2(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc7_t* mbc7 = (gb_mbc7_t*)cartridge->mapper.data;

    //0xA000-0xAFFF;
    if(address < 0xB000){
        switch((address & 0xF0) >> 0x04){
            case 0x00:{
                if(value == 0x55){
                    mbc7->latch = true;
                    mbc7->latched_accel_x = 0x8000;
                    mbc7->latched_accel_y = 0x8000;
                }
                break;
            }
            case 0x01:{
                if(value == 0xAA && mbc7->latch){
                    mbc7->latch = false;
                    mbc7->latched_accel_x = 0x81D0;
                    mbc7->latched_accel_y = 0x81D0;
                }
                break;
            }
            case 0x08:{
                gb_eeprom93lc56_write(&mbc7->eeprom,value);
                break;
            }
        }
    }
}

uint8_t gb_mbc7_read_register_2(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc7_t* mbc7 = (gb_mbc7_t*)cartridge->mapper.data;

    uint8_t value = 0xFF;

    //0xA000-0xAFFF
    if(address < 0xB000){
        switch((address & 0xF0) >> 0x04){
            case 0x02: value = mbc7->latched_accel_x & 0xFF; break;
            case 0x03: value = mbc7->latched_accel_x >> 0x08; break;

            case 0x04: value = mbc7->latched_accel_y & 0xFF; break;
            case 0x05: value = mbc7->latched_accel_y >> 0x08; break;

            case 0x06: value = 0x00; break;
            case 0x07: value = 0xFF; break;

            case 0x08: value = gb_eeprom93lc56_read(&mbc7->eeprom); break;
        }
    }

    return value;
}


void gb_mbc7_reset(gb_cartridge_t* cartridge){
    gb_mbc7_t* mbc7 = (gb_mbc7_t*)cartridge->mapper.data;

    mbc7->ram_1_enabled = false;
    mbc7->ram_2_enabled = false;

    cartridge->ram_handler.write = gb_memory_write_empty;
    cartridge->ram_handler.read = gb_memory_read_empty;

    mbc7->rom_bank = 0x00;

    gb_cartridge_set_rom1_bank(cartridge,mbc7->rom_bank);

    mbc7->latch = false;
    mbc7->latched_accel_x = 0x8000;
    mbc7->latched_accel_y = 0x8000;

    gb_eeprom93lc56_reset(&mbc7->eeprom);
}


void gb_mbc7_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc7_t* mbc7 = (gb_mbc7_t*)cartridge->mapper.data;

    gb_state_write(state,mbc7->ram_1_enabled);
    gb_state_write(state,mbc7->ram_2_enabled);

    gb_state_write(state,mbc7->rom_bank);

    gb_state_write(state,mbc7->latch);
    gb_state_write(state,mbc7->latched_accel_x);
    gb_state_write(state,mbc7->latched_accel_y);

    gb_eeprom93lc56_save_state(&mbc7->eeprom,state);
}

void gb_mbc7_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc7_t* mbc7 = (gb_mbc7_t*)cartridge->mapper.data;

    gb_state_read(state,mbc7->ram_1_enabled);
    gb_state_read(state,mbc7->ram_2_enabled);

    if(mbc7->ram_1_enabled && mbc7->ram_2_enabled){
        cartridge->ram_handler.write = gb_mbc7_write_register_2;
        cartridge->ram_handler.read = gb_mbc7_read_register_2;
    }
    else{
        cartridge->ram_handler.write = gb_memory_write_empty;
        cartridge->ram_handler.read = gb_memory_read_empty;
    }

    gb_state_write(state,mbc7->rom_bank);

    gb_cartridge_set_rom1_bank(cartridge,mbc7->rom_bank);

    gb_state_write(state,mbc7->latch);
    gb_state_write(state,mbc7->latched_accel_x);
    gb_state_write(state,mbc7->latched_accel_y);

    gb_eeprom93lc56_load_state(&mbc7->eeprom,state);
}


void gb_eeprom93lc56_write(gb_eeprom93lc56_t* eeprom,uint8_t value){
    bool prev_clk = eeprom->clk;

    eeprom->di = value & 0x02;
    eeprom->clk = value & 0x40;
    eeprom->cs = value & 0x80;

    if(!eeprom->cs){
        eeprom->state = gb_eeprom93lc56_idle_state;
        return;
    }

    if(prev_clk || !eeprom->clk) return;

    switch(eeprom->state){
        case gb_eeprom93lc56_idle_state:{
            if(eeprom->di){
                eeprom->state = gb_eeprom93lc56_command_state;
                eeprom->command = 0x00;
                eeprom->command_bits = 0x00;
            }
            break;
        }
        case gb_eeprom93lc56_command_state:{
            eeprom->command <<= 0x01;
            eeprom->command |= eeprom->di;
            if(++eeprom->command_bits >= 0x0A){
                switch(eeprom->command & 0x300){
                    //WRITE
                    case 0x100:{
                        eeprom->write_data = 0x0000;
                        eeprom->write_count = 0x00;
                        eeprom->state = gb_eeprom93lc56_write_state;
                        break;
                    }
                    //READ
                    case 0x200:{
                        uint8_t address = (eeprom->command & 0x7F) << 0x01;
                        eeprom->read_data = (eeprom->ram[address + 0x01] << 0x08) | eeprom->ram[address + 0x00];
                        eeprom->read_count = -1;
                        eeprom->state = gb_eeprom93lc56_read_state;
                        break;
                    }
                    //ERASE
                    case 0x300:{
                        if(eeprom->write_enabled){
                            uint8_t address = (eeprom->command & 0x7F) << 0x01;
                            eeprom->ram[address + 0x00] = 0xFF;
                            eeprom->ram[address + 0x01] = 0xFF;
                        }
                        eeprom->state = gb_eeprom93lc56_idle_state;
                        break;
                    }
                    default:{
                        switch(eeprom->command & 0x3C0){
                            //EWDS
                            case 0x00:{
                                eeprom->write_enabled = false;
                                eeprom->state = gb_eeprom93lc56_idle_state;
                                break;
                            }
                            //WRAL
                            case 0x40:{
                                eeprom->write_data = 0x0000;
                                eeprom->write_count = 0x00;
                                eeprom->state = gb_eeprom93lc56_write_all_state;
                                break;
                            }
                            //ERAL
                            case 0x80:{
                                if(eeprom->write_enabled){
                                    memset(eeprom->ram,0xFF,gb_eeprom93lc56_ram_size);
                                }
                                eeprom->state = gb_eeprom93lc56_idle_state;
                                break;
                            }
                            //EWEN
                            case 0xC0:{
                                eeprom->write_enabled = true;
                                eeprom->state = gb_eeprom93lc56_idle_state;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            break;
        }
        case gb_eeprom93lc56_write_state:{
            eeprom->write_data <<= 0x01;
            eeprom->write_data |= eeprom->di;
            if(++eeprom->write_count >= 0x10){
                if(eeprom->write_enabled){
                    uint8_t address = (eeprom->command & 0x7F) << 0x01;
                    eeprom->ram[address + 0x00] = eeprom->write_data & 0xFF;
                    eeprom->ram[address + 0x01] = eeprom->write_data >> 0x08;
                }
                eeprom->state = gb_eeprom93lc56_idle_state;
            }
            break;
        }
        case gb_eeprom93lc56_write_all_state:{
            eeprom->write_data <<= 0x01;
            eeprom->write_data |= eeprom->di;
            if(++eeprom->write_count >= 0x10){
                if(eeprom->write_enabled){
                    for(int i = 0x00; i < gb_eeprom93lc56_ram_size; i += 0x02){
                        eeprom->ram[i + 0x00] = eeprom->write_data & 0xFF;
                        eeprom->ram[i + 0x01] = eeprom->write_data >> 0x08;
                    }
                }
                eeprom->state = gb_eeprom93lc56_idle_state;
            }
            break;
        }
        case gb_eeprom93lc56_read_state:{
            if(eeprom->read_count >= 0x00){
                eeprom->read_data <<= 0x01;
            }
            if(++eeprom->read_count >= 0x10){
                eeprom->state = eeprom->state = gb_eeprom93lc56_idle_state;
            }
            break;
        }
    }
}

uint8_t gb_eeprom93lc56_read(gb_eeprom93lc56_t* eeprom){

    uint8_t value = (
        (eeprom->di ? 0x02 : 0x00) |
        (eeprom->clk ? 0x40 : 0x00) |
        (eeprom->cs ? 0x80 : 0x00)
    );

    if(eeprom->state == gb_eeprom93lc56_read_state){
        if(eeprom->read_count >= 0x00){
            value |= (eeprom->read_data & 0x8000) ? 0x01 : 0x00;
        }
    }
    else{
        value |= (eeprom->state == gb_eeprom93lc56_idle_state) ? 0x01 : 0x00;
    }

    return value;
}


void gb_eeprom93lc56_reset(gb_eeprom93lc56_t* eeprom){
    eeprom->di = 0x00;
    eeprom->clk = 0x00;
    eeprom->cs = 0x00;

    eeprom->write_enabled = false;

    eeprom->state = gb_eeprom93lc56_idle_state;
    
    eeprom->command = 0x00;
    eeprom->command_bits = 0x00;

    eeprom->read_data = 0x00;
    eeprom->read_count = 0x00;

    eeprom->write_data = 0x00;
    eeprom->write_count = 0x00;
}


void gb_eeprom93lc56_save_state(gb_eeprom93lc56_t* eeprom,gb_state_t* state){
    gb_state_write(state,eeprom->di);
    gb_state_write(state,eeprom->clk);
    gb_state_write(state,eeprom->cs);
    gb_state_write(state,eeprom->write_enabled);
    gb_state_write(state,eeprom->state);
    gb_state_write(state,eeprom->command);
    gb_state_write(state,eeprom->command_bits);
    gb_state_write(state,eeprom->read_data);
    gb_state_write(state,eeprom->read_count);
    gb_state_write(state,eeprom->write_data);
    gb_state_write(state,eeprom->write_count);
    gb_state_write_ex(state,eeprom->ram,gb_eeprom93lc56_ram_size);
}

void gb_eeprom93lc56_load_state(gb_eeprom93lc56_t* eeprom,gb_state_t* state){
    gb_state_read(state,eeprom->di);
    gb_state_read(state,eeprom->clk);
    gb_state_read(state,eeprom->cs);
    gb_state_read(state,eeprom->write_enabled);
    gb_state_read(state,eeprom->state);
    gb_state_read(state,eeprom->command);
    gb_state_read(state,eeprom->command_bits);
    gb_state_read(state,eeprom->read_data);
    gb_state_read(state,eeprom->read_count);
    gb_state_read(state,eeprom->write_data);
    gb_state_read(state,eeprom->write_count);
    gb_state_read_ex(state,eeprom->ram,gb_eeprom93lc56_ram_size);
}
