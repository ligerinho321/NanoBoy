#include "memory_type.h"
#include "../gba.h"

const char* gba_memory_type_names[gba_memory_type_count] = {
    "CPU Memory",
    "BIOS ROM",
    "External Work RAM",
    "Internal Work RAM",
    "Color RAM",
    "Video RAM",
    "Object RAM",
    "ROM",
    "Save RAM"
};


uint8_t gba_memory_type(uint32_t address){
    switch(address >> 24){
        // BIOS
        case 0x00:
            if(address < gba_boot_length){
                return gba_memory_bios_type;
            }
            else{
                return -1;
            }
        // EWRAM
        case 0x02: return gba_memory_ewram_type;
        // IWRAM
        case 0x03: return gba_memory_iwram_type;
        // I/O
        case 0x04: return -1;
        // CRAM
        case 0x05: return gba_memory_cram_type;
        // VRAM
        case 0x06: return gba_memory_vram_type;
        // OAM
        case 0x07: return gba_memory_oam_type;
        // ROM
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
            return gba_memory_rom_type;
        // SRAM
        case 0x0E: case 0x0F:
            return gba_memory_sram_type;
        default:
            return -1;
    }
}

size_t gba_memory_type_length(gba_t* gba,uint8_t memory_type){
    switch(memory_type){
        case gba_memory_cpu_type: return gba_bus_length;
        case gba_memory_bios_type: return gba_boot_length;
        case gba_memory_ewram_type: return gba_ewram_length;
        case gba_memory_iwram_type: return gba_iwram_length;
        case gba_memory_cram_type: return gba_cram_length;
        case gba_memory_vram_type: return gba_vram_length;
        case gba_memory_oam_type: return gba_oam_length;
        case gba_memory_rom_type: return gba->cartridge.rom_length;
        case gba_memory_sram_type: return gba->cartridge.sram_length;
        default: return 0;
    }
}

size_t gba_memory_type_absolute_address(gba_t* gba,uint8_t memory_type,uint32_t relative_address){
    switch(memory_type){
        case gba_memory_cpu_type:
            return relative_address;
        case gba_memory_bios_type:
            return relative_address & gba_boot_address_mask;
        case gba_memory_ewram_type:
            return relative_address & gba_ewram_address_mask;
        case gba_memory_iwram_type:
            return relative_address & gba_iwram_address_mask;
        case gba_memory_cram_type:
            return relative_address & gba_cram_address_mask;
        case gba_memory_vram_type:
            return relative_address & gba_vram_address_mask;
        case gba_memory_oam_type:
            return relative_address & gba_oam_address_mask;
        case gba_memory_rom_type:
            relative_address &= gba_cartridge_rom_address_mask;
            if(relative_address < gba->cartridge.rom_length){
                return relative_address;
            }
            else{
                return -1;
            }
        case gba_memory_sram_type:
            relative_address &= gba_cartridge_sram_address_mask;
            if(relative_address < gba->cartridge.sram_length){
                return relative_address;
            }
            else{
                return -1;
            }
        default:
            return -1;
    }
}


void gba_memory_type_write_byte(gba_t* gba,uint8_t memory_type,uint8_t value,size_t address){
    switch(memory_type){
        case gba_memory_cpu_type:{
            // TODO
            break;
        }
        case gba_memory_bios_type:{
            gba->boot.data[address % gba_boot_length] = value;
            break;
        }
        case gba_memory_ewram_type:{
            gba->memory.ewram[address % gba_ewram_length] = value;
            break;
        }
        case gba_memory_iwram_type:{
            gba->memory.iwram[address % gba_iwram_length] = value;
            break;
        }
        case gba_memory_cram_type:{
            gba->ppu.cram[address % gba_cram_length] = value;
            break;
        }
        case gba_memory_vram_type:{
            gba->ppu.vram[address % gba_vram_length] = value;
            break;
        }
        case gba_memory_oam_type:{
            gba->ppu.oam[address % gba_oam_length] = value;
            break;
        }
        case gba_memory_rom_type:{
            gba->cartridge.rom[address % gba->cartridge.rom_length] = value;
            break;
        }
        case gba_memory_sram_type:{
            gba->cartridge.sram[address % gba->cartridge.sram_length] = value;
            break;
        }
    }
}

uint8_t gba_memory_type_read_byte(gba_t* gba,uint8_t memory_type,size_t address){
    switch(memory_type){
        case gba_memory_cpu_type:{
            // TODO
            return 0x00;
        }
        case gba_memory_bios_type:{
            return gba->boot.data[address % gba_boot_length];
        }
        case gba_memory_ewram_type:{
            return gba->memory.ewram[address % gba_ewram_length];
        }
        case gba_memory_iwram_type:{
            return gba->memory.iwram[address % gba_iwram_length];
        }
        case gba_memory_cram_type:{
            return gba->ppu.cram[address % gba_cram_length];
        }
        case gba_memory_vram_type:{
            return gba->ppu.vram[address % gba_vram_length];
        }
        case gba_memory_oam_type:{
            return gba->ppu.oam[address % gba_oam_length];
        }
        case gba_memory_rom_type:{
            return gba->cartridge.rom[address % gba->cartridge.rom_length];
        }
        case gba_memory_sram_type:{
            return gba->cartridge.sram[address % gba->cartridge.sram_length];
        }
        default:{
            return 0x00;
        }
    }
}


void gba_memory_type_write(gba_t* gba,uint8_t memory_type,size_t address,uint8_t* src,size_t length){
    switch(memory_type){
        case gba_memory_cpu_type:{
            // TODO
            break;
        }
        case gba_memory_bios_type:{
            uint8_t* dst = gba->boot.data;
            while(length--) dst[address++ % gba_boot_length] = *src++;
            break;
        }
        case gba_memory_ewram_type:{
            uint8_t* dst = gba->memory.ewram;
            while(length--) dst[address++ % gba_ewram_length] = *src++;
            break;
        }
        case gba_memory_iwram_type:{
            uint8_t* dst = gba->memory.iwram;
            while(length--) dst[address++ % gba_iwram_length] = *src++;
            break;
        }
        case gba_memory_cram_type:{
            uint8_t* dst = gba->ppu.cram;
            while(length--) dst[address++ % gba_cram_length] = *src++;
            break;
        }
        case gba_memory_vram_type:{
            uint8_t* dst = gba->ppu.vram;
            while(length--) dst[address++ % gba_vram_length] = *src++;
            break;
        }
        case gba_memory_oam_type:{
            uint8_t* dst = gba->ppu.oam;
            while(length--) dst[address++ % gba_oam_length] = *src++;
            break;
        }
        case gba_memory_rom_type:{
            uint8_t* dst = gba->cartridge.rom;
            size_t dst_length = gba->cartridge.rom_length;
            while(length--) dst[address++ % dst_length] = *src++;
            break;
        }
        case gba_memory_sram_type:{
            uint8_t* dst = gba->cartridge.sram;
            size_t dst_length = gba->cartridge.sram_length;
            while(length--) dst[address++ % dst_length] = *src++;
            break;
        }
    }
}

void gba_memory_type_read(gba_t* gba,uint8_t memory_type,size_t address,uint8_t* dst,size_t length){
    switch(memory_type){
        case gba_memory_cpu_type:{
            // TODO
            break;
        }
        case gba_memory_bios_type:{
            uint8_t* src = gba->boot.data;
            while(length--) *dst++ = src[address++ % gba_boot_length];
            break;
        }
        case gba_memory_ewram_type:{
            uint8_t* src = gba->memory.ewram;
            while(length--) *dst++ = src[address++ % gba_ewram_length];
            break;
        }
        case gba_memory_iwram_type:{
            uint8_t* src = gba->memory.iwram;
            while(length--) *dst++ = src[address++ % gba_iwram_length];
            break;
        }
        case gba_memory_cram_type:{
            uint8_t* src = gba->ppu.cram;
            while(length--) *dst++ = src[address++ % gba_cram_length];
            break;
        }
        case gba_memory_vram_type:{
            uint8_t* src = gba->ppu.vram;
            while(length--) *dst++ = src[address++ % gba_vram_length];
            break;
        }
        case gba_memory_oam_type:{
            uint8_t* src = gba->ppu.oam;
            while(length--) *dst++ = src[address++ % gba_oam_length];
            break;
        }
        case gba_memory_rom_type:{
            uint8_t* src = gba->cartridge.rom;
            size_t src_length = gba->cartridge.rom_length;
            while(length--) *dst++ = src[address++ % src_length];
            break;
        }
        case gba_memory_sram_type:{
            uint8_t* src = gba->cartridge.sram;
            size_t src_length = gba->cartridge.sram_length;
            while(length--) *dst++ = src[address++ % src_length];
            break;
        }
    }
}


void gba_memory_type_import(gba_t* gba,uint8_t memory_type,const char* path){
    uint8_t* data = NULL;
    size_t length = 0;

    if(!gba_load_file(path,(void**)&data,&length)) return;

    gba_memory_type_write(gba,memory_type,0,data,length);
}

void gba_memory_type_export(gba_t* gba,uint8_t memory_type,const char* path){
    switch(memory_type){
        case gba_memory_cpu_type:{
            // TODO
            break;
        }
        case gba_memory_bios_type:{
            gba_save_file(path,gba->boot.data,gba_boot_length);
            break;
        }
        case gba_memory_ewram_type:{
            gba_save_file(path,gba->memory.ewram,gba_ewram_length);
            break;
        }
        case gba_memory_iwram_type:{
            gba_save_file(path,gba->memory.iwram,gba_iwram_length);
            break;
        }
        case gba_memory_cram_type:{
            gba_save_file(path,gba->ppu.cram,gba_cram_length);
            break;
        }
        case gba_memory_vram_type:{
            gba_save_file(path,gba->ppu.vram,gba_vram_length);
            break;
        }
        case gba_memory_oam_type:{
            gba_save_file(path,gba->ppu.oam,gba_oam_length);
            break;
        }
        case gba_memory_rom_type:{
            gba_save_file(path,gba->cartridge.rom,gba->cartridge.rom_length);
            break;
        }
        case gba_memory_sram_type:{
            gba_save_file(path,gba->cartridge.sram,gba->cartridge.sram_length);
            break;
        }
    }
}