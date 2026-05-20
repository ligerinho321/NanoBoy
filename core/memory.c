#include "./memory.h"
#include "./gb.h"

void gb_memory_init(gb_memory_t* memory,gb_t* gb){
    memory->gb = gb;

    //VRAM
    memory->vram_handler = (gb_memory_handler_t){
        gb_memory_write_vram,
        gb_memory_read_vram,
        memory
    };

    memory->vbk_register_handler = (gb_memory_handler_t){
        gb_memory_write_vbk_register,
        gb_memory_read_vbk_register,
        memory
    };

    //WRAM
    memory->wram0_handler = (gb_memory_handler_t){
        gb_memory_write_wram0,
        gb_memory_read_wram0,
        memory
    };

    memory->wram1_handler = (gb_memory_handler_t){
        gb_memory_write_wram1,
        gb_memory_read_wram1,
        memory
    };

    memory->wbk_register_handler = (gb_memory_handler_t){
        gb_memory_write_wbk_register,
        gb_memory_read_wbk_register,
        memory
    };

    //OAM
    memory->oam_handler = (gb_memory_handler_t){
        gb_memory_write_oam,
        gb_memory_read_oam,
        memory
    };

    //HRAM
    memory->hram_handler = (gb_memory_handler_t){
        gb_memory_write_hram,
        gb_memory_read_hram,
        memory
    };
}


void gb_memory_write(gb_memory_t* memory,uint8_t value,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];
    if(handler && handler->write){
        handler->write(handler->data,value,address);
    }
}

uint8_t gb_memory_read(gb_memory_t* memory,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];
    if(handler && handler->read){
        return handler->read(handler->data,address);
    }
    return 0xFF;
}


void gb_memory_write_vram(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->vram[memory->vram_bank][address & 0x1FFF] = value;
}

uint8_t gb_memory_read_vram(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return memory->vram[memory->vram_bank][address & 0x1FFF];
}

void gb_memory_map_vram(gb_memory_t* memory){
    gb_memory_map(memory,memory->vram_handler,0x8000,0x9FFF);
}


void gb_memory_write_wram0(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->wram[0][address & 0x0FFF] = value;
}

uint8_t gb_memory_read_wram0(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return memory->wram[0][address & 0x0FFF];
}

void gb_memory_write_wram1(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->wram[!memory->wram_bank ? 0x01 : memory->wram_bank][address & 0x0FFF] = value;
}

uint8_t gb_memory_read_wram1(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return memory->wram[!memory->wram_bank ? 0x01 : memory->wram_bank][address & 0x0FFF];
}

void gb_memory_map_wram(gb_memory_t* memory){
    gb_memory_map(memory,memory->wram0_handler,0xC000,0xCFFF);
    gb_memory_map(memory,memory->wram0_handler,0xE000,0xEFFF);

    gb_memory_map(memory,memory->wram1_handler,0xD000,0xDFFF);
    gb_memory_map(memory,memory->wram1_handler,0xF000,0xFDFF);
}


void gb_memory_write_oam(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->oam[address & 0xFF] = value;
}

uint8_t gb_memory_read_oam(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return memory->oam[address & 0xFF];
}

void gb_memory_map_oam(gb_memory_t* memory){
    gb_memory_map(memory,memory->oam_handler,0xFE00,0xFE9F);
}


void gb_memory_write_hram(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->hram[address & 0x7F] = value;
}

uint8_t gb_memory_read_hram(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return memory->hram[address & 0x7F];
}

void gb_memory_map_hram(gb_memory_t* memory){
    gb_memory_map(memory,memory->hram_handler,0xFF80,0xFFFE);
}


void gb_memory_write_bank_register(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    
    if(value & 0x01){
        gb_memory_map(memory,memory->rom0_handler,0x0000,0x00FF);
        
        if(memory->gb->type == gb_cgb){
            gb_memory_map(memory,memory->rom0_handler,0x0200,0x0BFF);
        }
    }
}


void gb_memory_write_vbk_register(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->vram_bank = value & 0x01;
}

uint8_t gb_memory_read_vbk_register(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return 0xFE | (memory->vram_bank & 0x01);
}


void gb_memory_write_wbk_register(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->wram_bank = value & 0x07;
}

uint8_t gb_memory_read_wbk_register(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return 0xF8 | (memory->vram_bank & 0x07);
}


void gb_memory_clear_bus(gb_memory_t* memory){
    memset(memory->bus,0,sizeof(memory->bus));
}

void gb_memory_map_general_registers(gb_memory_t* memory){
    
    gb_joypad_map_registers(&memory->gb->joypad);
    
    gb_serial_map_registers(&memory->gb->serial);
    
    gb_timer_map_registers(&memory->gb->timer);
    
    gb_interrupt_map_registers(&memory->gb->interrupt);
    
    gb_apu_map_registers(&memory->gb->apu);
    
    gb_ppu_map_registers(&memory->gb->ppu);

    gb_dma_oam_map_registers(&memory->gb->dma);
}

void gb_memory_map_cgb_registers(gb_memory_t* memory){
    gb_memory_handler_t** bus = memory->bus;

    //KEY0
    bus[0xFF4C] = &memory->gb->key0_register_handler;
    
    //KEY1
    bus[0xFF4D] = &memory->gb->key1_register_handler;
    
    //VBK
    bus[0xFF4F] = &memory->vbk_register_handler;
    
    //VRAM DMA
    gb_dma_vram_map_registers(&memory->gb->dma);

    //Palette
    gb_palette_map_cgb_registers(&memory->gb->palette);
    
    //OPRI
    bus[0xFF6C] = &memory->gb->opri_register_handler;

    //WBK
    bus[0xFF70] = &memory->wbk_register_handler;

    gb_apu_map_pcm_registers(&memory->gb->apu);
}


void gb_memory_reset(gb_memory_t* memory){

}