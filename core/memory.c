#include "memory.h"
#include "gb.h"

void gb_memory_init(gb_memory_t* memory,gb_t* gb){
    memory->gb = gb;

    memory->empty_handler = (gb_memory_handler_t){
        gb_memory_write_empty,
        gb_memory_read_empty,
        NULL
    };

    gb_memory_unmap_in_range(memory,0x0000,0xFFFF);

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

    memory->hram_handler = (gb_memory_handler_t){
        gb_memory_write_hram,
        gb_memory_read_hram,
        memory
    };

    memory->wbk_register_handler = (gb_memory_handler_t){
        gb_memory_write_wbk_register,
        gb_memory_read_wbk_register,
        memory
    };
}


void gb_memory_write_empty(void* data,uint8_t value,uint16_t address){
    return;
}

uint8_t gb_memory_read_empty(void* data,uint16_t address){
    return 0xFF;
}


static inline void gb_memory_apply_cheat(gb_memory_t* memory,uint8_t* value,uint16_t address){
    gb_cheat_code_t* code = memory->codes[address];
    while(code != NULL){
        if(*code->enabled && code->address == address && (code->old_value < 0 || code->old_value == *value)){
            *value = code->new_value;
        }
        code = code->next;
    }
}

void gb_memory_add_cheat_code(gb_memory_t* memory,gb_cheat_code_t* code){
    gb_list_add_element(memory->codes[code->address],code,gb_cheat_code_t);
}

void gb_memory_remove_cheat_code(gb_memory_t* memory,gb_cheat_code_t* code){
    gb_list_remove_element(memory->codes[code->address],code,gb_cheat_code_t);
}


void gb_memory_cpu_write(gb_memory_t* memory,uint8_t value,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];

    if(memory->gb->dma.oam_state != gb_oam_dma_state_transfer || !gb_oam_dma_bus_conflict(&memory->gb->dma,address)){
        handler->write(handler->data,value,address);
    }

}

uint8_t gb_memory_cpu_read(gb_memory_t* memory,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];
    
    uint8_t value = 0xFF;

    if(memory->gb->dma.oam_state != gb_oam_dma_state_transfer || !gb_oam_dma_bus_conflict(&memory->gb->dma,address)){
        
        value = handler->read(handler->data,address);

        gb_memory_apply_cheat(memory,&value,address);
    }
    else{
        value = memory->gb->dma.oam_byte;
    }

    return value;
}


uint8_t gb_memory_oam_dma_read(gb_memory_t* memory,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];

    uint8_t value = handler->read(handler->data,address);

    gb_memory_apply_cheat(memory,&value,address);

    return value;
}


void gb_memory_vram_dma_write(gb_memory_t* memory,uint8_t value,uint16_t address){
    gb_memory_handler_t* handler = memory->bus[address];

    handler->write(handler->data,value,address);
}

uint8_t gb_memory_vram_dma_read(gb_memory_t* memory,uint16_t address){

    if((address >= 0x8000 && address <= 0x9FFF) || address >= 0xE000){
        return 0xFF;
    }

    gb_memory_handler_t* handler = memory->bus[address];

    uint8_t value = handler->read(handler->data,address);
    
    gb_memory_apply_cheat(memory,&value,address);

    return value;
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
    gb_memory_map_in_range(memory,&memory->wram0_handler,0xC000,0xCFFF);
    gb_memory_map_in_range(memory,&memory->wram0_handler,0xE000,0xEFFF);

    gb_memory_map_in_range(memory,&memory->wram1_handler,0xD000,0xDFFF);
    gb_memory_map_in_range(memory,&memory->wram1_handler,0xF000,0xFDFF);
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
    gb_memory_map_in_range(memory,&memory->hram_handler,0xFF80,0xFFFE);
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


void gb_memory_save_state(gb_memory_t* memory,gb_state_t* state){
    gb_state_write_ex(state,memory->wram,sizeof(memory->wram));
    gb_state_write(state,memory->wram_bank);

    gb_state_write_ex(state,memory->hram,sizeof(memory->hram));
}

void gb_memory_load_state(gb_memory_t* memory,gb_state_t* state){
    gb_state_read_ex(state,memory->wram,sizeof(memory->wram));
    gb_state_read(state,memory->wram_bank);
    memory->wram_bank_ptr = memory->wram + (memory->wram_bank << 0x0C);

    gb_state_read_ex(state,memory->hram,sizeof(memory->hram));
}