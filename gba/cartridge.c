#include "cartridge.h"
#include "gba.h"

static const char* gba_cartridge_backup_id[] = {
    "EEPROM_V",
    "SRAM_V",
    "FLASH512_V",
    "FLASH1M_V",
    "FLASH_V",
};

static const int gba_cartridge_backup_id_count = sizeof(gba_cartridge_backup_id) / sizeof(gba_cartridge_backup_id[0]);


void gba_cartridge_init(gba_cartridge_t* cartridge,gba_t* gba){
    cartridge->gba = gba;
}


bool gba_cartridge_load(gba_cartridge_t* cartridge,const char* path){
    FILE* file = fopen(path,"rb");
    if(!file){
        gba_printf_errno(fopen);
        goto fail;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    if(!size) goto fail;

    cartridge->rom = malloc(size);

    if(!cartridge->rom){
        gba_printf_errno(malloc);
        goto fail;
    }

    cartridge->rom_size = size;

    fread(cartridge->rom,1,cartridge->rom_size,file);

    printf("Game Title: %.12s\n",cartridge->rom + 0xA0);
    printf("Game Code: %.4s\n",cartridge->rom + 0xAC);
    printf("Maker Code: %.2s\n",cartridge->rom + 0xB0);

    for(int i = 0; i < gba_cartridge_backup_id_count; ++i){
        
        size_t length = strlen(gba_cartridge_backup_id[i]);

        for(int j = 0; j < cartridge->rom_size; j += sizeof(uint32_t)){

            if(j + length > cartridge->rom_size) break;

            if(!memcmp(cartridge->rom + j,gba_cartridge_backup_id[i],length)){
                printf("Backup Type: %s\n",gba_cartridge_backup_id[i]);
            }
        }
    }

    fclose(file);

    return true;
    
    fail:
    
    fclose(file);

    return false;
}

void gba_cartridge_unload(gba_cartridge_t* cartridge){
    if(cartridge->rom != NULL){
        free(cartridge->rom);
    }
    cartridge->rom_size = 0;
}


static void gba_cartridge_write_rom_raw(gba_cartridge_t* cartridge,uint8_t value,uint32_t address){

    if(address >= 0x08000064 && address <= 0x08000069){
        switch(address){
            // Data
            case 0x08000064: cartridge->io.data = value & 0x0F; break;
            case 0x08000065: break;
            // Direction
            case 0x08000066: cartridge->io.direction = value & 0x0F; break;
            case 0x08000067: break;
            // Control
            case 0x08000068: cartridge->io.read_enabled = value & 0x01; break;
            case 0x08000069: break;
        }
    }
}

static uint8_t gba_cartridge_read_rom_raw(gba_cartridge_t* cartridge,uint32_t address){

    if(cartridge->io.read_enabled && address >= 0x08000064 && address <= 0x08000069){
        switch(address){
            case 0x08000064: return cartridge->io.data & 0x0F;
            case 0x08000065: return 0x00;

            case 0x08000066: return cartridge->io.direction & 0x0F;
            case 0x08000067: return 0x00;

            case 0x08000068: return (cartridge->io.read_enabled ? 0x01 : 0x00);
            case 0x08000069: return 0x00;
        }
    }

    address &= gba_cartridge_rom_address_mask;

    if(address < cartridge->rom_size){
        return cartridge->rom[address]; 
    }

    return (address >> ((address & 0x01) ? 0x09 : 0x01)) & 0xFF;
}


void gba_cartridge_write_rom(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t size,bool sequential){

    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(cartridge->gba,table[page][sequential]);

            gba_cartridge_write_rom_raw(cartridge,value.b[0],address);
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(cartridge->gba,table[page][sequential]);

            gba_cartridge_write_rom_raw(cartridge,value.b[0],address | 0x00);
            gba_cartridge_write_rom_raw(cartridge,value.b[1],address | 0x01);
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(cartridge->gba,table[page][sequential]);

            gba_cartridge_write_rom_raw(cartridge,value.b[0],address | 0x00);
            gba_cartridge_write_rom_raw(cartridge,value.b[1],address | 0x01);

            gba_clock(cartridge->gba,table[page][gba_memory_sequential_access]);

            gba_cartridge_write_rom_raw(cartridge,value.b[2],address | 0x02);
            gba_cartridge_write_rom_raw(cartridge,value.b[3],address | 0x03);
            break;
        }
    }

}

gba_word_t gba_cartridge_read_rom(gba_cartridge_t* cartridge,uint32_t address,uint8_t size,bool sequential){

    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_word_t value = {0};

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(cartridge->gba,table[page][sequential]);

            value.b[0] = gba_cartridge_read_rom_raw(cartridge,address);
            value.b[1] = value.b[0];
            value.b[2] = value.b[0];
            value.b[3] = value.b[0];
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(cartridge->gba,table[page][sequential]);

            value.b[0] = gba_cartridge_read_rom_raw(cartridge,address | 0x00);
            value.b[1] = gba_cartridge_read_rom_raw(cartridge,address | 0x01);
            value.b[2] = value.b[0];
            value.b[3] = value.b[1];
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(cartridge->gba,table[page][sequential]);

            value.b[0] = gba_cartridge_read_rom_raw(cartridge,address | 0x00);
            value.b[1] = gba_cartridge_read_rom_raw(cartridge,address | 0x01);

            gba_clock(cartridge->gba,table[page][gba_memory_sequential_access]);

            value.b[2] = gba_cartridge_read_rom_raw(cartridge,address | 0x02);
            value.b[3] = gba_cartridge_read_rom_raw(cartridge,address | 0x03);
            break;
        }
    }

    return value;
}


static void gba_cartridge_write_sram_raw(gba_cartridge_t* cartridge,uint8_t value,uint32_t address){
    
}

static uint8_t gba_cartridge_read_sram_raw(gba_cartridge_t* cartridge,uint32_t address){
    return 0xFF;
}


void gba_cartridge_write_sram(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t size,bool sequential){
    
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(cartridge->gba,table[page][sequential]);

            gba_cartridge_write_sram_raw(cartridge,value.b[0],address);
            break;
        }
        case gba_memory_halfword_access:{
            gba_clock(cartridge->gba,table[page][sequential]);

            gba_cartridge_write_sram_raw(cartridge,value.b[address & 0x01],address);
            break;
        }
        case gba_memory_word_access:{
            gba_clock(cartridge->gba,table[page][sequential]);

            gba_cartridge_write_sram_raw(cartridge,value.b[address & 0x03],address);
            break;
        }
    }
}

gba_word_t gba_cartridge_read_sram(gba_cartridge_t* cartridge,uint32_t address,uint8_t size,bool sequential){

    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_word_t value = {0};

    switch(size){
        case gba_memory_byte_access:
        case gba_memory_halfword_access:
        case gba_memory_word_access:{
            gba_clock(cartridge->gba,table[page][sequential]);

            value.b[0] = gba_cartridge_read_sram_raw(cartridge,address);
            value.b[1] = value.b[0];
            value.b[2] = value.b[0];
            value.b[3] = value.b[0];
            break;
        }
    }

    return value;
}


void gba_cartridge_reset(gba_cartridge_t* cartridge){
    cartridge->io.data = 0x00;
    cartridge->io.direction = 0x00;
    cartridge->io.read_enabled = false;
}