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


bool gba_cartridge_load(gba_cartridge_t* cartridge,uint8_t* rom,size_t length){

    if(!gba_cartridge_rom_is_valid(rom,length)) goto fail;

    cartridge->rom = rom;
    cartridge->rom_length = length;

    printf("Game Title: %.12s\n",cartridge->rom + 0xA0);
    printf("Game Code: %.4s\n",cartridge->rom + 0xAC);
    printf("Maker Code: %.2s\n",cartridge->rom + 0xB0);

    for(int i = 0; i < gba_cartridge_backup_id_count; ++i){
        
        size_t length = strlen(gba_cartridge_backup_id[i]);

        for(size_t j = 0; j < cartridge->rom_length; j += sizeof(uint32_t)){

            if(j + length > cartridge->rom_length) break;

            if(!memcmp(cartridge->rom + j,gba_cartridge_backup_id[i],length)){
                printf("Backup Type: %s\n",gba_cartridge_backup_id[i]);
            }
        }
    }

    return true;
    
    fail:
    gba_cartridge_unload(cartridge);

    return false;
}

void gba_cartridge_unload(gba_cartridge_t* cartridge){
    if(cartridge->rom != NULL){
        free(cartridge->rom);
        cartridge->rom = NULL;
    }
    cartridge->rom_length = 0;
}


bool gba_cartridge_rom_is_valid(const uint8_t* rom,size_t length){
    if(length < gba_cartridge_rom_min_length || length > gba_cartridge_rom_max_length) return false;
    if(rom[0xB2] != 0x96) return false;
    if(!gba_cartridge_verify_header_nintendo_logo(rom)) return false;
    if(!gba_cartridge_verify_header_checksum(rom)) return false;
    return true;
}

bool gba_cartridge_verify_header_nintendo_logo(const uint8_t* rom){

    static uint8_t nintendo_logo[0x9C] = {
        0x24,0xFF,0xAE,0x51,0x69,0x9A,0xA2,0x21,0x3D,0x84,0x82,0x0A,0x84,0xE4,0x09,0xAD,
        0x11,0x24,0x8B,0x98,0xC0,0x81,0x7F,0x21,0xA3,0x52,0xBE,0x19,0x93,0x09,0xCE,0x20,
        0x10,0x46,0x4A,0x4A,0xF8,0x27,0x31,0xEC,0x58,0xC7,0xE8,0x33,0x82,0xE3,0xCE,0xBF,
        0x85,0xF4,0xDF,0x94,0xCE,0x4B,0x09,0xC1,0x94,0x56,0x8A,0xC0,0x13,0x72,0xA7,0xFC,
        0x9F,0x84,0x4D,0x73,0xA3,0xCA,0x9A,0x61,0x58,0x97,0xA3,0x27,0xFC,0x03,0x98,0x76,
        0x23,0x1D,0xC7,0x61,0x03,0x04,0xAE,0x56,0xBF,0x38,0x84,0x00,0x40,0xA7,0x0E,0xFD,
        0xFF,0x52,0xFE,0x03,0x6F,0x95,0x30,0xF1,0x97,0xFB,0xC0,0x85,0x60,0xD6,0x80,0x25,
        0xA9,0x63,0xBE,0x03,0x01,0x4E,0x38,0xE2,0xF9,0xA2,0x34,0xFF,0xBB,0x3E,0x03,0x44,
        0x78,0x00,0x90,0xCB,0x88,0x11,0x3A,0x94,0x65,0xC0,0x7C,0x63,0x87,0xF0,0x3C,0xAF,
        0xD6,0x25,0xE4,0x8B,0x38,0x0A,0xAC,0x72,0x21,0xD4,0xF8,0x07
    };

    for(int i = 0x00; i < 0x9C; ++i){

        uint8_t mask = 0xFF;
        
        int offset = i + 0x04;

        if(offset == 0x9C) mask &= ~0x84;
        if(offset == 0x9E) mask &= ~0x03;

        if((rom[offset] & mask) != (nintendo_logo[i] & mask)) return false;
    }

    return true;
}

bool gba_cartridge_verify_header_checksum(const uint8_t* rom){
    uint8_t checksum = 0x00;
    for(int i = 0xA0; i <= 0xBC; ++i){
        checksum -= rom[i];
    }
    checksum -= 0x19;
    return checksum == rom[0xBD];
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

    if(address < cartridge->rom_length){
        return cartridge->rom[address]; 
    }

    return (address >> ((address & 0x01) ? 0x09 : 0x01)) & 0xFF;
}


void gba_cartridge_write_rom_byte(gba_cartridge_t* cartridge,uint8_t value,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_clock(cartridge->gba,table[page][access]);

    gba_cartridge_write_rom_raw(cartridge,value,address);
}

gba_word_t gba_cartridge_read_rom_byte(gba_cartridge_t* cartridge,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_clock(cartridge->gba,table[page][access]);

    gba_word_t value = {0};

    value.b[0] = gba_cartridge_read_rom_raw(cartridge,address);
    value.b[1] = value.b[0];
    value.b[2] = value.b[0];
    value.b[3] = value.b[0];

    return value;
}


void gba_cartridge_write_rom_halfword(gba_cartridge_t* cartridge,gba_halfword_t value,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    address &= ~0x01;

    gba_clock(cartridge->gba,table[page][access]);

    gba_cartridge_write_rom_raw(cartridge,value.b[0],address | 0x00);
    gba_cartridge_write_rom_raw(cartridge,value.b[1],address | 0x01);
}

gba_word_t gba_cartridge_read_rom_halfword(gba_cartridge_t* cartridge,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    address &= ~0x01;

    gba_clock(cartridge->gba,table[page][access]);

    gba_word_t value = {0};

    value.b[0] = gba_cartridge_read_rom_raw(cartridge,address | 0x00);
    value.b[1] = gba_cartridge_read_rom_raw(cartridge,address | 0x01);
    value.b[2] = value.b[0];
    value.b[3] = value.b[1];

    return value;
}


void gba_cartridge_write_rom_word(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    address &= ~0x03;

    gba_clock(cartridge->gba,table[page][access]);

    gba_cartridge_write_rom_raw(cartridge,value.b[0],address | 0x00);
    gba_cartridge_write_rom_raw(cartridge,value.b[1],address | 0x01);

    gba_clock(cartridge->gba,table[page][gba_sequential_access]);

    gba_cartridge_write_rom_raw(cartridge,value.b[2],address | 0x02);
    gba_cartridge_write_rom_raw(cartridge,value.b[3],address | 0x03);
}

gba_word_t gba_cartridge_read_rom_word(gba_cartridge_t* cartridge,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    address &= ~0x03;

    gba_word_t value = {0};

    gba_clock(cartridge->gba,table[page][access]);

    value.b[0] = gba_cartridge_read_rom_raw(cartridge,address | 0x00);
    value.b[1] = gba_cartridge_read_rom_raw(cartridge,address | 0x01);

    gba_clock(cartridge->gba,table[page][gba_sequential_access]);

    value.b[2] = gba_cartridge_read_rom_raw(cartridge,address | 0x02);
    value.b[3] = gba_cartridge_read_rom_raw(cartridge,address | 0x03);

    return value;
}



static void gba_cartridge_write_sram_raw(gba_cartridge_t* cartridge,uint8_t value,uint32_t address){
    gba_unused(cartridge);
    gba_unused(value);
    gba_unused(address);
}

static uint8_t gba_cartridge_read_sram_raw(gba_cartridge_t* cartridge,uint32_t address){
    gba_unused(cartridge);
    gba_unused(address);
    return 0xFF;
}


void gba_cartridge_write_sram_byte(gba_cartridge_t* cartridge,uint8_t value,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_clock(cartridge->gba,table[page][access]);

    gba_cartridge_write_sram_raw(cartridge,value,address);
}

gba_word_t gba_cartridge_read_sram_byte(gba_cartridge_t* cartridge,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_clock(cartridge->gba,table[page][access]);

    gba_word_t value = {0};

    value.b[0] = gba_cartridge_read_sram_raw(cartridge,address);
    value.b[1] = value.b[0];
    value.b[2] = value.b[0];
    value.b[3] = value.b[0];

    return value;
}


void gba_cartridge_write_sram_halfword(gba_cartridge_t* cartridge,gba_halfword_t value,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_clock(cartridge->gba,table[page][access]);

    gba_cartridge_write_sram_raw(cartridge,value.b[address & 0x01],address);
}


void gba_cartridge_write_sram_word(gba_cartridge_t* cartridge,gba_word_t value,uint32_t address,uint8_t access){
    uint8_t (*table)[2] = cartridge->gba->memory.waitstate_table;
    uint8_t page = (address >> 24) & 0x07;

    gba_clock(cartridge->gba,table[page][access]);

    gba_cartridge_write_sram_raw(cartridge,value.b[address & 0x03],address);
}



void gba_cartridge_reset(gba_cartridge_t* cartridge){
    cartridge->io.data = 0x00;
    cartridge->io.direction = 0x00;
    cartridge->io.read_enabled = false;
}