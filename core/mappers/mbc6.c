#include "mbc6.h"
#include "../gb.h"

bool gb_mbc6_init(gb_cartridge_t* cartridge,uint8_t flags){

    printf("Mapper: MBC6\n");

    gb_mbc6_t* mbc6 = (gb_mbc6_t*)malloc(sizeof(gb_mbc6_t));

    if(!mbc6){
        gb_printf_errno(malloc);
        return false;
    }
    
    memset(mbc6,0x00,sizeof(gb_mbc6_t));

    mbc6->rom_bank_mask = (cartridge->rom_length / 0x2000) - 0x01;
    mbc6->ram_bank_mask = 0x00;

    cartridge->mapper.data = mbc6;
    cartridge->mapper.data_length = sizeof(gb_mbc6_t);
    
    cartridge->mapper.rom_absolute_address = gb_mbc6_rom_absolute_address;
    cartridge->mapper.ram_absolute_address = gb_mbc6_ram_absolute_address;
    
    cartridge->mapper.reset = gb_mbc6_reset;

    cartridge->mapper.save_state = gb_mbc6_save_state;
    cartridge->mapper.load_state = gb_mbc6_load_state;

    gb_cartridge_set_rom0_bank(cartridge,0x00);

    cartridge->rom0_descriptor.write = gb_mbc6_write_register;
    cartridge->rom1_descriptor.read = gb_mbc6_read_rom_or_flash;

    if(flags & gb_cartridge_ram){
        
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }

        if(cartridge->ram_length == 0x800){
            mbc6->ram_bank_mask = 0x00;
            mbc6->ram_address_mask = 0x07FF;
        }
        else if(cartridge->ram_length >= 0x2000){
            mbc6->ram_bank_mask = (cartridge->ram_length >> 0x0C) - 0x01;
            mbc6->ram_address_mask = 0x0FFF;
        }
    }

    return true;
}


static void gb_mbc6_update_mapping(gb_cartridge_t* cartridge){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    gb_mbc6_state_t* state = &mbc6->state;

    if(cartridge->ram_length > 0){

        mbc6->ram_0_ptr = cartridge->ram + ((state->ram_bank_0 & mbc6->ram_bank_mask) << 0x0C);

        mbc6->ram_1_ptr = cartridge->ram + ((state->ram_bank_1 & mbc6->ram_bank_mask) << 0x0C);
    }

    if(state->flash_bank_0_enabled){
        mbc6->rom_or_flash_0_ptr = state->flash_data + ((state->rom_or_flash_bank_0 & gb_mbc6_flash_bank_mask) << 0x0D);
    }
    else{
        mbc6->rom_or_flash_0_ptr = cartridge->rom + ((state->rom_or_flash_bank_0 & mbc6->rom_bank_mask) << 0x0D);
    }

    if(state->flash_bank_1_enabled){
        mbc6->rom_or_flash_1_ptr = state->flash_data + ((state->rom_or_flash_bank_1 & gb_mbc6_flash_bank_mask) << 0x0D);
    }
    else{
        mbc6->rom_or_flash_1_ptr = cartridge->rom + ((state->rom_or_flash_bank_1 & mbc6->rom_bank_mask) << 0x0D);
    }
}


void gb_mbc6_write_register(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    gb_mbc6_state_t* state = &mbc6->state;

    //0x0000-0x03FF
    if(address < 0x0400){
        state->ram_enabled = (value & 0x0F) == 0x0A;

        if(cartridge->ram_length > 0x00){
            if(state->ram_enabled){
                cartridge->ram_descriptor.write = gb_mbc6_write_ram;
                cartridge->ram_descriptor.read = gb_mbc6_read_ram;
            }
            else{
                cartridge->ram_descriptor.write = gb_memory_write_empty;
                cartridge->ram_descriptor.read = gb_memory_read_empty;
            }
        }
    }
    //0x0400-0x07FF
    else if(address < 0x0800){
        state->ram_bank_0 = value & 0x07;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x0800-0x0BFF
    else if(address < 0x0C00){
        state->ram_bank_1 = value & 0x07;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x0C00-0x0FFF
    else if(address < 0x1000){
        
        state->flash_enabled = value & 0x01;

        if(state->flash_enabled){
            cartridge->rom1_descriptor.write = gb_mbc6_write_flash;
        }
        else{
            cartridge->rom1_descriptor.write = gb_memory_write_empty;
        }
    }
    //0x1000-0x1FFF
    else if(address < 0x2000){
        state->flash_write_enabled = value & 0x01;
    }
    //0x2000-0x27FF
    else if(address < 0x2800){
        state->rom_or_flash_bank_0 = value & 0x7F;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x2800-0x2FFF
    else if(address < 0x3000){
        state->flash_bank_0_enabled = value == 0x08;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x3000-0x37FF
    else if(address < 0x3800){
        state->rom_or_flash_bank_1 = value & 0x7F;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x3800-0x3FFF
    else{
        state->flash_bank_1_enabled = value == 0x08;
        gb_mbc6_update_mapping(cartridge);
    }
}


void gb_mbc6_write_flash(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    gb_mbc6_state_t* state = &mbc6->state;

    if(!((address < 0x6000) ? state->flash_bank_0_enabled : state->flash_bank_1_enabled)) return;

    uint8_t bank = (address < 0x6000) ? state->rom_or_flash_bank_0 : state->rom_or_flash_bank_1;

    uint32_t chip_address = (bank << 0x0D) | (address & 0x1FFF);

    switch(state->flash_command){
        case gb_mbc6_flash_program_flash_command:{
            
            uint8_t flash_buffer_address = chip_address & 0x7F;

            if(flash_buffer_address == state->flash_buffer_last_write_address){
                
                state->flash_command = 0x00;

                uint8_t sector = chip_address >> 0x11;

                if(!((sector > 0x00) || (state->flash_write_enabled && !state->flash_protect_sector_0))) return;

                if(value == gb_mbc6_flash_reset_command) return;

                uint8_t* src = state->flash_buffer;

                uint8_t* dst = state->flash_data + (chip_address & ~0x7F);
                
                uint8_t* end = dst + sizeof(state->flash_buffer);

                while(dst < end) *dst++ &= *src++;
            }
            else{
                state->flash_buffer[flash_buffer_address] = value;
                state->flash_buffer_last_write_address = flash_buffer_address;
            }

            return;
        }
        case gb_mbc6_flash_program_map_command:{

            uint8_t flash_buffer_address = chip_address & 0x7F;

            if(flash_buffer_address == state->flash_buffer_last_write_address){
                
                state->flash_command = 0x00;

                if(value == gb_mbc6_flash_reset_command) return;

                uint8_t* src = state->flash_buffer;

                uint8_t* dst = state->flash_map_data + (chip_address & 0x80);
                
                uint8_t* end = dst + sizeof(state->flash_buffer);

                while(dst < end) *dst++ &= *src++;

            }
            else{
                state->flash_buffer[flash_buffer_address] = value;
                state->flash_buffer_last_write_address = flash_buffer_address;
            }

            return;
        }
    }

    if(value == gb_mbc6_flash_reset_command){
        state->flash_pre_command = 0x00;
        state->flash_command = 0x00;
        state->flash_state = 0x00;
        return;
    }

    switch(state->flash_state){
        case 0x00:{
            if(chip_address == 0x5555 && value == 0xAA){
                state->flash_state = 0x01;
            }
            else{
                state->flash_pre_command = 0x00;
            }
            break;
        }
        case 0x01:{
            if(chip_address == 0x2AAA && value == 0x55){
                state->flash_state = state->flash_pre_command ? 0x03 : 0x02;
            }
            else{
                state->flash_state = 0x00;
                state->flash_pre_command = 0x00;
            }
            break;
        }
        case 0x02:{
            if(chip_address == 0x5555){
                switch(value){
                    case gb_mbc6_flash_read_id_command:{
                        state->flash_command = gb_mbc6_flash_read_id_command;
                        break;
                    }
                    case gb_mbc6_flash_program_flash_command:{
                        state->flash_command = gb_mbc6_flash_program_flash_command;
                        state->flash_buffer_last_write_address = 0xFF;
                        memset(state->flash_buffer,0xFF,sizeof(state->flash_buffer));
                        break;
                    }
                    case 0x60:{
                        state->flash_pre_command = 0x60;
                        break;
                    }
                    case 0x77:{
                        state->flash_pre_command = 0x77;
                        break;
                    }
                    case 0x80:{
                        state->flash_pre_command = 0x80;
                        break;
                    }
                }
            }
            state->flash_state = 0x00;
            break;
        }
        case 0x03:{
            switch(state->flash_pre_command){
                case 0x60:{
                    switch(value){
                        case gb_mbc6_flash_erase_map_command:{
                            if(chip_address == 0x5555 && state->flash_write_enabled){
                                state->flash_command = gb_mbc6_flash_erase_map_command;
                                memset(state->flash_map_data,0xFF,sizeof(state->flash_map_data));
                            }
                            break;
                        }
                        case gb_mbc6_flash_protect_sector_0_command:{
                            if(!state->flash_protect_sector_0 && bank < 0x10){
                                state->flash_protect_sector_0 = true;
                                state->flash_command = gb_mbc6_flash_protect_sector_0_command;
                            }
                            break;
                        }
                        case gb_mbc6_flash_unprotect_sector_0_command:{
                            if(state->flash_protect_sector_0 && bank < 0x10){
                                state->flash_protect_sector_0 = false;
                                state->flash_command = gb_mbc6_flash_unprotect_sector_0_command;
                            }
                            break;
                        }
                        case gb_mbc6_flash_program_map_command:{
                            if(chip_address == 0x5555 && state->flash_write_enabled){
                                state->flash_command = gb_mbc6_flash_program_map_command;
                                state->flash_buffer_last_write_address = 0xFF;
                                memset(state->flash_buffer,0xFF,sizeof(state->flash_buffer));
                            }
                            break;
                        }
                    }
                    break;
                }
                case 0x77:{
                    if(value == gb_mbc6_flash_read_map_command && chip_address == 0x5555){
                        state->flash_command = gb_mbc6_flash_read_map_command;
                    }
                    break;
                }
                case 0x80:{
                    switch(value){
                        case gb_mbc6_flash_mass_erase_flash_command:{
                            if(chip_address == 0x5555){
                                
                                state->flash_command = gb_mbc6_flash_mass_erase_flash_command;

                                if(state->flash_write_enabled && !state->flash_protect_sector_0){
                                    memset(state->flash_data,0xFF,0x20000);
                                }
                                
                                memset(state->flash_data + 0x20000,0xFF,0xE0000);
                            }
                            break;
                        }
                        case gb_mbc6_flash_erase_flash_sector_command:{
                            state->flash_command = gb_mbc6_flash_erase_flash_sector_command;

                            uint8_t sector = chip_address >> 0x11;

                            if((sector > 0x00) || (state->flash_write_enabled && !state->flash_protect_sector_0)){
                                memset(state->flash_data + (sector << 0x11),0xFF,0x20000);
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            
            state->flash_pre_command = 0x00;

            state->flash_state = 0x00;

            break;
        }
        default:{
            state->flash_state = 0x00;
            break;
        }
    }
}

uint8_t gb_mbc6_read_rom_or_flash(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    gb_mbc6_state_t* state = &mbc6->state;

    uint8_t value = 0xFF;

    if(!(state->flash_command && ((address < 0x6000) ? state->flash_bank_0_enabled : state->flash_bank_1_enabled))){
        
        //0x4000-0x5FFF
        if(address < 0x6000){
            value = mbc6->rom_or_flash_0_ptr[address & 0x1FFF];
        }
        //0x6000-0x7FFF
        else{
            value = mbc6->rom_or_flash_1_ptr[address & 0x1FFF];
        }

        return value;
    }

    switch(state->flash_command){
        case gb_mbc6_flash_read_id_command:{
            switch(address & 0x03){
                case 0x00:{
                    //(manufacturer ID: Macronix)
                    value = 0xC2;
                    break;
                }
                case 0x01:{
                    //0x89, if device ID is 29F008ATC (NP GB Mem); 0x81, if device ID is 29F008TC (Net de Get)
                    value = 0x81;
                    break;
                }
                case 0x02:{
                    //0xc2, if read from sector 0; 0x00, if read from sectors 1-7
                    uint8_t bank = (address < 0x6000) ? state->rom_or_flash_bank_0 : state->rom_or_flash_bank_1;
                    value = (bank < 0x10) ? 0xC2 : 0x00;
                    break;
                }
                case 0x03:{
                    value = 0xFF;
                    break;
                }
            }
            break;
        }
        case gb_mbc6_flash_read_map_command:{
            value = state->flash_map_data[address & 0xFF];
            break;
        }
        case gb_mbc6_flash_program_flash_command:
        case gb_mbc6_flash_erase_map_command:
        case gb_mbc6_flash_protect_sector_0_command:
        case gb_mbc6_flash_unprotect_sector_0_command:
        case gb_mbc6_flash_program_map_command:
        case gb_mbc6_flash_mass_erase_flash_command:
        case gb_mbc6_flash_erase_flash_sector_command:{
            value = 0x80 | (state->flash_protect_sector_0 ? 0x02 : 0x00);
            break;
        }
    }

    return value;
}


void gb_mbc6_write_ram(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    //0xA000-0xAFFF
    if(address < 0xB000){
        mbc6->ram_0_ptr[address & mbc6->ram_address_mask] = value;
    }
    //0xB000-0xBFFF
    else{
        mbc6->ram_1_ptr[address & mbc6->ram_address_mask] = value;
    }
}

uint8_t gb_mbc6_read_ram(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    //0xA000-0xAFFF
    if(address < 0xB000){
        return mbc6->ram_0_ptr[address & mbc6->ram_address_mask];
    }
    //0xB000-0xBFFF
    else{
        return mbc6->ram_1_ptr[address & mbc6->ram_address_mask];
    }
}


size_t gb_mbc6_rom_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    //$0000-$3FFF
    if((relative_address & 0x7FFF) < 0x4000){
        return relative_address & 0x3FFF;
    }
    else{
        //$4000-$5FFF
        if((relative_address & 0x3FFF) < 0x2000){
            if(!mbc6->state.flash_bank_0_enabled){
                return ((mbc6->state.rom_or_flash_bank_0 & mbc6->rom_bank_mask) << 0x0D) | (relative_address & 0x1FFF);
            }
            else{
                return (size_t)-1;
            }
        }
        //$6000-$7FFF
        else{
            if(!mbc6->state.flash_bank_1_enabled){
                return ((mbc6->state.rom_or_flash_bank_1 & mbc6->rom_bank_mask) << 0x0D) | (relative_address & 0x1FFF);
            }
            else{
                return (size_t)-1;
            }
        }
    }
}

size_t gb_mbc6_ram_absolute_address(gb_cartridge_t* cartridge,uint16_t relative_address){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    if(mbc6->state.ram_enabled && cartridge->ram_length > 0){
        //$A000-$AFFF
        if((relative_address & 0x1FFF) < 0x1000){
            return ((mbc6->state.ram_bank_0 & mbc6->ram_bank_mask) << 0x0C) | (relative_address & mbc6->ram_address_mask);
        }
        //$B000-$BFFF
        else{
            return ((mbc6->state.ram_bank_1 & mbc6->ram_bank_mask) << 0x0C) | (relative_address & mbc6->ram_address_mask);
        }
    }
    else{
        return (size_t)-1;
    }
}


void gb_mbc6_reset(gb_cartridge_t* cartridge){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    gb_mbc6_state_t* state = &mbc6->state;

    state->ram_enabled = false;

    if(cartridge->ram_length > 0x00){
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
    }

    state->flash_enabled = false;

    cartridge->rom1_descriptor.write = gb_memory_write_empty;

    state->flash_write_enabled = false;

    state->flash_bank_0_enabled = false;
    state->flash_bank_1_enabled = false;

    state->flash_protect_sector_0 = false;

    state->flash_state = 0x00;
    state->flash_pre_command = 0x00;
    state->flash_command = 0x00;

    state->rom_or_flash_bank_0 = 0x02;
    state->rom_or_flash_bank_1 = 0x03;

    state->ram_bank_0 = 0x00;
    state->ram_bank_1 = 0x01;
    
    gb_mbc6_update_mapping(cartridge);
}


void gb_mbc6_save_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    memcpy((uint8_t*)snapshot + sizeof(gb_snapshot_t) + cartridge->ram_length,mbc6,sizeof(gb_mbc6_t));
}

void gb_mbc6_load_state(gb_cartridge_t* cartridge,gb_snapshot_t* snapshot){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    memcpy(mbc6,(uint8_t*)snapshot + sizeof(gb_snapshot_t) + cartridge->ram_length,sizeof(gb_mbc6_t));

    if(cartridge->ram_length > 0x00){
        if(mbc6->state.ram_enabled){
            cartridge->ram_descriptor.write = gb_mbc6_write_ram;
            cartridge->ram_descriptor.read = gb_mbc6_read_ram;
        }
        else{
            cartridge->ram_descriptor.write = gb_memory_write_empty;
            cartridge->ram_descriptor.read = gb_memory_read_empty;
        }
    }

    if(mbc6->state.flash_enabled){
        cartridge->rom1_descriptor.write = gb_mbc6_write_flash;
    }
    else{
        cartridge->rom1_descriptor.write = gb_memory_write_empty;
    }

    gb_mbc6_update_mapping(cartridge);
}