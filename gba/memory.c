#include "memory.h"
#include "gba.h"

void gba_memory_init(gba_memory_t* memory,gba_t* gba){
    memory->gba = gba;
}


void gba_memory_write_byte(gba_memory_t* memory,uint8_t value,uint32_t address,uint8_t access){
    switch(address >> 24){
        // BIOS
        case 0x00:
            break;
        // EWRAM
        case 0x02:
            gba_memory_write_ewram_byte(memory,value,address);
            break;
        // IWRAM
        case 0x03:
            gba_memory_write_iwram_byte(memory,value,address);
            break;
        // I/O
        case 0x04:
            gba_memory_write_register_byte(memory,value,address);
            break;
        // CRAM
        case 0x05:
            gba_ppu_write_cram_byte(&memory->gba->ppu,value,address);
            break;
        // VRAM
        case 0x06:
            gba_ppu_write_vram_byte(&memory->gba->ppu,value,address);
            break;
        // OAM
        case 0x07:
            gba_ppu_write_oam_byte(&memory->gba->ppu,value,address);
            break;
        // ROM
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
            gba_cartridge_write_rom_byte(&memory->gba->cartridge,value,address,access);
            break;
        // SRAM
        case 0x0E: case 0x0F:
            gba_cartridge_write_sram_byte(&memory->gba->cartridge,value,address,access);
            break;
    }
}

uint8_t gba_memory_read_byte(gba_memory_t* memory,uint32_t address,uint8_t access){
    switch(address >> 24){
        // BIOS
        case 0x00:
            if(memory->gba->boot.inserted && address < gba_boot_length){
                memory->bus = gba_boot_read_byte(&memory->gba->boot,address);
            }
            else{
                memory->bus = gba_memory_read_unused_byte(memory,address);
            }
            break;
        // EWRAM
        case 0x02:
            memory->bus = gba_memory_read_ewram_byte(memory,address);
            break;
        // IWRAM
        case 0x03:
            memory->bus = gba_memory_read_iwram_byte(memory,address);
            break;
        // I/O
        case 0x04:
            memory->bus = gba_memory_read_register_byte(memory,address);
            break;
        // CRAM
        case 0x05:
            memory->bus = gba_ppu_read_cram_byte(&memory->gba->ppu,address);
            break;
        // VRAM
        case 0x06:
            memory->bus = gba_ppu_read_vram_byte(&memory->gba->ppu,address);
            break;
        // OAM
        case 0x07:
            memory->bus = gba_ppu_read_oam_byte(&memory->gba->ppu,address);
            break;
        // ROM
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
            memory->bus = gba_cartridge_read_rom_byte(&memory->gba->cartridge,address,access);
            break;
        // SRAM
        case 0x0E: case 0x0F:
            memory->bus = gba_cartridge_read_sram_byte(&memory->gba->cartridge,address,access);
            break;
        default:
            memory->bus = gba_memory_read_unused_byte(memory,address);
            break;
    }

    return memory->bus.v;
}


void gba_memory_write_halfword(gba_memory_t* memory,uint16_t value,uint32_t address,uint8_t access){
    gba_halfword_t _value = {.v = value};

    switch(address >> 24){
        // BIOS
        case 0x00:
            break;
        // EWRAM
        case 0x02:
            gba_memory_write_ewram_halfword(memory,_value,address);
            break;
        // IWRAM
        case 0x03:
            gba_memory_write_iwram_halfword(memory,_value,address);
            break;
        // I/O
        case 0x04:
            gba_memory_write_register_halfword(memory,_value,address);
            break;
        // CRAM
        case 0x05:
            gba_ppu_write_cram_halfword(&memory->gba->ppu,_value,address);
            break;
        // VRAM
        case 0x06:
            gba_ppu_write_vram_halfword(&memory->gba->ppu,_value,address);
            break;
        // OAM
        case 0x07:
            gba_ppu_write_oam_halfword(&memory->gba->ppu,_value,address);
            break;
        // ROM
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
            gba_cartridge_write_rom_halfword(&memory->gba->cartridge,_value,address,access);
            break;
        // SRAM
        case 0x0E: case 0x0F:
            gba_cartridge_write_sram_halfword(&memory->gba->cartridge,_value,address,access);
            break;
    }
}

uint16_t gba_memory_read_halfword(gba_memory_t* memory,uint32_t address,uint8_t access){
    switch(address >> 24){
        // BIOS
        case 0x00:
            if(memory->gba->boot.inserted && address < gba_boot_length){
                memory->bus = gba_boot_read_halfword(&memory->gba->boot,address);
            }
            else{
                memory->bus = gba_memory_read_unused_halfword(memory,address);
            }
            break;
        // EWRAM
        case 0x02:
            memory->bus = gba_memory_read_ewram_halfword(memory,address);
            break;
        // IWRAM
        case 0x03:
            memory->bus = gba_memory_read_iwram_halfword(memory,address);
            break;
        // I/O
        case 0x04:
            memory->bus = gba_memory_read_register_halfword(memory,address);
            break;
        // CRAM
        case 0x05:
            memory->bus = gba_ppu_read_cram_halfword(&memory->gba->ppu,address);
            break;
        // VRAM
        case 0x06:
            memory->bus = gba_ppu_read_vram_halfword(&memory->gba->ppu,address);
            break;
        // OAM
        case 0x07:
            memory->bus = gba_ppu_read_oam_halfword(&memory->gba->ppu,address);
            break;
        // ROM
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
            memory->bus = gba_cartridge_read_rom_halfword(&memory->gba->cartridge,address,access);
            break;
        // SRAM
        case 0x0E: case 0x0F:
            memory->bus = gba_cartridge_read_sram_byte(&memory->gba->cartridge,address,access);
            break;
        default:
            memory->bus = gba_memory_read_unused_halfword(memory,address);
            break;
    }

    return memory->bus.v;
}


void gba_memory_write_word(gba_memory_t* memory,uint32_t value,uint32_t address,uint8_t access){
    gba_word_t _value = {.v = value};

    switch(address >> 24){
        // BIOS
        case 0x00:
            break;
        // EWRAM
        case 0x02:
            gba_memory_write_ewram_word(memory,_value,address);
            break;
        // IWRAM
        case 0x03:
            gba_memory_write_iwram_word(memory,_value,address);
            break;
        // I/O
        case 0x04:
            gba_memory_write_register_word(memory,_value,address);
            break;
        // CRAM
        case 0x05:
            gba_ppu_write_cram_word(&memory->gba->ppu,_value,address);
            break;
        // VRAM
        case 0x06:
            gba_ppu_write_vram_word(&memory->gba->ppu,_value,address);
            break;
        // OAM
        case 0x07:
            gba_ppu_write_oam_word(&memory->gba->ppu,_value,address);
            break;
        // ROM
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
            gba_cartridge_write_rom_word(&memory->gba->cartridge,_value,address,access);
            break;
        // SRAM
        case 0x0E: case 0x0F:
            gba_cartridge_write_sram_word(&memory->gba->cartridge,_value,address,access);
            break;
    }
}

uint32_t gba_memory_read_word(gba_memory_t* memory,uint32_t address,uint8_t access){
    switch(address >> 24){
        // BIOS
        case 0x00:
            if(memory->gba->boot.inserted && address < gba_boot_length){
                memory->bus = gba_boot_read_word(&memory->gba->boot,address);
            }
            else{
                memory->bus = gba_memory_read_unused_word(memory,address);
            }
            break;
        // EWRAM
        case 0x02:
            memory->bus = gba_memory_read_ewram_word(memory,address);
            break;
        // IWRAM
        case 0x03:
            memory->bus = gba_memory_read_iwram_word(memory,address);
            break;
        // I/O
        case 0x04:
            memory->bus = gba_memory_read_register_word(memory,address);
            break;
        // CRAM
        case 0x05:
            memory->bus = gba_ppu_read_cram_word(&memory->gba->ppu,address);
            break;
        // VRAM
        case 0x06:
            memory->bus = gba_ppu_read_vram_word(&memory->gba->ppu,address);
            break;
        // OAM
        case 0x07:
            memory->bus = gba_ppu_read_oam_word(&memory->gba->ppu,address);
            break;
        // ROM
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
            memory->bus = gba_cartridge_read_rom_word(&memory->gba->cartridge,address,access);
            break;
        // SRAM
        case 0x0E: case 0x0F:
            memory->bus = gba_cartridge_read_sram_byte(&memory->gba->cartridge,address,access);
            break;
        default:
            memory->bus = gba_memory_read_unused_word(memory,address);
            break;
    }

    return memory->bus.v;
}


void gba_memory_update_waitstate_table(gba_memory_t* memory){
    static const uint8_t ws_first_access[4] = {5,4,3,9};
    static const uint8_t ws0_second_access[2] = {3,2};
    static const uint8_t ws1_second_access[2] = {5,2};
    static const uint8_t ws2_second_access[2] = {9,2};

    gba_waitstate_control_t* control = &memory->waitstate_control;
    uint8_t (*table)[2] = memory->waitstate_table;

    table[0][0] = table[1][0] = ws_first_access[control->ws0_first_access];
    table[0][1] = table[1][1] = ws0_second_access[control->ws0_second_access];

    table[2][0] = table[3][0] = ws_first_access[control->ws1_first_access];
    table[2][1] = table[3][1] = ws1_second_access[control->ws1_second_access];

    table[4][0] = table[5][0] = ws_first_access[control->ws2_first_access];
    table[4][1] = table[5][1] = ws2_second_access[control->ws2_second_access];

    table[6][0] = table[7][0] = ws_first_access[control->ws_sram];
    table[6][1] = table[7][1] = ws_first_access[control->ws_sram];
}


static inline void gba_memory_write_waitstate_control_lsb(gba_memory_t* memory,uint8_t value){
    gba_waitstate_control_t* control = &memory->waitstate_control;

    control->ws_sram = value & 0x03;
    control->ws0_first_access = (value >> 0x02) & 0x03;
    control->ws0_second_access = (value & 0x10) != 0x00;
    control->ws1_first_access = (value >> 0x05) & 0x03;
    control->ws1_second_access = (value & 0x80) != 0x00;

    gba_memory_update_waitstate_table(memory);
}

static inline uint8_t gba_memory_read_waitstate_control_lsb(gba_waitstate_control_t* control){
    return (
        ((control->ws_sram & 0x03) << 0x00) |
        ((control->ws0_first_access & 0x03) << 0x02) |
        (control->ws0_second_access ? 0x10 : 0x00) |
        ((control->ws1_first_access & 0x03) << 0x05) |
        (control->ws1_second_access ? 0x80 : 0x00)
    );
}

static inline void gba_memory_write_waitstate_control_msb(gba_memory_t* memory,uint8_t value){\
    gba_waitstate_control_t* control = &memory->waitstate_control;

    control->ws2_first_access = value & 0x03;
    control->ws2_second_access = (value & 0x04) != 0x00;
    control->phi = (value >> 0x03) & 0x03;
    control->prefetch_enabled = (value & 0x40) != 0x00;

    gba_memory_update_waitstate_table(memory);
}

static inline uint8_t gba_memory_read_waitstate_control_msb(gba_waitstate_control_t* control){
    return (
        ((control->ws2_first_access & 0x03) << 0x00) |
        (control->ws2_second_access ? 0x04 : 0x00) |
        ((control->phi & 0x03) << 0x03) |
        (control->prefetch_enabled ? 0x40 : 0x00)
    );
}


static inline void gba_memory_write_control_lsb(gba_memory_control_t* control,uint8_t value){
    control->wram_disabled = (value & 0x01) != 0x00;
    control->unknown0 = (value >> 0x01) & 0x03;
    control->cgb_bootrom_disabled = (value & 0x08) != 0x00;
    control->ewram_enabled = (value & 0x20) != 0x00;
}

static inline uint8_t gba_memory_read_control_lsb(gba_memory_control_t* control){
    return (
        (control->wram_disabled ? 0x01 : 0x00) |
        ((control->unknown0 & 0x03) << 0x01) |
        (control->cgb_bootrom_disabled ? 0x08 : 0x00) |
        (control->ewram_enabled ? 0x20 : 0x00)
    );
}

static inline void gba_memory_write_control_msb(gba_memory_control_t* control,uint8_t value){
    control->wram_waitstate = value & 0x0F;
    control->unknown1 = (value >> 0x04) & 0x0F;
}

static inline uint8_t gba_memory_read_control_msb(gba_memory_control_t* control){
    return (
        ((control->wram_waitstate & 0x0F) << 0x00) |
        ((control->unknown1 & 0x0F) << 0x04) 
    );
}



static void gba_memory_write_register_raw(gba_memory_t* memory,uint8_t value,uint32_t address){
    // LCD I/O Registers
    if(address < 0x04000060){
        gba_ppu_write_register_raw(&memory->gba->ppu,value,address);
    }
    // Sound Registers
    else if(address < 0x040000B0){
        gba_apu_write_register_raw(&memory->gba->apu,value,address);
    }
    // DMA Registers
    else if(address < 0x04000100){
        gba_dma_write_register_raw(&memory->gba->dma,value,address);
    }
    // Timer Registers
    else if(address < 0x04000120){
        gba_timer_write_register_raw(&memory->gba->timer,value,address);
    }
    // Serial Registers 1
    else if(address < 0x04000130){
        gba_serial_write_register1_raw(&memory->gba->serial,value,address);
    }
    // Keypad Registers
    else if(address < 0x04000134){
        gba_keypad_write_register_raw(&memory->gba->keypad,value,address);
    }
    // Serial Registers 2
    else if(address < 0x04000200){
        gba_serial_write_register2_raw(&memory->gba->serial,value,address);
    }
    // Others Registers
    else{
        switch(address){
            // Interrupt Enable
            case 0x04000200: memory->gba->interrupt.enable.b[0] = value; break;
            case 0x04000201: memory->gba->interrupt.enable.b[1] = value & 0x3F; break;
            // Interrupt Flag
            case 0x04000202: memory->gba->interrupt.flag.b[0] &= ~value; break;
            case 0x04000203: memory->gba->interrupt.flag.b[1] &= ~(value & 0x3F); break;

            // WaitState Control
            case 0x04000204: gba_memory_write_waitstate_control_lsb(memory,value); break;
            case 0x04000205: gba_memory_write_waitstate_control_msb(memory,value); break;
            case 0x04000206: break;
            case 0x04000207: break;

            // Interrupt Master Enable
            case 0x04000208: memory->gba->interrupt.master_enabled = (value & 0x01) != 0x00; break;
            case 0x04000209: break;
            case 0x0400020A: break;
            case 0x0400020B: break;

            // Post Flag / Debug Control
            case 0x04000300: break;
            // Low Power Mode Control
            case 0x04000301: break;
            case 0x04000302: break;
            case 0x04000303: break;

            default:{
                address &= 0xFFFF;
                switch(address){
                    case 0x0800: gba_memory_write_control_lsb(&memory->control,value); break;
                    case 0x0801: break;
                    case 0x0802: break;
                    case 0x0803: gba_memory_write_control_msb(&memory->control,value); break;
                }
                break;
            }
        }
    }
}

static uint8_t gba_memory_read_register_raw(gba_memory_t* memory,uint32_t address){
    // LCD I/O Registers
    if(address < 0x04000060){
        return gba_ppu_read_register_raw(&memory->gba->ppu,address);
    }
    // Sound Registers
    else if(address < 0x040000B0){
        return gba_apu_read_register_raw(&memory->gba->apu,address);
    }
    // DMA Registers
    else if(address < 0x04000100){
        return gba_dma_read_register_raw(&memory->gba->dma,address);
    }
    // Timer Registers
    else if(address < 0x04000120){
        return gba_timer_read_register_raw(&memory->gba->timer,address);
    }
    // Serial Registers 1
    else if(address < 0x04000130){
        return gba_serial_read_register1_raw(&memory->gba->serial,address);
    }
    // Keypad Registers
    else if(address < 0x04000134){
        return gba_keypad_read_register_raw(&memory->gba->keypad,address);
    }
    // Serial Registers 2
    else if(address < 0x04000200){
        return gba_serial_read_register2_raw(&memory->gba->serial,address);
    }
    // Others Registers
    else{
        uint8_t value = memory->bus.b[address & 0x03];

        switch(address){
            // Interrupt Enable
            case 0x04000200: value = memory->gba->interrupt.enable.b[0]; break;
            case 0x04000201: value = memory->gba->interrupt.enable.b[1]; break;
            // Interrupt Flag
            case 0x04000202: value = memory->gba->interrupt.flag.b[0]; break;
            case 0x04000203: value = memory->gba->interrupt.flag.b[1]; break;

            // WaitState Control
            case 0x04000204: value = gba_memory_read_waitstate_control_lsb(&memory->waitstate_control); break;
            case 0x04000205: value = gba_memory_read_waitstate_control_msb(&memory->waitstate_control); break;
            case 0x04000206: value = 0x00; break;
            case 0x04000207: value = 0x00; break;

            // Interrupt Master Enable
            case 0x04000208: value = memory->gba->interrupt.master_enabled; break;
            case 0x04000209: value = 0x00; break;
            case 0x0400020A: value = 0x00; break;
            case 0x0400020B: value = 0x00; break;

            // Post Flag / Debug Control
            case 0x04000300: break;
            // Low Power Mode Control
            case 0x04000301: break;
            case 0x04000302: break;
            case 0x04000303: break;

            default:{
                address &= 0xFFFF;
                switch(address){
                    case 0x0800: value = gba_memory_read_control_lsb(&memory->control); break;
                    case 0x0801: value = 0x00; break;
                    case 0x0802: value = 0x00; break;
                    case 0x0803: value = gba_memory_read_control_msb(&memory->control); break;
                }
                break;
            }
        }

        return value;
    }
}


void gba_memory_write_register_byte(gba_memory_t* memory,uint8_t value,uint32_t address){
    gba_clock(memory->gba,1);

    gba_memory_write_register_raw(memory,value,address);
}

gba_word_t gba_memory_read_register_byte(gba_memory_t* memory,uint32_t address){
    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = gba_memory_read_register_raw(memory,address);
    value.b[1] = value.b[0];
    value.b[2] = value.b[0];
    value.b[3] = value.b[0];

    return value;
}


void gba_memory_write_register_halfword(gba_memory_t* memory,gba_halfword_t value,uint32_t address){
    address &= ~0x01;

    gba_clock(memory->gba,1);

    gba_memory_write_register_raw(memory,value.b[0],address | 0x00);
    gba_memory_write_register_raw(memory,value.b[1],address | 0x01);
}

gba_word_t gba_memory_read_register_halfword(gba_memory_t* memory,uint32_t address){
    address &= ~0x01;

    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = gba_memory_read_register_raw(memory,address | 0x00);
    value.b[1] = gba_memory_read_register_raw(memory,address | 0x01);
    value.b[2] = value.b[0];
    value.b[3] = value.b[1];

    return value;
}


void gba_memory_write_register_word(gba_memory_t* memory,gba_word_t value,uint32_t address){
    address &= ~0x03;

    gba_clock(memory->gba,1);

    gba_memory_write_register_raw(memory,value.b[0],address | 0x00);
    gba_memory_write_register_raw(memory,value.b[1],address | 0x01);
    gba_memory_write_register_raw(memory,value.b[2],address | 0x02);
    gba_memory_write_register_raw(memory,value.b[3],address | 0x03);
}

gba_word_t gba_memory_read_register_word(gba_memory_t* memory,uint32_t address){
    address &= ~0x03;

    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = gba_memory_read_register_raw(memory,address | 0x00);
    value.b[1] = gba_memory_read_register_raw(memory,address | 0x01);
    value.b[2] = gba_memory_read_register_raw(memory,address | 0x02);
    value.b[3] = gba_memory_read_register_raw(memory,address | 0x03);

    return value;
}



void gba_memory_write_ewram_byte(gba_memory_t* memory,uint8_t value,uint32_t address){
    address &= gba_ewram_address_mask;
    
    gba_clock(memory->gba,3);

    memory->ewram[address] = value;
}

gba_word_t gba_memory_read_ewram_byte(gba_memory_t* memory,uint32_t address){
    address &= gba_ewram_address_mask;

    gba_clock(memory->gba,3);

    gba_word_t value = {0};

    value.b[0] = memory->ewram[address];
    value.b[1] = value.b[0];
    value.b[2] = value.b[0];
    value.b[3] = value.b[0];

    return value;
}


void gba_memory_write_ewram_halfword(gba_memory_t* memory,gba_halfword_t value,uint32_t address){
    address &= gba_ewram_address_mask;
    address &= ~0x01;
    
    gba_clock(memory->gba,3);

    memory->ewram[address | 0x00] = value.b[0];
    memory->ewram[address | 0x01] = value.b[1];
}

gba_word_t gba_memory_read_ewram_halfword(gba_memory_t* memory,uint32_t address){
    address &= gba_ewram_address_mask;
    address &= ~0x01;

    gba_clock(memory->gba,3);

    gba_word_t value = {0};

    value.b[0] = memory->ewram[address | 0x00];
    value.b[1] = memory->ewram[address | 0x01];
    value.b[2] = value.b[0];
    value.b[3] = value.b[1];

    return value;
}


void gba_memory_write_ewram_word(gba_memory_t* memory,gba_word_t value,uint32_t address){
    address &= gba_ewram_address_mask;
    address &= ~0x03;

    gba_clock(memory->gba,3);

    memory->ewram[address | 0x00] = value.b[0];
    memory->ewram[address | 0x01] = value.b[1];

    gba_clock(memory->gba,3);

    memory->ewram[address | 0x02] = value.b[2];
    memory->ewram[address | 0x03] = value.b[3];
}

gba_word_t gba_memory_read_ewram_word(gba_memory_t* memory,uint32_t address){
    address &= gba_ewram_address_mask;
    address &= ~0x03;

    gba_word_t value = {0};

    gba_clock(memory->gba,3);

    value.b[0] = memory->ewram[address | 0x00];
    value.b[1] = memory->ewram[address | 0x01];

    gba_clock(memory->gba,3);

    value.b[2] = memory->ewram[address | 0x02];
    value.b[3] = memory->ewram[address | 0x03];

    return value;
}



void gba_memory_write_iwram_byte(gba_memory_t* memory,uint8_t value,uint32_t address){
    address &= gba_iwram_address_mask;

    gba_clock(memory->gba,1);

    memory->iwram[address] = value;
}

gba_word_t gba_memory_read_iwram_byte(gba_memory_t* memory,uint32_t address){
    address &= gba_iwram_address_mask;

    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = memory->iwram[address];
    value.b[1] = value.b[0];
    value.b[2] = value.b[0];
    value.b[3] = value.b[0];

    return value;
}


void gba_memory_write_iwram_halfword(gba_memory_t* memory,gba_halfword_t value,uint32_t address){
    address &= gba_iwram_address_mask;
    address &= ~0x01;

    gba_clock(memory->gba,1);

    memory->iwram[address | 0x00] = value.b[0];
    memory->iwram[address | 0x01] = value.b[1];
}

gba_word_t gba_memory_read_iwram_halfword(gba_memory_t* memory,uint32_t address){
    address &= gba_iwram_address_mask;
    address &= ~0x01;

    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = memory->iwram[address | 0x00];
    value.b[1] = memory->iwram[address | 0x01];
    value.b[2] = value.b[0];
    value.b[3] = value.b[1];

    return value;
}


void gba_memory_write_iwram_word(gba_memory_t* memory,gba_word_t value,uint32_t address){
    address &= gba_iwram_address_mask;
    address &= ~0x03;

    gba_clock(memory->gba,1);

    memory->iwram[address | 0x00] = value.b[0];
    memory->iwram[address | 0x01] = value.b[1];
    memory->iwram[address | 0x02] = value.b[2];
    memory->iwram[address | 0x03] = value.b[3];
}

gba_word_t gba_memory_read_iwram_word(gba_memory_t* memory,uint32_t address){
    address &= gba_iwram_address_mask;
    address &= ~0x03;

    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = memory->iwram[address | 0x00];
    value.b[1] = memory->iwram[address | 0x01];
    value.b[2] = memory->iwram[address | 0x02];
    value.b[3] = memory->iwram[address | 0x03];

    return value;
}



gba_word_t gba_memory_read_unused_byte(gba_memory_t* memory,uint32_t address){
    address &= 0x03;

    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = memory->bus.b[address];
    value.b[1] = value.b[0];
    value.b[2] = value.b[0];
    value.b[3] = value.b[0];

    return value;
}

gba_word_t gba_memory_read_unused_halfword(gba_memory_t* memory,uint32_t address){
    address &= 0x03;
    address &= ~0x01;

    gba_clock(memory->gba,1);

    gba_word_t value = {0};

    value.b[0] = memory->bus.b[address | 0x00];
    value.b[1] = memory->bus.b[address | 0x01];
    value.b[2] = value.b[0];
    value.b[3] = value.b[1];

    return value;
}

gba_word_t gba_memory_read_unused_word(gba_memory_t* memory,uint32_t address){
    gba_unused(address);

    gba_clock(memory->gba,1);

    return memory->bus;
}



void gba_memory_reset(gba_memory_t* memory){
    memset(memory->ewram,0x00,sizeof(memory->ewram));
    memset(memory->iwram,0x00,sizeof(memory->iwram));

    memset(&memory->waitstate_control,0x00,sizeof(memory->waitstate_control));
    gba_memory_update_waitstate_table(memory);
    
    gba_memory_control_t* control = &memory->control;
    control->wram_disabled = false;
    control->unknown0 = 0x00;
    control->cgb_bootrom_disabled = false;
    control->ewram_enabled = true;
    control->wram_waitstate = 0x0D;
    control->unknown1 = 0x00;

    memset(memory->prefetch_buffer,0x00,sizeof(memory->prefetch_buffer));

    memory->bus.v = 0x00;
}