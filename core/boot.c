#include "boot.h"
#include "gb.h"

void gb_boot_init(gb_boot_t* boot,gb_t* gb){
    boot->gb = gb;
    
    boot->rom_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_empty,
        gb_memory_read_empty,
        boot
    };

    boot->bank_register_descriptor = (gb_memory_descriptor_t){
        gb_boot_write_bank_register,
        gb_boot_read_bank_register,
        boot
    };
}


static void gb_boot_dmg_update_rom(gb_boot_t* boot){
    
    boot->dmg_rom_inserted = false;
    boot->dmg_rom_crc32 = 0;

    if(!boot->dmg_rom_path) return;

    FILE* file = fopen(boot->dmg_rom_path,"rb");
    
    if(!file) return;

    fseek(file,0,SEEK_END);

    size_t size = ftell(file);

    if(size != gb_boot_dmg_rom_size) goto end;

    fseek(file,0,SEEK_SET);

    fread(boot->dmg_rom,1,sizeof(boot->dmg_rom),file);

    boot->dmg_rom_crc32 = gb_crc32(boot->dmg_rom,size);

    boot->dmg_rom_inserted = true;

    end:
    fclose(file);
}

static void gb_boot_cgb_update_rom(gb_boot_t* boot){
    
    boot->cgb_rom_inserted = false;
    boot->cgb_rom_crc32 = 0;

    if(!boot->cgb_rom_path) return;

    FILE* file = fopen(boot->cgb_rom_path,"rb");
    
    if(!file) return;

    fseek(file,0,SEEK_END);

    size_t size = ftell(file);
    
    if(size != sizeof(boot->cgb_rom)) goto end;

    fseek(file,0,SEEK_SET);

    fread(boot->cgb_rom,1,sizeof(boot->cgb_rom),file);

    boot->cgb_rom_crc32 = gb_crc32(boot->cgb_rom,sizeof(boot->cgb_rom));

    boot->cgb_rom_inserted = true;

    end:
    fclose(file);
}

void gb_boot_update_roms(gb_boot_t* boot){
    gb_boot_dmg_update_rom(boot);
    gb_boot_cgb_update_rom(boot);
}


void gb_boot_map(gb_boot_t* boot){
    gb_memory_t* memory = &boot->gb->memory;

    boot->mapped = true;

    gb_memory_map_in_range(memory,&boot->rom_descriptor,0x0000,0x00FF);

    if(boot->gb->is_cgb){
        gb_memory_map_in_range(memory,&boot->rom_descriptor,0x0200,0x08FF);

        boot->rom_descriptor.read = gb_boot_cgb_read_rom;
    }
    else{
        boot->rom_descriptor.read = gb_boot_dmg_read_rom;
    }
}

void gb_boot_unmap(gb_boot_t* boot){
    gb_memory_t* memory = &boot->gb->memory;

    boot->mapped = false;

    gb_memory_map_in_range(memory,&boot->gb->cartridge.rom0_descriptor,0x0000,0x00FF);
    
    if(boot->gb->is_cgb){
        gb_memory_map_in_range(memory,&boot->gb->cartridge.rom0_descriptor,0x0200,0x08FF);
    }
}


void gb_boot_write_bank_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_boot_t* boot = (gb_boot_t*)data;
    
    if(value & 0x01){
        gb_boot_unmap(boot);
        gb_update_mapping(boot->gb);
    }
}

uint8_t gb_boot_read_bank_register(void* data,uint16_t address){
    gb_unused(address);

    gb_boot_t* boot = (gb_boot_t*)data;

    return 0xFE | !boot->mapped;
}


uint8_t gb_boot_dmg_read_rom(void* data,uint16_t address){
    gb_boot_t* boot = (gb_boot_t*)data;
    return boot->dmg_rom[address];
}

uint8_t gb_boot_cgb_read_rom(void* data,uint16_t address){
    gb_boot_t* boot = (gb_boot_t*)data;
    return boot->cgb_rom[address];
}