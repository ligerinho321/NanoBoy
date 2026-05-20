#include "./cartridge.h"

bool gb_cartridge_load(gb_cartridge_t* cartridge,const char* path){
    FILE* file = fopen(path,"rb");
    if(!file){
        PRINTF_ERRNO(fopen);
        return false;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    if((size < GB_CARTRIDGE_ROM_MIN_SIZE) || (size > GB_CARTRIDGE_ROM_MAX_SIZE)){
        goto invalid_rom;
    }

    fread(cartridge->rom,sizeof(uint8_t),size,file);

    cartridge->rom_size = gb_cartridge_rom_size(cartridge);
    cartridge->ram_size = gb_cartridge_ram_size(cartridge);

    if(
        (cartridge->rom_size != size) || 
        !gb_cartridge_verify_header_checksum(cartridge) || 
        !gb_cartridge_verify_global_checksum(cartridge)
    ){
        goto invalid_rom;
    }

    fclose(file);
    return true;

    invalid_rom:

    PRINTF_ERROR("Invalid ROM");
    
    gb_cartridge_clear(cartridge);

    fclose(file);
    
    return false;
}

bool gb_cartridge_verify_header_checksum(gb_cartridge_t* cartridge){
    uint8_t checksum = 0x00;
    for (uint16_t address = 0x0134; address <= 0x014C; address++) {
        checksum = checksum - cartridge->rom[address] - 1;
    }
    return checksum == cartridge->rom[0x14D];
}

bool gb_cartridge_verify_global_checksum(gb_cartridge_t* cartridge){
    uint32_t checksum = 0x00;
    for(uint32_t i = 0; i < cartridge->rom_size; ++i){
        if(i == 0x14E || i == 0x14F) continue;
        checksum += cartridge->rom[i];
    }
    return (checksum & 0xFFFF) == ((cartridge->rom[0x14E] << 0x08) | cartridge->rom[0x14F]);
}

uint8_t gb_cartridge_mapper(gb_cartridge_t* cartridge){
    return cartridge->rom[0x147];
}

size_t gb_cartridge_rom_size(gb_cartridge_t* cartridge){
    size_t size = 0;
    switch(cartridge->rom[0x148]){
        case 0x00: size = 0x8000;   break; //32KB
        case 0x01: size = 0x10000;  break; //64KB
        case 0x02: size = 0x20000;  break; //128KB
        case 0x03: size = 0x40000;  break; //256KB
        case 0x04: size = 0x80000;  break; //512KB
        case 0x05: size = 0x100000; break; //1MB
        case 0x06: size = 0x200000; break; //2MB
        case 0x07: size = 0x400000; break; //4MB
        case 0x08: size = 0x800000; break; //8MB
    }
    return size;
}

size_t gb_cartridge_ram_size(gb_cartridge_t* cartridge){
    size_t size = 0;
    switch(cartridge->rom[0x149]){
        case 0x01: PRINTF_ERROR("Unused RAM size value 0x01"); break; //2KB
        case 0x02: size = 0x2000; break; //8KB
        case 0x03: size = 0x8000; break; //32KB
        case 0x04: size = 0x20000; break; //128KB
        case 0x05: size = 0x10000; break; //64KB
    }
    return size;
}

void gb_cartridge_clear(gb_cartridge_t* cartridge){
    cartridge->rom_size = 0x00;
    cartridge->ram_size = 0x00;
}