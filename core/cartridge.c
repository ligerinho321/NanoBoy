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
        return false;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    if((size < gb_cartridge_rom_min_size) || (size > gb_cartridge_rom_max_size)){
        gb_printf_error("invalid rom");
        goto fail;
    }

    fread(cartridge->rom,1,size,file);

    uint8_t* mmm01_header = cartridge->rom + (size - 0x8000) + 0x100;
    uint8_t mapper = mmm01_header[0x47];

    uint8_t* default_header = cartridge->rom + 0x100;

    if(
        (mapper == 0x0B || mapper == 0x0C || mapper == 0x0D) &&
        gb_cartridge_verify_nintendo_logo(mmm01_header) &&
        gb_cartridge_verify_header_checksum(mmm01_header)
    ){
        cartridge->header = mmm01_header;
    }
    else if(
        gb_cartridge_verify_nintendo_logo(default_header) &&
        gb_cartridge_verify_header_checksum(default_header)
    ){
        cartridge->header = default_header;
    }
    else{
        gb_printf_error("invalid rom");
        goto fail;
    }

    gb_cartridge_init_rom(cartridge);

    if(cartridge->rom_size != size){
        gb_printf_error("invalid rom");
        goto fail;
    }

    if(!gb_cartridge_init_mapper(cartridge)){
        gb_printf_error("invalid rom mapper");
        goto fail;
    }

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


void gb_cartridge_init_rom(gb_cartridge_t* cartridge){

    switch(cartridge->header[0x48]){
        //32KB
        case 0x00:
            cartridge->rom_size = 0x8000;
            cartridge->rom_bank_mask = 0x01;
            break;
        //64KB
        case 0x01:
            cartridge->rom_size = 0x10000;
            cartridge->rom_bank_mask = 0x03;
            break;
        //128KB
        case 0x02:
            cartridge->rom_size = 0x20000;
            cartridge->rom_bank_mask = 0x07;
            break;
        //256KB
        case 0x03:
            cartridge->rom_size = 0x40000;
            cartridge->rom_bank_mask = 0x0F;
            break;
        //512KB
        case 0x04:
            cartridge->rom_size = 0x80000;
            cartridge->rom_bank_mask = 0x1F;
            break;
        //1MB
        case 0x05:
            cartridge->rom_size = 0x100000;
            cartridge->rom_bank_mask = 0x3F;
            break;
        //2MB
        case 0x06:
            cartridge->rom_size = 0x200000;
            cartridge->rom_bank_mask = 0x7F;
            break;
        //4MB 
        case 0x07:
            cartridge->rom_size = 0x400000;
            cartridge->rom_bank_mask = 0xFF;
            break;
        //8MB 
        case 0x08:
            cartridge->rom_size = 0x800000;
            cartridge->rom_bank_mask = 0x1FF;
            break;
    }
}

void gb_cartridge_init_ram(gb_cartridge_t* cartridge,bool battery){

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

    if(cartridge->ram_size){
        
        memset(cartridge->ram,0,cartridge->ram_size);

        if(cartridge->ram_size == 0x800){
            cartridge->ram_address_mask = 0x07FF;
        }
        else{
            cartridge->ram_address_mask = 0x1FFF;
        }
        
        cartridge->ram_has_battery = battery;
    }
}

bool gb_cartridge_init_mapper(gb_cartridge_t* cartridge){

    bool result = true;

    switch(cartridge->header[0x47]){
        //ROM ONLY
        case 0x00: gb_no_mbc_init(cartridge,0x00); break;
        //MBC1
        case 0x01: gb_mbc1_init(cartridge,0x00); break;
        //MBC1+RAM
        case 0x02: gb_mbc1_init(cartridge,gb_cartridge_ram); break;
        //MBC1+RAM+BATTERY
        case 0x03: gb_mbc1_init(cartridge,gb_cartridge_ram | gb_cartridge_battery); break;
        //MBC2
        case 0x05: gb_mbc2_init(cartridge,0x00); break;
        //MBC2+BATTERY
        case 0x06: gb_mbc2_init(cartridge,gb_cartridge_battery); break;
        //ROM+RAM
        case 0x08: gb_no_mbc_init(cartridge,gb_cartridge_ram); break;
        //ROM+RAM+BATTERY
        case 0x09: gb_no_mbc_init(cartridge,gb_cartridge_ram | gb_cartridge_battery); break;
        //MMM01
        case 0x0B: gb_mmm01_init(cartridge,0x00); break;
        //MMM01+BATTERY
        case 0x0C: gb_mmm01_init(cartridge,gb_cartridge_battery); break;
        //MMM01+RAM+BATTERY
        case 0x0D: gb_mmm01_init(cartridge,gb_cartridge_ram | gb_cartridge_battery); break;
        //MBC3+TIMER+BATTERY
        case 0x0F: gb_mbc3_init(cartridge,gb_cartridge_rtc | gb_cartridge_battery); break;
        //MBC3+TIMER+RAM+BATTERY
        case 0x10: gb_mbc3_init(cartridge,gb_cartridge_rtc | gb_cartridge_ram | gb_cartridge_battery); break;
        //MBC3
        case 0x11: gb_mbc3_init(cartridge,0x00); break;
        //MBC3+RAM
        case 0x12: gb_mbc3_init(cartridge,gb_cartridge_ram); break;
        //MBC3+RAM+BATTERY
        case 0x13: gb_mbc3_init(cartridge,gb_cartridge_ram | gb_cartridge_battery); break;
        //MBC5
        case 0x19: gb_mbc5_init(cartridge,0x00); break;
        //MBC5+RAM
        case 0x1A: gb_mbc5_init(cartridge,gb_cartridge_ram); break;
        //MBC5+RAM+BATTERY
        case 0x1B: gb_mbc5_init(cartridge,gb_cartridge_ram | gb_cartridge_battery); break;
        //MBC5+RUMBLE
        case 0x1C: gb_mbc5_init(cartridge,gb_cartridge_rumble); break;
        //MBC5+RUMBLE+RAM
        case 0x1D: gb_mbc5_init(cartridge,gb_cartridge_rumble | gb_cartridge_ram); break;
        //MBC5+RUMBLE+RAM+BATTERY
        case 0x1E: gb_mbc5_init(cartridge,gb_cartridge_rumble | gb_cartridge_ram | gb_cartridge_battery); break;
        //MBC6
        case 0x20: result = false; break;
        //MBC7+SENSOR+RUMBLE+RAM+BATTERY
        case 0x22: result = false; break;
        //POCKET CAMERA
        case 0xFC: result = false; break;
        //BANDAI TAMA5
        case 0xFD: result = false; break;
        //HuC3
        case 0xFE: result = false; break;
        //HuC1+RAM+BATTERY
        case 0xFF: result = false; break;
        
        default: result = false; break;
    }

    return result;
}


void gb_no_mbc_init(gb_cartridge_t* cartridge,uint8_t flags){
    
    printf("Mapper: NoMBC\n");

    gb_cartridge_set_rom0_bank(cartridge,0x00);
    gb_cartridge_set_rom1_bank(cartridge,0x01);
    
    if(flags & gb_cartridge_ram){
        
        gb_cartridge_init_ram(cartridge,flags & gb_cartridge_battery);

        if(cartridge->ram_size){
            
            gb_cartridge_set_ram_bank(cartridge,0x00);

            cartridge->ram_handler.write = gb_cartridge_write_ram;
            cartridge->ram_handler.read = gb_cartridge_read_ram;
        }
    }
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
    if(cartridge->rtc_update_timer != NULL){
        cartridge->rtc_update_timer(cartridge);
    }
}

void gb_cartridge_save_rtc(gb_cartridge_t* cartridge,const char* path){
    if(cartridge->rtc_save != NULL){
        cartridge->rtc_save(cartridge,path);
    }
}

void gb_cartridge_load_rtc(gb_cartridge_t* cartridge,const char* path){
    if(cartridge->rtc_load != NULL){
        cartridge->rtc_load(cartridge,path);
    }
}

void gb_cartridge_reset(gb_cartridge_t* cartridge){
    if(cartridge->reset != NULL){
        cartridge->reset(cartridge);
    }    
}


void gb_cartridge_clear(gb_cartridge_t* cartridge){
    cartridge->rom_size = 0x00;
    cartridge->rom0_handler.write = gb_memory_write_empty;
    cartridge->rom1_handler.write = gb_memory_write_empty;
    cartridge->rom0_ptr = NULL;
    cartridge->rom1_ptr = NULL;
    cartridge->rom_bank_mask = 0x00;

    cartridge->ram_size = 0x00;
    cartridge->ram_handler.write = gb_memory_write_empty;
    cartridge->ram_handler.read = gb_memory_read_empty;
    cartridge->ram_ptr = NULL;
    cartridge->ram_bank_mask = 0x00;
    cartridge->ram_address_mask = 0x00;
    cartridge->ram_has_battery = false;

    cartridge->rtc_update_timer = NULL;
    cartridge->rtc_save = NULL;
    cartridge->rtc_load = NULL;
    cartridge->reset = NULL;
}