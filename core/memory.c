#include "memory.h"
#include "gb.h"

void gb_memory_init(gb_memory_t* memory,gb_t* gb){
    memory->gb = gb;

    memory->empty_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_empty,
        gb_memory_read_empty,
        NULL
    };

    gb_memory_unmap_in_range(memory,0x0000,0xFFFF);

    memory->wram0_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_wram0,
        gb_memory_read_wram0,
        memory
    };

    memory->wram1_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_wram1,
        gb_memory_read_wram1,
        memory
    };

    memory->hram_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_hram,
        gb_memory_read_hram,
        memory
    };

    memory->wbk_register_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_wbk_register,
        gb_memory_read_wbk_register,
        memory
    };
}


void gb_memory_write_empty(void* data,uint8_t value,uint16_t address){
    gb_unused(data);
    gb_unused(value);
    gb_unused(address);
}

uint8_t gb_memory_read_empty(void* data,uint16_t address){
    gb_unused(data);
    gb_unused(address);
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

void gb_memory_add_cheat_code(gb_t* gb,gb_cheat_code_t* code){
    gb_memory_t* memory = &gb->memory;
    gb_list_add_element(memory->codes[code->address],code,gb_cheat_code_t);
}

void gb_memory_remove_cheat_code(gb_t* gb,gb_cheat_code_t* code){
    gb_memory_t* memory = &gb->memory;
    gb_list_remove_element(memory->codes[code->address],code,gb_cheat_code_t);
}


void gb_memory_cpu_write(gb_memory_t* memory,uint8_t value,uint16_t address){
    gb_memory_descriptor_t* descriptor = memory->bus[address];

    if(memory->gb->dma.oam_state != gb_oam_dma_state_transfer || !gb_oam_dma_bus_conflict(&memory->gb->dma,address)){
        
        descriptor->write(descriptor->data,value,address);

        if(memory->gb->event_manager.enabled){
            gb_event_manager_io(memory->gb,gb_event_write_flag,value,address);
        }
    }
}

uint8_t gb_memory_cpu_read(gb_memory_t* memory,uint16_t address){
    gb_memory_descriptor_t* descriptor = memory->bus[address];
    
    uint8_t value = 0xFF;

    if(memory->gb->dma.oam_state != gb_oam_dma_state_transfer || !gb_oam_dma_bus_conflict(&memory->gb->dma,address)){
        
        value = descriptor->read(descriptor->data,address);

        gb_memory_apply_cheat(memory,&value,address);

        if(memory->gb->event_manager.enabled){
            gb_event_manager_io(memory->gb,gb_event_read_flag,value,address);
        }
    }
    else{
        value = memory->gb->dma.oam_byte;
    }

    return value;
}


uint8_t gb_memory_oam_dma_read(gb_memory_t* memory,uint16_t address){
    gb_memory_descriptor_t* descriptor = memory->bus[address];

    uint8_t value = descriptor->read(descriptor->data,address);

    gb_memory_apply_cheat(memory,&value,address);

    if(memory->gb->event_manager.enabled){
        gb_event_manager_io(memory->gb,gb_event_read_flag,value,address);
    }

    return value;
}


void gb_memory_vram_dma_write(gb_memory_t* memory,uint8_t value,uint16_t address){
    gb_memory_descriptor_t* descriptor = memory->bus[address];

    descriptor->write(descriptor->data,value,address);

    if(memory->gb->event_manager.enabled){
        gb_event_manager_io(memory->gb,gb_event_write_flag,value,address);
    }
}

uint8_t gb_memory_vram_dma_read(gb_memory_t* memory,uint16_t address){

    if((address >= 0x8000 && address <= 0x9FFF) || address >= 0xE000){
        return 0xFF;
    }

    gb_memory_descriptor_t* descriptor = memory->bus[address];

    uint8_t value = descriptor->read(descriptor->data,address);
    
    gb_memory_apply_cheat(memory,&value,address);

    if(memory->gb->event_manager.enabled){
        gb_event_manager_io(memory->gb,gb_event_read_flag,value,address);
    }
    
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
    gb_memory_map_in_range(memory,&memory->wram0_descriptor,0xC000,0xCFFF);
    gb_memory_map_in_range(memory,&memory->wram0_descriptor,0xE000,0xEFFF);

    gb_memory_map_in_range(memory,&memory->wram1_descriptor,0xD000,0xDFFF);
    gb_memory_map_in_range(memory,&memory->wram1_descriptor,0xF000,0xFDFF);
}

size_t gb_memory_wram_absolute_address(gb_memory_t* memory,uint16_t relative_address){
    if((relative_address & 0x1FFF) < 0x1000){
        return relative_address & 0x0FFF;
    }
    else{
        return ((memory->wram_bank ? memory->wram_bank : 0x01) << 0x0C) | (relative_address & 0x0FFF);
    }
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
    gb_memory_map_in_range(memory,&memory->hram_descriptor,0xFF80,0xFFFE);
}

size_t gb_memory_hram_absolute_address(gb_memory_t* memory,uint16_t relative_address){
    gb_unused(memory);
    return relative_address & 0x7F;
}


#define gb_memory_update_wram_bank_ptr(memory)\
    (memory)->wram_bank_ptr = (memory)->wram + (((memory)->wram_bank ? (memory)->wram_bank : 0x01) << 0x0C);


void gb_memory_write_wbk_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_memory_t* memory = (gb_memory_t*)data;

    memory->wram_bank = value & 0x07;

    gb_memory_update_wram_bank_ptr(memory);
}

uint8_t gb_memory_read_wbk_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_memory_t* memory = (gb_memory_t*)data;

    return 0xF8 | (memory->wram_bank & 0x07);
}


void gb_memory_reset(gb_memory_t* memory){
    
    memset(memory->wram,0x00,sizeof(memory->wram));
    memory->wram_bank = 0x00;
    gb_memory_update_wram_bank_ptr(memory);

    memset(memory->hram,0x00,sizeof(memory->hram));

    if(memory->gb->is_cgb){
        memory->hram[0x7E] = 0xC0;
    }
    else{
        memory->hram[0x7E] = 0x4C;
    }
}

void gb_memory_skip_boot(gb_memory_t* memory){

    if(memory->gb->is_cgb){
        
        if(memory->gb->cgb_mode){

            const uint8_t hram[127] = {
                0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
                0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 
                0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
                0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 
                0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
                0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x71, 0x02, 0x4D, 0x01, 0xC1, 0xFF, 
                0x0D, 0x00, 0xD3, 0x05, 0xF9, 0x00, 0xC0,
            };

            memcpy(memory->hram,hram,sizeof(memory->hram));
        }
        else{
            const uint8_t hram[127] = {
                0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
                0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 
                0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
                0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 
                0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
                0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0x00, 0x00, 0x71, 0x02, 0x4D, 0x01, 0x00, 0xFF, 
                0x0D, 0x00, 0xF5, 0x05, 0xF9, 0x00, 0xC0
            };

            memcpy(memory->hram,hram,sizeof(memory->hram));
        }
    }
    else{
        const uint8_t hram[127] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x39, 0x01, 0x2E, 0x00, 0x4C,
        };

        memcpy(memory->hram,hram,sizeof(memory->hram));
    }
}


void gb_memory_save_state(gb_memory_t* memory,gb_state_t* state){
    gb_state_write_ex(state,memory->wram,sizeof(memory->wram));
    gb_state_write(state,memory->wram_bank);

    gb_state_write_ex(state,memory->hram,sizeof(memory->hram));
}

void gb_memory_load_state(gb_memory_t* memory,gb_state_t* state){
    gb_state_read_ex(state,memory->wram,sizeof(memory->wram));
    gb_state_read(state,memory->wram_bank);
    gb_memory_update_wram_bank_ptr(memory);

    gb_state_read_ex(state,memory->hram,sizeof(memory->hram));
}