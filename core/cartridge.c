#include "cartridge.h"
#include "gb.h"

static const uint8_t nintendo_logo[0x30] = {
    0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,
    0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,
    0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E
};


void gb_cartridge_init(gb_cartridge_t* cartridge,gb_t* gb){
    cartridge->gb = gb;

    cartridge->rom0_handler = (gb_memory_handler_t){
        gb_memory_write_empty,
        gb_cartridge_read_rom0,
        cartridge
    };

    cartridge->rom1_handler = (gb_memory_handler_t){
        gb_memory_write_empty,
        gb_cartridge_read_rom1,
        cartridge
    };

    cartridge->ram_handler = (gb_memory_handler_t){
        gb_memory_write_empty,
        gb_memory_read_empty,
        cartridge
    };
}


bool gb_cartridge_load(gb_cartridge_t* cartridge,const char* path){

    FILE* file = fopen(path,"rb");
    if(!file){
        gb_printf_errno(fopen);
        goto fail;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    if((size < gb_cartridge_rom_min_size) || (size > gb_cartridge_rom_max_size)){
        gb_printf_error("invalid rom");
        goto fail;
    }

    cartridge->rom = (uint8_t*)malloc(size);
    if(!cartridge->rom){
        gb_printf_errno(malloc);
        goto fail;
    }

    fread(cartridge->rom,1,size,file);

    uint8_t* singlecard_header = cartridge->rom + 0x100;
    size_t singlecard_rom_size = gb_cartridge_get_rom_size(singlecard_header);

    if(gb_cartridge_verify_nintendo_logo(singlecard_header) && gb_cartridge_verify_header_checksum(singlecard_header) && (singlecard_rom_size == size)){
        
        cartridge->header = singlecard_header;

        cartridge->rom_size = singlecard_rom_size;
        
        cartridge->rom_bank_mask = (singlecard_rom_size >> 0x0E) - 0x01;
    }
    else{
        uint8_t* multicard_header = cartridge->rom + (size - 0x8000) + 0x100;
        size_t multicard_rom_size = gb_cartridge_get_rom_size(multicard_header);

        if(gb_cartridge_verify_nintendo_logo(multicard_header) && gb_cartridge_verify_header_checksum(multicard_header) && (multicard_rom_size == size)){
            
            cartridge->header = multicard_header;
            
            cartridge->rom_size = multicard_rom_size;
            
            cartridge->rom_bank_mask = (multicard_rom_size >> 0x0E) - 0x01;
        }
        else{
            gb_printf_error("invalid rom");
            goto fail;
        }
    }

    if(!gb_cartridge_init_mapper(cartridge)){
        gb_printf_error("invalid rom mapper");
        goto fail;
    }

    cartridge->rom_crc32 = crc32(cartridge->rom,cartridge->rom_size);
    
    fclose(file);
    return true;

    fail:
    gb_cartridge_clear(cartridge);

    fclose(file);
    
    return false;
}


void gb_cartridge_save_ram(gb_cartridge_t* cartridge,const char* path){
    if(!cartridge->ram_size || !cartridge->ram_has_battery) return;
    gb_save_file(path,cartridge->ram,cartridge->ram_size);
}

void gb_cartridge_load_ram(gb_cartridge_t* cartridge,const char* path){
    if(!cartridge->ram_size || !cartridge->ram_has_battery) return;

    FILE* file = fopen(path,"rb");
    if(!file){
        gb_printf_errno(fopen);
        goto end;    
    }

    fseek(file,0,SEEK_END);
    size_t len = ftell(file);
    fseek(file,0,SEEK_SET);

    if(len != cartridge->ram_size) goto end;

    fread(cartridge->ram,1,cartridge->ram_size,file);

    end:
    if(file != NULL) fclose(file);
}


bool gb_cartridge_verify_nintendo_logo(uint8_t* header){

    if(!memcmp(header + 0x04,nintendo_logo,sizeof(nintendo_logo))){
        return true;
    }

    return false;
}

bool gb_cartridge_verify_header_checksum(uint8_t* header){
    uint8_t checksum = 0x00;
    
    for (uint16_t address = 0x34; address <= 0x4C; address++) {
        checksum = checksum - header[address] - 1;
    }

    return checksum == header[0x4D];
}


size_t gb_cartridge_get_rom_size(uint8_t* header){

    size_t rom_size = 0x00;

    switch(header[0x48]){
        //32KB
        case 0x00: rom_size = 0x8000; break;
        //64KB
        case 0x01: rom_size = 0x10000; break;
        //128KB
        case 0x02: rom_size = 0x20000; break;
        //256KB
        case 0x03: rom_size = 0x40000; break;
        //512KB
        case 0x04: rom_size = 0x80000; break;
        //1MB
        case 0x05: rom_size = 0x100000; break;
        //2MB
        case 0x06: rom_size = 0x200000; break;
        //4MB 
        case 0x07: rom_size = 0x400000; break;
        //8MB 
        case 0x08: rom_size = 0x800000; break;
    }

    return rom_size;
}


bool gb_cartridge_init_ram(gb_cartridge_t* cartridge,bool battery){

    switch(cartridge->header[0x49]){
        //2KB
        case 0x01:
            cartridge->ram_size = 0x800;
            cartridge->ram_bank_mask = 0x00;
            break;
        //8KB
        case 0x02:
            cartridge->ram_size = 0x2000;
            cartridge->ram_bank_mask = 0x00;
            break;
        //32KB
        case 0x03:
            cartridge->ram_size = 0x8000;
            cartridge->ram_bank_mask = 0x03;
            break;
        //128KB
        case 0x04:
            cartridge->ram_size = 0x20000;
            cartridge->ram_bank_mask = 0x0F;
            break;
        //64KB
        case 0x05:
            cartridge->ram_size = 0x10000;
            cartridge->ram_bank_mask = 0x07;
            break;
    }

    if(cartridge->ram_size > 0x00){
        
        cartridge->ram = (uint8_t*)malloc(cartridge->ram_size);

        if(!cartridge->ram){
            gb_printf_errno(malloc);
            return false;
        }

        memset(cartridge->ram,0x00,cartridge->ram_size);

        if(cartridge->ram_size == 0x800){
            cartridge->ram_address_mask = 0x07FF;
        }
        else{
            cartridge->ram_address_mask = 0x1FFF;
        }
        
        cartridge->ram_has_battery = battery;

        return true;
    }

    return false;
}

bool gb_cartridge_init_mapper(gb_cartridge_t* cartridge){

    switch(cartridge->header[0x47]){
        //ROM ONLY
        case 0x00: return gb_no_mbc_init(cartridge,0x00);
        //MBC1
        case 0x01: return gb_mbc1_init(cartridge,0x00);
        //MBC1+RAM
        case 0x02: return gb_mbc1_init(cartridge,gb_cartridge_ram);
        //MBC1+RAM+BATTERY
        case 0x03: return gb_mbc1_init(cartridge,gb_cartridge_ram | gb_cartridge_battery);
        //MBC2
        case 0x05: return gb_mbc2_init(cartridge,0x00);
        //MBC2+BATTERY
        case 0x06: return gb_mbc2_init(cartridge,gb_cartridge_battery);
        //ROM+RAM
        case 0x08: return gb_no_mbc_init(cartridge,gb_cartridge_ram);
        //ROM+RAM+BATTERY
        case 0x09: return gb_no_mbc_init(cartridge,gb_cartridge_ram | gb_cartridge_battery);
        //MMM01
        case 0x0B: return gb_mmm01_init(cartridge,0x00);
        //MMM01+BATTERY
        case 0x0C: return gb_mmm01_init(cartridge,gb_cartridge_battery);
        //MMM01+RAM+BATTERY
        case 0x0D: return gb_mmm01_init(cartridge,gb_cartridge_ram | gb_cartridge_battery);
        //MBC3+TIMER+BATTERY
        case 0x0F: return gb_mbc3_init(cartridge,gb_cartridge_rtc | gb_cartridge_battery);
        //MBC3+TIMER+RAM+BATTERY
        case 0x10: return gb_mbc3_init(cartridge,gb_cartridge_rtc | gb_cartridge_ram | gb_cartridge_battery);
        //MBC3
        case 0x11: return gb_mbc3_init(cartridge,0x00);
        //MBC3+RAM
        case 0x12: return gb_mbc3_init(cartridge,gb_cartridge_ram);
        //MBC3+RAM+BATTERY
        case 0x13: return gb_mbc3_init(cartridge,gb_cartridge_ram | gb_cartridge_battery);
        //MBC5
        case 0x19: return gb_mbc5_init(cartridge,0x00);
        //MBC5+RAM
        case 0x1A: return gb_mbc5_init(cartridge,gb_cartridge_ram);
        //MBC5+RAM+BATTERY
        case 0x1B: return gb_mbc5_init(cartridge,gb_cartridge_ram | gb_cartridge_battery);
        //MBC5+RUMBLE
        case 0x1C: return gb_mbc5_init(cartridge,gb_cartridge_rumble);
        //MBC5+RUMBLE+RAM
        case 0x1D: return gb_mbc5_init(cartridge,gb_cartridge_rumble | gb_cartridge_ram);
        //MBC5+RUMBLE+RAM+BATTERY
        case 0x1E: return gb_mbc5_init(cartridge,gb_cartridge_rumble | gb_cartridge_ram | gb_cartridge_battery);
        //MBC6+RAM+BATTERY
        case 0x20: return gb_mbc6_init(cartridge,gb_cartridge_ram | gb_cartridge_battery);
        //MBC7+SENSOR+BATTERY
        case 0x22: return gb_mbc7_init(cartridge,gb_cartridge_sensor | gb_cartridge_battery);
        //POCKET CAMERA
        case 0xFC: break;
        //BANDAI TAMA5
        case 0xFD: break;
        //HuC3
        case 0xFE: break;
        //HuC1+RAM+BATTERY
        case 0xFF: return gb_huc1_init(cartridge,gb_cartridge_ram | gb_cartridge_battery);
    }

    return false;
}


bool gb_no_mbc_init(gb_cartridge_t* cartridge,uint8_t flags){
    
    printf("Mapper: NoMBC\n");

    gb_cartridge_set_rom0_bank(cartridge,0x00);
    gb_cartridge_set_rom1_bank(cartridge,0x01);
    
    if(flags & gb_cartridge_ram){
        
        if(!gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery)){
            return false;
        }

        if(cartridge->ram_size > 0x00){
            
            gb_cartridge_set_ram_bank(cartridge,0x00);

            cartridge->ram_handler.write = gb_cartridge_write_ram;
            cartridge->ram_handler.read = gb_cartridge_read_ram;
        }
    }

    return true;
}


uint8_t gb_cartridge_read_rom0(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    return cartridge->rom0_ptr[address & 0x3FFF];
}

uint8_t gb_cartridge_read_rom1(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    return cartridge->rom1_ptr[address & 0x3FFF];
}


void gb_cartridge_write_ram(void* data,uint8_t value,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    cartridge->ram_ptr[address & cartridge->ram_address_mask] = value;
}

uint8_t gb_cartridge_read_ram(void* data,uint16_t address){
    gb_cartridge_t* cartridge = (gb_cartridge_t*)data;
    return cartridge->ram_ptr[address & cartridge->ram_address_mask];
}


void gb_cartridge_map(gb_cartridge_t* cartridge){
    gb_memory_t* memory = &cartridge->gb->memory;
    gb_memory_map_in_range(memory,&cartridge->rom0_handler,0x0000,0x3FFF);
    gb_memory_map_in_range(memory,&cartridge->rom1_handler,0x4000,0x7FFF);
    gb_memory_map_in_range(memory,&cartridge->ram_handler,0xA000,0xBFFF); 
}


void gb_cartridge_update_rtc_timer(gb_cartridge_t* cartridge){
    if(cartridge->mapper.rtc_update_timer != NULL){
        cartridge->mapper.rtc_update_timer(cartridge);
    }
}

void gb_cartridge_save_rtc(gb_cartridge_t* cartridge,const char* path){
    if(cartridge->mapper.rtc_save != NULL){
        cartridge->mapper.rtc_save(cartridge,path);
    }
}

void gb_cartridge_load_rtc(gb_cartridge_t* cartridge,const char* path){
    if(cartridge->mapper.rtc_load != NULL){
        cartridge->mapper.rtc_load(cartridge,path);
    }
}

void gb_cartridge_reset(gb_cartridge_t* cartridge){
    if(cartridge->mapper.reset != NULL){
        cartridge->mapper.reset(cartridge);
    }    
}


void gb_cartridge_save_state(gb_cartridge_t* cartridge,gb_state_t* state){
    if(cartridge->ram_size > 0){
        gb_state_write_ex(state,cartridge->ram,cartridge->ram_size);
    }
    if(cartridge->mapper.save_state != NULL){
        cartridge->mapper.save_state(cartridge,state);
    }
}

void gb_cartridge_load_state(gb_cartridge_t* cartridge,gb_state_t* state){
    if(cartridge->ram_size > 0){
        gb_state_read_ex(state,cartridge->ram,cartridge->ram_size);
    }
    if(cartridge->mapper.load_state != NULL){
        cartridge->mapper.load_state(cartridge,state);
    }
}


void gb_cartridge_clear(gb_cartridge_t* cartridge){
    
    if(cartridge->rom != NULL){
        free(cartridge->rom);
        cartridge->rom = NULL;
    }

    cartridge->rom_size = 0x00;

    cartridge->rom_crc32 = 0x00;
    
    cartridge->rom0_handler.write = gb_memory_write_empty;
    cartridge->rom0_handler.read = gb_cartridge_read_rom0;
    
    cartridge->rom1_handler.write = gb_memory_write_empty;
    cartridge->rom1_handler.read = gb_cartridge_read_rom1;

    cartridge->rom0_ptr = NULL;
    cartridge->rom1_ptr = NULL;
    
    cartridge->rom_bank_mask = 0x00;

    if(cartridge->ram != NULL){
        free(cartridge->ram);
        cartridge->ram = NULL;
    }

    cartridge->ram_size = 0x00;
    cartridge->ram_handler.write = gb_memory_write_empty;
    cartridge->ram_handler.read = gb_memory_read_empty;
    cartridge->ram_ptr = NULL;
    cartridge->ram_bank_mask = 0x00;
    cartridge->ram_address_mask = 0x00;
    cartridge->ram_has_battery = false;


    if(cartridge->mapper.data != NULL){
        free(cartridge->mapper.data);
        cartridge->mapper.data = NULL;
    }
    cartridge->mapper.rtc_update_timer = NULL;
    cartridge->mapper.rtc_save = NULL;
    cartridge->mapper.rtc_load = NULL;
    cartridge->mapper.reset = NULL;
}