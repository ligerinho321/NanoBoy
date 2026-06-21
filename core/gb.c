#include "gb.h"

gb_t* gb_new(){
    gb_t* gb = (gb_t*)malloc(sizeof(gb_t));
    
    if(!gb){
        gb_printf_errno(malloc);
        return NULL;
    }

    memset(gb,0x00,sizeof(gb_t));

    gb->type = gb_cgb;
    gb->type_pending = gb->type;
    gb->speed = 1.0f;
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

    return gb;
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


void gb_add_ppu_callback(gb_t* gb,gb_ppu_callback_handler_t* callback){
    if(gb->callback_handles != NULL){
        gb_ppu_callback_handler_t* ptr = gb->callback_handles;
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

void gb_remove_ppu_callback(gb_t* gb,gb_ppu_callback_handler_t* callback){
    if(gb->callback_handles == NULL) return;

    if(gb->callback_handles == callback){
        gb->callback_handles = callback->next;
    }
    else{
        gb_ppu_callback_handler_t* prev = NULL;
        gb_ppu_callback_handler_t* current = gb->callback_handles;
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
    gb->joypad.callback_data = data;
}


void gb_set_speed(gb_t* gb,float new_speed){
    if(new_speed < gb_speed_min || new_speed > gb_speed_max) return;
    gb->speed = new_speed;
    gb_apu_update_rates(&gb->apu);
}


void gb_half_machine_cycle(gb_t* gb){
    gb->cycle += 2;
    gb->apu.cycles += gb->double_speed ? 1 : 2;

    gb_ppu_clock(&gb->ppu,gb->double_speed ? 1 : 2);
    
    if((gb->cycle & 0x03) == 0x03){

        gb_timer_clock(&gb->timer);

        gb_oam_dma_clock(&gb->dma);
    }
}

void gb_machine_cycle(gb_t* gb){
    gb->cycle += 4;
    gb->apu.cycles += gb->double_speed ? 2 : 4;
    
    gb_ppu_clock(&gb->ppu,gb->double_speed ? 2 : 4);

    gb_timer_clock(&gb->timer);

    gb_oam_dma_clock(&gb->dma);
}


void gb_write_key0_register(void* data,uint8_t value,uint16_t address){
    gb_t* gb = (gb_t*)data;
    gb->cgb_mode = !(value & 0x0C);
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


void gb_map_cgb_registers(gb_t* gb){
    gb_memory_handler_t** bus = gb->memory.bus;
    //KEY0
    bus[0xFF4C] = &gb->key0_register_handler;
    //KEY1
    bus[0xFF4D] = &gb->key1_register_handler;
    //VBK
    bus[0xFF4F] = &gb->ppu.vbk_register_handler;
    //VRAM DMA
    gb_vram_dma_map_registers(&gb->dma);
    //Palette
    gb_palette_map_cgb_registers(&gb->palette);
    //OPRI
    bus[0xFF6C] = &gb->opri_register_handler;
    //WBK
    bus[0xFF70] = &gb->memory.wbk_register_handler;
    //PCM
    gb_apu_map_pcm_registers(&gb->apu);
}

void gb_unmap_cgb_registers(gb_t* gb){
    gb_memory_handler_t** bus = gb->memory.bus;
    //KEY1
    bus[0xFF4D] = NULL;
    //VBK
    bus[0xFF4F] = NULL;
    //VRAM DMA
    gb_vram_dma_unmap_registers(&gb->dma);
    //Palette
    gb_palette_unmap_cgb_registers(&gb->palette);
    //OPRI
    bus[0xFF6C] = NULL;
    //WBK
    bus[0xFF70] = NULL;
    //PCM
    gb_apu_unmap_pcm_registers(&gb->apu);
}


void gb_reset(gb_t* gb){
    
    if(gb->type_pending != gb->type){
        gb->type = gb->type_pending;
    }

    gb_cpu_reset(&gb->cpu);
    gb_ppu_reset(&gb->ppu);
    gb_apu_reset(&gb->apu,true);
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

    gb->double_speed = false;
    gb->speed_switch_needed = false;
    
    if(gb->type == gb_cgb){
        gb->cgb_mode = true;
        gb->obj_priority_mode = false;
        gb_map_cgb_registers(gb);
    }
    else{
        gb->cgb_mode = false;
        gb->obj_priority_mode = true;
        gb_unmap_cgb_registers(gb);
    }
    
    gb->cycle = (uint64_t)-1;
}


void gb_delete(gb_t* gb){
    gb_apu_free(&gb->apu);
    free(gb);
}