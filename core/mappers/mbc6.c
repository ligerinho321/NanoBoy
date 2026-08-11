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

    mbc6->rom_bank_mask = (cartridge->rom_size / 0x2000) - 0x01;
    mbc6->ram_bank_mask = 0x00;

    cartridge->mapper.data = mbc6;
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

        if(cartridge->ram_size > 0x1000){
            mbc6->ram_bank_mask = (cartridge->ram_size / 0x1000) - 0x01;
        }
    }

    return true;
}


static void gb_mbc6_update_mapping(gb_cartridge_t* cartridge){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    if(cartridge->ram_size > 0){

        mbc6->ram_0_ptr = cartridge->ram + ((mbc6->ram_bank_0 & mbc6->ram_bank_mask) << 0x0C);

        mbc6->ram_1_ptr = cartridge->ram + ((mbc6->ram_bank_1 & mbc6->ram_bank_mask) << 0x0C);
    }

    if(mbc6->flash_bank_0_enabled){
        mbc6->rom_or_flash_0_ptr = mbc6->flash_data + ((mbc6->rom_or_flash_bank_0 & gb_mbc6_flash_bank_mask) << 0x0D);
    }
    else{
        mbc6->rom_or_flash_0_ptr = cartridge->rom + ((mbc6->rom_or_flash_bank_0 & mbc6->rom_bank_mask) << 0x0D);
    }

    if(mbc6->flash_bank_1_enabled){
        mbc6->rom_or_flash_1_ptr = mbc6->flash_data + ((mbc6->rom_or_flash_bank_1 & gb_mbc6_flash_bank_mask) << 0x0D);
    }
    else{
        mbc6->rom_or_flash_1_ptr = cartridge->rom + ((mbc6->rom_or_flash_bank_1 & mbc6->rom_bank_mask) << 0x0D);
    }
}


void gb_mbc6_write_register(void* data,uint8_t value,uint16_t address){

    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    //0x0000-0x03FF
    if(address < 0x0400){
        mbc6->ram_enabled = (value & 0x0F) == 0x0A;

        if(cartridge->ram_size > 0x00){
            if(mbc6->ram_enabled){
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
        mbc6->ram_bank_0 = value & 0x07;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x0800-0x0BFF
    else if(address < 0x0C00){
        mbc6->ram_bank_1 = value & 0x07;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x0C00-0x0FFF
    else if(address < 0x1000){
        
        mbc6->flash_enabled = value & 0x01;

        if(mbc6->flash_enabled){
            cartridge->rom1_descriptor.write = gb_mbc6_write_flash;
        }
        else{
            cartridge->rom1_descriptor.write = gb_memory_write_empty;
        }
    }
    //0x1000-0x1FFF
    else if(address < 0x2000){
        mbc6->flash_write_enabled = value & 0x01;
    }
    //0x2000-0x27FF
    else if(address < 0x2800){
        mbc6->rom_or_flash_bank_0 = value & 0x7F;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x2800-0x2FFF
    else if(address < 0x3000){
        mbc6->flash_bank_0_enabled = value == 0x08;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x3000-0x37FF
    else if(address < 0x3800){
        mbc6->rom_or_flash_bank_1 = value & 0x7F;
        gb_mbc6_update_mapping(cartridge);
    }
    //0x3800-0x3FFF
    else{
        mbc6->flash_bank_1_enabled = value == 0x08;
        gb_mbc6_update_mapping(cartridge);
    }
}


void gb_mbc6_write_flash(void* data,uint8_t value,uint16_t address){
    
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;

    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    if(!((address < 0x6000) ? mbc6->flash_bank_0_enabled : mbc6->flash_bank_1_enabled)) return;

    uint8_t bank = (address < 0x6000) ? mbc6->rom_or_flash_bank_0 : mbc6->rom_or_flash_bank_1;

    uint32_t chip_address = (bank << 0x0D) | (address & 0x1FFF);

    switch(mbc6->flash_command){
        case gb_mbc6_flash_program_flash_command:{
            
            uint8_t flash_buffer_address = chip_address & 0x7F;

            if(flash_buffer_address == mbc6->flash_buffer_last_write_address){
                
                mbc6->flash_command = 0x00;

                uint8_t sector = chip_address >> 0x11;

                if(!((sector > 0x00) || (mbc6->flash_write_enabled && !mbc6->flash_protect_sector_0))) return;

                if(value == gb_mbc6_flash_reset_command) return;

                uint8_t* src = mbc6->flash_buffer;

                uint8_t* dst = mbc6->flash_data + (chip_address & ~0x7F);
                
                uint8_t* end = dst + sizeof(mbc6->flash_buffer);

                while(dst < end) *dst++ &= *src++;
            }
            else{
                mbc6->flash_buffer[flash_buffer_address] = value;
                mbc6->flash_buffer_last_write_address = flash_buffer_address;
            }

            return;
        }
        case gb_mbc6_flash_program_map_command:{

            uint8_t flash_buffer_address = chip_address & 0x7F;

            if(flash_buffer_address == mbc6->flash_buffer_last_write_address){
                
                mbc6->flash_command = 0x00;

                if(value == gb_mbc6_flash_reset_command) return;

                uint8_t* src = mbc6->flash_buffer;

                uint8_t* dst = mbc6->flash_map_data + (chip_address & 0x80);
                
                uint8_t* end = dst + sizeof(mbc6->flash_buffer);

                while(dst < end) *dst++ &= *src++;

            }
            else{
                mbc6->flash_buffer[flash_buffer_address] = value;
                mbc6->flash_buffer_last_write_address = flash_buffer_address;
            }

            return;
        }
    }

    if(value == gb_mbc6_flash_reset_command){
        mbc6->flash_pre_command = 0x00;
        mbc6->flash_command = 0x00;
        mbc6->flash_state = 0x00;
        return;
    }

    switch(mbc6->flash_state){
        case 0x00:{
            if(chip_address == 0x5555 && value == 0xAA){
                mbc6->flash_state = 0x01;
            }
            else{
                mbc6->flash_pre_command = 0x00;
            }
            break;
        }
        case 0x01:{
            if(chip_address == 0x2AAA && value == 0x55){
                mbc6->flash_state = mbc6->flash_pre_command ? 0x03 : 0x02;
            }
            else{
                mbc6->flash_state = 0x00;
                mbc6->flash_pre_command = 0x00;
            }
            break;
        }
        case 0x02:{
            if(chip_address == 0x5555){
                switch(value){
                    case gb_mbc6_flash_read_id_command:{
                        mbc6->flash_command = gb_mbc6_flash_read_id_command;
                        break;
                    }
                    case gb_mbc6_flash_program_flash_command:{
                        mbc6->flash_command = gb_mbc6_flash_program_flash_command;
                        mbc6->flash_buffer_last_write_address = 0xFF;
                        memset(mbc6->flash_buffer,0xFF,sizeof(mbc6->flash_buffer));
                        break;
                    }
                    case 0x60:{
                        mbc6->flash_pre_command = 0x60;
                        break;
                    }
                    case 0x77:{
                        mbc6->flash_pre_command = 0x77;
                        break;
                    }
                    case 0x80:{
                        mbc6->flash_pre_command = 0x80;
                        break;
                    }
                }
            }
            mbc6->flash_state = 0x00;
            break;
        }
        case 0x03:{
            switch(mbc6->flash_pre_command){
                case 0x60:{
                    switch(value){
                        case gb_mbc6_flash_erase_map_command:{
                            if(chip_address == 0x5555 && mbc6->flash_write_enabled){
                                mbc6->flash_command = gb_mbc6_flash_erase_map_command;
                                memset(mbc6->flash_map_data,0xFF,sizeof(mbc6->flash_map_data));
                            }
                            break;
                        }
                        case gb_mbc6_flash_protect_sector_0_command:{
                            if(!mbc6->flash_protect_sector_0 && bank < 0x10){
                                mbc6->flash_protect_sector_0 = true;
                                mbc6->flash_command = gb_mbc6_flash_protect_sector_0_command;
                            }
                            break;
                        }
                        case gb_mbc6_flash_unprotect_sector_0_command:{
                            if(mbc6->flash_protect_sector_0 && bank < 0x10){
                                mbc6->flash_protect_sector_0 = false;
                                mbc6->flash_command = gb_mbc6_flash_unprotect_sector_0_command;
                            }
                            break;
                        }
                        case gb_mbc6_flash_program_map_command:{
                            if(chip_address == 0x5555 && mbc6->flash_write_enabled){
                                mbc6->flash_command = gb_mbc6_flash_program_map_command;
                                mbc6->flash_buffer_last_write_address = 0xFF;
                                memset(mbc6->flash_buffer,0xFF,sizeof(mbc6->flash_buffer));
                            }
                            break;
                        }
                    }
                    break;
                }
                case 0x77:{
                    if(value == gb_mbc6_flash_read_map_command && chip_address == 0x5555){
                        mbc6->flash_command = gb_mbc6_flash_read_map_command;
                    }
                    break;
                }
                case 0x80:{
                    switch(value){
                        case gb_mbc6_flash_mass_erase_flash_command:{
                            if(chip_address == 0x5555){
                                
                                mbc6->flash_command = gb_mbc6_flash_mass_erase_flash_command;

                                if(mbc6->flash_write_enabled && !mbc6->flash_protect_sector_0){
                                    memset(mbc6->flash_data,0xFF,0x20000);
                                }
                                
                                memset(mbc6->flash_data + 0x20000,0xFF,0xE0000);
                            }
                            break;
                        }
                        case gb_mbc6_flash_erase_flash_sector_command:{
                            mbc6->flash_command = gb_mbc6_flash_erase_flash_sector_command;

                            uint8_t sector = chip_address >> 0x11;

                            if((sector > 0x00) || (mbc6->flash_write_enabled && !mbc6->flash_protect_sector_0)){
                                memset(mbc6->flash_data + (sector << 0x11),0xFF,0x20000);
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            
            mbc6->flash_pre_command = 0x00;

            mbc6->flash_state = 0x00;

            break;
        }
        default:{
            mbc6->flash_state = 0x00;
            break;
        }
    }
}

uint8_t gb_mbc6_read_rom_or_flash(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    uint8_t value = 0xFF;

    if(!(mbc6->flash_command && ((address < 0x6000) ? mbc6->flash_bank_0_enabled : mbc6->flash_bank_1_enabled))){
        
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

    switch(mbc6->flash_command){
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
                    uint8_t bank = (address < 0x6000) ? mbc6->rom_or_flash_bank_0 : mbc6->rom_or_flash_bank_1;
                    value = (bank < 0x10) ? 0xC2 : 0x00;
                }
                case 0x03:{
                    value = 0xFF;
                    break;
                }
            }
            break;
        }
        case gb_mbc6_flash_read_map_command:{
            value = mbc6->flash_map_data[address & 0xFF];
            break;
        }
        case gb_mbc6_flash_program_flash_command:
        case gb_mbc6_flash_erase_map_command:
        case gb_mbc6_flash_protect_sector_0_command:
        case gb_mbc6_flash_unprotect_sector_0_command:
        case gb_mbc6_flash_program_map_command:
        case gb_mbc6_flash_mass_erase_flash_command:
        case gb_mbc6_flash_erase_flash_sector_command:{
            value = 0x80 | (mbc6->flash_protect_sector_0 ? 0x02 : 0x00);
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
        mbc6->ram_0_ptr[address & 0x0FFF] = value;
    }
    //0xB000-0xBFFF
    else{
        mbc6->ram_1_ptr[address & 0x0FFF] = value;
    }
}

uint8_t gb_mbc6_read_ram(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;
    //0xA000-0xAFFF
    if(address < 0xB000){
        return mbc6->ram_0_ptr[address & 0x0FFF];
    }
    //0xB000-0xBFFF
    else{
        return mbc6->ram_1_ptr[address & 0x0FFF];
    }
}


void gb_mbc6_reset(gb_cartridge_t* cartridge){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    mbc6->ram_enabled = false;

    if(cartridge->ram_size > 0x00){
        cartridge->ram_descriptor.write = gb_memory_write_empty;
        cartridge->ram_descriptor.read = gb_memory_read_empty;
    }

    mbc6->flash_enabled = false;

    cartridge->rom1_descriptor.write = gb_memory_write_empty;

    mbc6->flash_write_enabled = false;

    mbc6->flash_bank_0_enabled = false;
    mbc6->flash_bank_1_enabled = false;

    mbc6->flash_protect_sector_0 = false;

    mbc6->flash_state = 0x00;
    mbc6->flash_pre_command = 0x00;
    mbc6->flash_command = 0x00;

    mbc6->rom_or_flash_bank_0 = 0x02;
    mbc6->rom_or_flash_bank_1 = 0x03;

    mbc6->ram_bank_0 = 0x00;
    mbc6->ram_bank_1 = 0x01;
    
    gb_mbc6_update_mapping(cartridge);
}


void gb_mbc6_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    gb_state_write(state,mbc6->ram_enabled);

    gb_state_write(state,mbc6->flash_enabled);
    gb_state_write(state,mbc6->flash_write_enabled);

    gb_state_write(state,mbc6->flash_bank_0_enabled);
    gb_state_write(state,mbc6->flash_bank_1_enabled);

    gb_state_write(state,mbc6->flash_protect_sector_0);

    gb_state_write(state,mbc6->flash_state);
    gb_state_write(state,mbc6->flash_pre_command);
    gb_state_write(state,mbc6->flash_command);

    gb_state_write(state,mbc6->rom_or_flash_bank_0);
    gb_state_write(state,mbc6->rom_or_flash_bank_1);

    gb_state_write(state,mbc6->ram_bank_0);
    gb_state_write(state,mbc6->ram_bank_1);

    gb_state_write_ex(state,mbc6->flash_data,sizeof(mbc6->flash_data));

    gb_state_write_ex(state,mbc6->flash_map_data,sizeof(mbc6->flash_map_data));

    gb_state_write_ex(state,mbc6->flash_buffer,sizeof(mbc6->flash_buffer));
    gb_state_write(state,mbc6->flash_buffer_last_write_address);
}

void gb_mbc6_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    gb_mbc6_t* mbc6 = (gb_mbc6_t*)cartridge->mapper.data;

    gb_state_read(state,mbc6->ram_enabled);

    if(cartridge->ram_size > 0x00){
        if(mbc6->ram_enabled){
            cartridge->ram_descriptor.write = gb_mbc6_write_ram;
            cartridge->ram_descriptor.read = gb_mbc6_read_ram;
        }
        else{
            cartridge->ram_descriptor.write = gb_memory_write_empty;
            cartridge->ram_descriptor.read = gb_memory_read_empty;
        }
    }

    gb_state_read(state,mbc6->flash_enabled);

    if(mbc6->flash_enabled){
        cartridge->rom1_descriptor.write = gb_mbc6_write_flash;
    }
    else{
        cartridge->rom1_descriptor.write = gb_memory_write_empty;
    }

    gb_state_read(state,mbc6->flash_write_enabled);

    gb_state_read(state,mbc6->flash_bank_0_enabled);
    gb_state_read(state,mbc6->flash_bank_1_enabled);

    gb_state_read(state,mbc6->flash_protect_sector_0);

    gb_state_read(state,mbc6->flash_state);
    gb_state_read(state,mbc6->flash_pre_command);
    gb_state_read(state,mbc6->flash_command);

    gb_state_read(state,mbc6->rom_or_flash_bank_0);
    gb_state_read(state,mbc6->rom_or_flash_bank_1);

    gb_state_read(state,mbc6->ram_bank_0);
    gb_state_read(state,mbc6->ram_bank_1);

    gb_mbc6_update_mapping(cartridge);

    gb_state_read_ex(state,mbc6->flash_data,sizeof(mbc6->flash_data));

    gb_state_read_ex(state,mbc6->flash_map_data,sizeof(mbc6->flash_map_data));

    gb_state_read_ex(state,mbc6->flash_buffer,sizeof(mbc6->flash_buffer));
    gb_state_read(state,mbc6->flash_buffer_last_write_address);
}