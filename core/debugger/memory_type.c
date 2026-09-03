#include "memory_type.h"
#include "../gb.h"

const char* gb_memory_type_names[7] = {
    "CPU Memory",
    "Cartridge ROM",
    "Video RAM",
    "Cartridge RAM",
    "Work RAM",
    "Object RAM",
    "High RAM",
};

uint8_t gb_memory_type(gb_t* gb,uint16_t address){
    //$0000-$7FFF
    if(address < 0x8000){
        if(!gb->boot.mapped){
            return gb_memory_rom_type;
        }
        else{
            //$0000-$00FF
            if(address < 0x0100){
                return -1;
            }
            //$0100-$01FF
            else if(address < 0x0200){
                return gb_memory_rom_type;
            }
            //$0200-$08FF
            else if(gb->is_cgb && address < 0x0900){
                return -1;
            }
            else{
                return gb_memory_rom_type;
            }
        }
    }
    //$8000-$9FFF
    else if(address < 0xA000){
        return gb_memory_vram_type;
    }
    //$A000-$BFFF
    else if(address < 0xC000){
        return gb_memory_ram_type;
    }
    //$C000-$FDFF
    else if(address < 0xFE00){
        return gb_memory_wram_type;
    }
    //$FE00-$FE9F
    else if(address < 0xFEA0){
        return gb_memory_oam_type;
    }
    //$FEA0-$FF7F
    else if(address < 0xFF80){
        return -1;
    }
    //$FF80-$FFFE
    else if(address < 0xFFFF){
        return gb_memory_hram_type;
    }
    //$FFFF
    else{
        return -1;
    }
}

size_t gb_memory_type_length(gb_t* gb,uint8_t memory_type){
    size_t length = 0;

    switch(memory_type){
        case gb_memory_cpu_type:
            length = gb_bus_length;
            break;
        case gb_memory_rom_type:
            length = gb->cartridge.rom_length;
            break;
        case gb_memory_vram_type:
            length = gb_vram_length;
            break;
        case gb_memory_ram_type:
            length = gb->cartridge.ram_length;
            break;
        case gb_memory_wram_type:
            length = gb_wram_length;
            break;
        case gb_memory_oam_type:
            length = gb_oam_length;
            break;
        case gb_memory_hram_type:
            length = gb_hram_length;
            break;
    }

    return length;
}

size_t gb_memory_type_absolute_address(gb_t* gb,uint8_t memory_type,uint16_t relative_address){
    size_t absolute_address = (size_t)-1;

    switch(memory_type){
        case gb_memory_cpu_type:{
            absolute_address = relative_address;
            break;
        }
        case gb_memory_rom_type:{
            absolute_address = gb_cartridge_rom_absolute_address(&gb->cartridge,relative_address);
            break;
        }
        case gb_memory_vram_type:{
            absolute_address = gb_ppu_vram_absolute_address(&gb->ppu,relative_address);
            break;
        }
        case gb_memory_ram_type:{
            absolute_address = gb_cartridge_ram_absolute_address(&gb->cartridge,relative_address);
            break;
        }
        case gb_memory_wram_type:{
            absolute_address = gb_memory_wram_absolute_address(&gb->memory,relative_address);
            break;
        }
        case gb_memory_oam_type:{
            absolute_address = gb_ppu_oam_absolute_address(&gb->ppu,relative_address);
            break;
        }
        case gb_memory_hram_type:{
            absolute_address = gb_memory_hram_absolute_address(&gb->memory,relative_address);
            break;
        }
    }

    return absolute_address;
}


void gb_memory_type_write_byte(gb_t* gb,uint8_t memory_type,uint8_t value,size_t address){
    switch(memory_type){
        case gb_memory_cpu_type:{
            gb_memory_cpu_write(&gb->memory,value,address);
            break;
        }
        case gb_memory_rom_type:{
            gb->cartridge.rom[address % gb->cartridge.rom_length] = value;
            break;
        }
        case gb_memory_vram_type:{
            gb->ppu.vram[address % gb_vram_length] = value;
            break;
        }
        case gb_memory_ram_type:{
            gb->cartridge.ram[address % gb->cartridge.ram_length] = value;
            break;
        }
        case gb_memory_wram_type:{
            gb->memory.wram[address % gb_wram_length] = value;
            break;
        }
        case gb_memory_oam_type:{
            gb->ppu.oam[address % gb_oam_length] = value;
            break;
        }
        case gb_memory_hram_type:{
            gb->memory.hram[address % gb_hram_length] = value;
            break;
        }
    }
}

uint8_t gb_memory_type_read_byte(gb_t* gb,uint8_t memory_type,size_t address){
    uint8_t byte = 0x00;

    switch(memory_type){
        case gb_memory_cpu_type:{
            byte = gb_memory_cpu_read(&gb->memory,address);
            break;
        }
        case gb_memory_rom_type:{
            byte = gb->cartridge.rom[address % gb->cartridge.rom_length];
            break;
        }
        case gb_memory_vram_type:{
            byte = gb->ppu.vram[address % gb_vram_length];
            break;
        }
        case gb_memory_ram_type:{
            byte = gb->cartridge.ram[address % gb->cartridge.ram_length];
            break;
        }
        case gb_memory_wram_type:{
            byte = gb->memory.wram[address % gb_wram_length];
            break;
        }
        case gb_memory_oam_type:{
            byte = gb->ppu.oam[address % gb_oam_length];
            break;
        }
        case gb_memory_hram_type:{
            byte = gb->memory.hram[address % gb_hram_length];
            break;
        }
    }

    return byte;
}


void gb_memory_type_write(gb_t* gb,uint8_t memory_type,size_t address,uint8_t* src,size_t len){
    switch(memory_type){
        case gb_memory_cpu_type:{
            gb_memory_t* memory = &gb->memory;
            while(len--) gb_memory_cpu_write(memory,*src++,address++ % gb_bus_length);
            break;
        }
        case gb_memory_rom_type:{
            uint8_t* rom = gb->cartridge.rom;
            while(len--) rom[address++ % gb->cartridge.rom_length] = *src++;
            break;
        }
        case gb_memory_vram_type:{
            uint8_t* vram = gb->ppu.vram;
            while(len--) vram[address++ % gb_vram_length] = *src++;
            break;
        }
        case gb_memory_ram_type:{
            uint8_t* ram = gb->cartridge.ram;
            while(len--) ram[address++ % gb->cartridge.ram_length] = *src++;
            break;
        }
        case gb_memory_wram_type:{
            uint8_t* wram = gb->memory.wram;
            while(len--) wram[address++ % gb_wram_length] = *src++;
            break;
        }
        case gb_memory_oam_type:{
            uint8_t* oam = gb->ppu.oam;
            while(len--) oam[address++ % gb_oam_length] = *src++;
            break;
        }
        case gb_memory_hram_type:{
            uint8_t* hram = gb->memory.hram;
            while(len--) hram[address++ % gb_hram_length] = *src++;
            break;
        }
    }
}

void gb_memory_type_read(gb_t* gb,uint8_t memory_type,size_t address,uint8_t* dst,size_t len){
    switch(memory_type){
        case gb_memory_cpu_type:{
            gb_memory_t* memory = &gb->memory;
            while(len--) *dst++ = gb_memory_cpu_read(memory,address++ % gb_bus_length);
            break;
        }
        case gb_memory_rom_type:{
            uint8_t* rom = gb->cartridge.rom;
            while(len--) *dst++ = rom[address++ % gb->cartridge.rom_length];
            break;
        }
        case gb_memory_vram_type:{
            uint8_t* vram = gb->ppu.vram;
            while(len--) *dst++ = vram[address++ % gb_vram_length];
            break;
        }
        case gb_memory_ram_type:{
            uint8_t* ram = gb->cartridge.ram;
            while(len--) *dst++ = ram[address++ % gb->cartridge.ram_length];
            break;
        }
        case gb_memory_wram_type:{
            uint8_t* wram = gb->memory.wram;
            while(len--) *dst++ = wram[address++ % gb_wram_length];
            break;
        }
        case gb_memory_oam_type:{
            uint8_t* oam = gb->ppu.oam;
            while(len--) *dst++ = oam[address++ % gb_oam_length];
            break;
        }
        case gb_memory_hram_type:{
            uint8_t* hram = gb->memory.hram;
            while(len--) *dst++ = hram[address++ % gb_hram_length];
            break;
        }
    }
}


void gb_memory_type_import(gb_t* gb,uint8_t memory_type,const char* filename){
    uint8_t* data = NULL;
    size_t len = 0;

    if(!gb_load_file(filename,(void**)&data,&len)) return;

    gb_memory_type_write(gb,memory_type,0,data,len);

    free(data);
}

void gb_memory_type_export(gb_t* gb,uint8_t memory_type,const char* filename){

    switch(memory_type){
        case gb_memory_cpu_type:{

            FILE* file = fopen(filename,"wb");
            
            if(!file){
                gb_printf_errno(fopen);
                return;
            }

            gb_memory_t* memory = &gb->memory;
            
            size_t address = 0;

#ifdef _WIN32
            _lock_file(file);

            while(address < gb_bus_length){
                _fputc_nolock(gb_memory_cpu_read(memory,address++),file);
            }

            _unlock_file(file);
#else
            flockfile(file);

            while(address < gb_bus_length){
                fputc_unlocked(gb_memory_cpu_read(memory,address++),file);
            }
            
            funlockfile(file);
#endif
            fclose(file);
            
            break;
        }
        case gb_memory_rom_type:{
            gb_save_file(filename,gb->cartridge.rom,gb->cartridge.rom_length);
            break;
        }
        case gb_memory_vram_type:{
            gb_save_file(filename,gb->ppu.vram,gb_vram_length);
            break;
        }
        case gb_memory_ram_type:{
            gb_save_file(filename,gb->cartridge.ram,gb->cartridge.ram_length);
            break;
        }
        case gb_memory_wram_type:{
            gb_save_file(filename,gb->memory.wram,gb_wram_length);
            break;
        }
        case gb_memory_oam_type:{
            gb_save_file(filename,gb->ppu.oam,gb_oam_length);
            break;
        }
        case gb_memory_hram_type:{
            gb_save_file(filename,gb->memory.hram,gb_hram_length);
            break;
        }
    }
}
