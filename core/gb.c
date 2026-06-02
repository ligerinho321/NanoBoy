#include "gb.h"

void gb_init(gb_t* gb){

    memset(gb,0x00,sizeof(gb_t));

    gb->type = gb_dmg;
    gb->cartridge_inserted = false;
    
    gb_cpu_init(&gb->cpu,gb);
    gb_ppu_init(&gb->ppu,gb);
    gb_apu_init(&gb->apu,gb);
    gb_joypad_init(&gb->joypad,gb);
    gb_interrupt_init(&gb->interrupt,gb);
    gb_timer_init(&gb->timer,gb);
    gb_dma_init(&gb->dma,gb);
    gb_palette_init(&gb->palette,gb);
    gb_serial_init(&gb->serial,gb);
    gb_boot_init(&gb->boot,gb);
    gb_memory_init(&gb->memory,gb);
    gb_cartridge_init(&gb->cartridge,gb);

    gb->key0_register_handler = (gb_memory_handler_t){
        gb_write_key0_register,
        NULL,
        gb
    };

    gb->key1_register_handler = (gb_memory_handler_t){
        gb_write_key1_register,
        gb_read_key1_register,
        gb
    };

    gb->opri_register_handler = (gb_memory_handler_t){
        gb_write_opri_register,
        gb_read_opri_register,
        gb
    };
}


bool gb_insert_cartridge(gb_t* gb,const char* path){
    if(!gb_cartridge_load(&gb->cartridge,path)) return false;
    gb_reset(gb);
    gb->cartridge_inserted = true;
    return true;
}

void gb_remove_cartridge(gb_t* gb){
    gb_cartridge_clear(&gb->cartridge);
    gb->cartridge_inserted = false;
}


void gb_add_callback(gb_t* gb,gb_callback_handler_t* callback){
    if(gb->callback_handles != NULL){
        gb_callback_handler_t* ptr = gb->callback_handles;
        while(ptr->next != NULL){
            if(ptr == callback) return;
            ptr = ptr->next;
        }
        ptr->next = callback;
        callback->next = NULL;
    }
    else{
        gb->callback_handles = callback;
    }
}

void gb_remove_callback(gb_t* gb,gb_callback_handler_t* callback){
    if(gb->callback_handles == NULL) return;

    if(gb->callback_handles == callback){
        gb->callback_handles = callback->next;
    }
    else{
        gb_callback_handler_t* prev = NULL;
        gb_callback_handler_t* current = gb->callback_handles;
        while(current->next != NULL){
            prev = current;
            current = current->next;
            if(current == callback){
                prev->next = current->next;
            }
        }
    }
}

void gb_set_joypad_callback(gb_t* gb,gb_joypad_callback_t callback,void* data){
    gb->joypad.callback = callback;
    gb->joypad.data = data;
}

void gb_master_clock(gb_t* gb){
    gb->cycles++;
    
    if((gb->cycles & 0x03) == 0x03){
        gb_dma_oam_clock(&gb->dma);
        gb_timer_clock(&gb->timer);
    }

    if(!gb->double_speed || (gb->cycles & 0x01)){
        gb->ppu.clock(&gb->ppu);
    }
}


void gb_write_key0_register(void* data,uint8_t value,uint16_t address){
    gb_t* gb = (gb_t*)data;
    gb->cgb_mode = !(value & 0x0C);
    if(!gb->cgb_mode){
        gb_memory_unmap_cgb_registers(&gb->memory);
    }
}


void gb_write_key1_register(void* data,uint8_t value,uint16_t address){
    gb_t* gb = (gb_t*)data;
    gb->speed_switch_needed = value & 0x01;
}

uint8_t gb_read_key1_register(void* data,uint16_t address){
    gb_t* gb = (gb_t*)data;
    return (gb->double_speed ? 0x80 : 0x00) | 0x7E | gb->speed_switch_needed;
}


void gb_write_opri_register(void* data,uint8_t value,uint16_t address){
    gb_t* gb = (gb_t*)data;
    gb->obj_priority_mode = value & 0x01;
}

uint8_t gb_read_opri_register(void* data,uint16_t address){
    gb_t* gb = (gb_t*)data;
    return 0xFE | gb->obj_priority_mode;
}


void gb_reset(gb_t* gb){
    gb->cycles = (uint64_t)-1;

    gb->double_speed = false;
    gb->speed_switch_needed = false;
    
    gb_cpu_reset(&gb->cpu);
    gb_ppu_reset(&gb->ppu);
    gb_apu_reset(&gb->apu);
    gb_joypad_reset(&gb->joypad);
    gb_interrupt_reset(&gb->interrupt);
    gb_timer_reset(&gb->timer);
    gb_dma_reset(&gb->dma);
    gb_palette_reset(&gb->palette);
    gb_serial_reset(&gb->serial);
    gb_boot_map(&gb->boot);
    gb_memory_reset(&gb->memory);
    
    if(gb->cartridge.reset){
        gb->cartridge.reset(&gb->cartridge);
    }
}