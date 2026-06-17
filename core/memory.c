#include "memory.h"
#include "gb.h"

void gb_memory_init(gb_memory_t* memory,gb_t* gb){
    memory->gb = gb;

    gb_memory_map_wram(memory);

    gb_memory_map_hram(memory);

    memory->wbk_register_handler = (gb_memory_handler_t){
        gb_memory_write_wbk_register,
        gb_memory_read_wbk_register,
        memory
    };
}


void gb_memory_write(gb_memory_t* memory,uint8_t value,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];

    if(!memory->gb->dma.oam_running || !gb_oam_dma_bus_conflict(&memory->gb->dma,address)){
        if(handler && handler->write){
            handler->write(handler->data,value,address);
        }
    }
}

uint8_t gb_memory_read(gb_memory_t* memory,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];

    if(!memory->gb->dma.oam_running || !gb_oam_dma_bus_conflict(&memory->gb->dma,address)){
        if(handler && handler->read){
            return handler->read(handler->data,address);
        }
    }
    else{
        return memory->gb->dma.oam_byte;
    }

    return 0xFF;
}


uint8_t gb_memory_oam_dma_read(gb_memory_t* memory,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];

    if(handler && handler->read){
        return handler->read(handler->data,address);
    }

    return 0xFF;
}


void gb_memory_vram_dma_write(gb_memory_t* memory,uint8_t value,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];

    if(handler && handler->write){
        handler->write(handler->data,value,address);
    }
}

uint8_t gb_memory_vram_dma_read(gb_memory_t* memory,uint16_t address){

    if((address >= 0x8000 && address <= 0x9FFF) || address >= 0xE000){
        return 0xFF;
    }

    gb_memory_handler_t* handler = memory->bus[address];

    if(handler && handler->read){
        return handler->read(handler->data,address);
    }

    return 0xFF;
}


void gb_memory_map(gb_memory_t* memory,gb_memory_handler_t* handler,uint32_t start,uint32_t end){
    gb_memory_handler_t** bus = memory->bus;
    
    while(start <= end) bus[start++] = handler;
}


void gb_memory_write_wram0(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->wram[address & 0x0FFF] = value;
}

uint8_t gb_memory_read_wram0(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return memory->wram[address & 0x0FFF];
}

void gb_memory_write_wram1(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->wram_bank_ptr[address & 0x0FFF] = value;
}

uint8_t gb_memory_read_wram1(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return memory->wram_bank_ptr[address & 0x0FFF];
}

void gb_memory_map_wram(gb_memory_t* memory){

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

    gb_memory_map(memory,&memory->wram0_handler,0xC000,0xCFFF);
    gb_memory_map(memory,&memory->wram0_handler,0xE000,0xEFFF);

    gb_memory_map(memory,&memory->wram1_handler,0xD000,0xDFFF);
    gb_memory_map(memory,&memory->wram1_handler,0xF000,0xFDFF);
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

    memory->hram_handler = (gb_memory_handler_t){
        gb_memory_write_hram,
        gb_memory_read_hram,
        memory
    };

    gb_memory_map(memory,&memory->hram_handler,0xFF80,0xFFFE);
}


void gb_memory_write_wbk_register(void* data,uint8_t value,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    memory->wram_bank = value & 0x07;
    if(!memory->wram_bank){
        memory->wram_bank = 0x01;
    }
    memory->wram_bank_ptr = memory->wram + (memory->wram_bank << 0x0C);
}

uint8_t gb_memory_read_wbk_register(void* data,uint16_t address){
    gb_memory_t* memory = (gb_memory_t*)data;
    return 0xF8 | (memory->wram_bank & 0x07);
}


void gb_memory_reset(gb_memory_t* memory){
    memset(memory->wram,0x00,sizeof(memory->wram));
    memory->wram_bank = 0x01;
    memory->wram_bank_ptr = memory->wram + (memory->wram_bank << 0x0C);

    memset(memory->hram,0x00,sizeof(memory->hram));
}