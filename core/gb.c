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
    
    gb_printer_init(&gb->printer,gb);

    gb_frame_timer_init(&gb->frame_timer);

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


uint32_t gb_get_clock_rate(gb_t* gb){
    return gb_clock_rate << (gb->double_speed ? 0x01 : 0x00); 
}


bool gb_insert_cartridge(gb_t* gb,const char* path){

    gb_remove_cartridge(gb);

    if(!gb_cartridge_load(&gb->cartridge,path)){
        return false;
    }

    gb->cartridge_inserted = true;

    gb_reset(gb);

    if(!gb->paused){
        gb_frame_timer_start(&gb->frame_timer);
    }

    return true;
}

void gb_remove_cartridge(gb_t* gb){
    gb_cartridge_clear(&gb->cartridge);
    
    gb->cartridge_inserted = false;

    gb_frame_timer_stop(&gb->frame_timer);
}


void gb_thread_safe_connect_printer(gb_t* gb,gb_printer_callback_t callback,void* userdata){
    gb_thread_stop(gb);
    gb_connect_printer(gb,callback,userdata);
    gb_thread_start(gb);
}

void gb_thread_safe_disconnect_printer(gb_t* gb){
    gb_thread_stop(gb);
    gb_disconnect_printer(gb);
    gb_thread_start(gb);
}


void gb_thread_safe_set_joypad_callback(gb_t* gb,gb_joypad_callback_t callback,void* data){
    gb_thread_stop(gb);
    gb_joypad_set_callback(&gb->joypad,callback,data);
    gb_thread_start(gb);
}

void gb_thread_safe_remove_joypad_callback(gb_t* gb){
    gb_thread_stop(gb);
    gb_joypad_remove_callback(&gb->joypad);
    gb_thread_start(gb);
}


void gb_thread_safe_set_apu_callback(gb_t* gb,gb_apu_callback_t callback,void* data){
    gb_thread_stop(gb);
    gb_apu_set_callback(&gb->apu,callback,data);
    gb_thread_start(gb);
}

void gb_thread_safe_remove_apu_callback(gb_t* gb){
    gb_thread_stop(gb);
    gb_apu_remove_callback(&gb->apu);
    gb_thread_start(gb);
}


void gb_thread_safe_add_ppu_handler(gb_t* gb,gb_ppu_handler_t* handler){
    gb_thread_stop(gb);
    gb_ppu_add_handler(&gb->ppu,handler);
    gb_thread_start(gb);
}

void gb_thread_safe_remove_ppu_handler(gb_t* gb,gb_ppu_handler_t* handler){
    gb_thread_stop(gb);
    gb_ppu_remove_handler(&gb->ppu,handler);
    gb_thread_start(gb);
}


void gb_thread_safe_set_speed(gb_t* gb,float new_speed){
    if(new_speed == gb->speed || new_speed < gb_speed_min || new_speed > gb_speed_max) return;
    
    gb_thread_stop(gb);

    gb->speed = new_speed;
    
    gb_apu_update_rates(&gb->apu);
    
    gb_thread_start(gb);
}

void gb_thread_safe_set_execution_mode(gb_t* gb,bool multi_thread){
    if(gb->multi_thread == multi_thread) return;
    
    gb_thread_stop(gb);

    gb->multi_thread = multi_thread;
    
    gb_thread_start(gb);
}

void gb_thread_safe_set_paused(gb_t* gb,bool paused){
    if(gb->paused == paused) return;
    
    gb_thread_stop(gb);
    
    gb->paused = paused;

    if(gb->paused){
        gb_frame_timer_stop(&gb->frame_timer);
    }
    else{
        gb_frame_timer_start(&gb->frame_timer);
    }
    
    gb_thread_start(gb);
}


void gb_thread_safe_reset(gb_t *gb){
    gb_thread_stop(gb);
    gb_reset(gb);
    gb_thread_start(gb);
}


void gb_connect_printer(gb_t* gb,gb_printer_callback_t callback,void* userdata){
    gb_serial_set_callback(&gb->serial,gb_printer_receive_bit,&gb->printer);
    gb_printer_set_callback(&gb->printer,callback,userdata);
}

void gb_disconnect_printer(gb_t* gb){
    gb_serial_remove_callback(&gb->serial);
    gb_printer_remove_callback(&gb->printer);
}


void gb_half_machine_cycle(gb_t* gb){
    gb->cycle += 2;

    int cycles = gb->double_speed ? 1 : 2;

    gb->apu.cycles += cycles;
    
    gb_printer_clock(&gb->printer,cycles);

    gb_ppu_clock(&gb->ppu,cycles);

    if((gb->cycle & 0x03) == 0x03){

        gb_timer_clock(&gb->timer);

        gb_oam_dma_clock(&gb->dma);

        gb_serial_clock(&gb->serial);
    }
}

void gb_machine_cycle(gb_t* gb){
    gb->cycle += 4;

    int cycles = gb->double_speed ? 2 : 4;

    gb->apu.cycles += cycles;
    
    gb_printer_clock(&gb->printer,cycles);

    gb_ppu_clock(&gb->ppu,cycles);

    gb_timer_clock(&gb->timer);

    gb_oam_dma_clock(&gb->dma);

    gb_serial_clock(&gb->serial);
}


void gb_execute_frame(gb_t* gb){
    if(!gb->cartridge_inserted || gb->multi_thread || gb->paused) return;

    uint64_t frame = gb->ppu.frame_count;

    while(frame == gb->ppu.frame_count){
        gb_cpu_execute(&gb->cpu);
    }

    gb_frame_timer_clock(&gb->frame_timer);
}


static void gb_execute(gb_t* gb){
    gb_ppu_t* ppu = &gb->ppu;
    gb_cpu_t* cpu = &gb->cpu;
    gb_frame_timer_t* frame_timer = &gb->frame_timer;
    uint64_t frame;
    
    while(gb_atomic_load_explicit(&gb->thread_running,gb_memory_order_relaxed)){
        
        frame = ppu->frame_count;

        while(frame == ppu->frame_count){
            gb_cpu_execute(cpu);
        }
        
        gb_frame_timer_clock(frame_timer);
    }
}

#ifdef _WIN32
DWORD WINAPI gb_thread_function(void* data){
    gb_execute((gb_t*)data);
    return 0;
}
#else
void* gb_thread_function(void* data){
    gb_execute((gb_t*)data);
    pthread_exit(NULL);
}
#endif


void gb_thread_stop(gb_t* gb){
    if(!gb->cartridge_inserted || gb->paused) return;

    if(!gb->multi_thread || !gb_atomic_load_explicit(&gb->thread_running,gb_memory_order_relaxed)) return;

    gb_atomic_store_explicit(&gb->thread_running,false,gb_memory_order_relaxed);

#ifdef _WIN32
    WaitForSingleObject(gb->thread_handle,INFINITE);
    CloseHandle(gb->thread_handle);
    gb->thread_handle = NULL;
#else
    pthread_join(gb->thread_id,NULL);
#endif
}

void gb_thread_start(gb_t* gb){
    if(!gb->cartridge_inserted || gb->paused) return;

    if(!gb->multi_thread || gb_atomic_load_explicit(&gb->thread_running,gb_memory_order_relaxed)) return;

    gb_atomic_store_explicit(&gb->thread_running,true,gb_memory_order_relaxed);

#ifdef _WIN32
    gb->thread_handle = CreateThread(NULL,0,gb_thread_function,gb,0,NULL);
    if(!gb->thread_handle){
        gb_atomic_store_explicit(&gb->thread_running,false,gb_memory_order_relaxed);
        gb_printf_error("CreateThread failed\n");
    }
#else
    if(pthread_create(&gb->thread_id,NULL,gb_thread_function,gb) != 0){
        gb_atomic_store_explicit(&gb->thread_running,false,gb_memory_order_relaxed);
        gb_printf_error("pthread_create failed");
    }
#endif
}


void gb_switch_speed(gb_t* gb){
    gb_apu_run(&gb->apu);

    gb_cartridge_update_rtc_timer(&gb->cartridge);
    
    gb_timer_set_div(&gb->timer,0);

    gb->speed_switch_needed = false;
    gb->double_speed = !gb->double_speed;

    gb->timer.apu_div_bit = gb->double_speed ? 0x2000 : 0x1000;
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

    gb_cartridge_reset(&gb->cartridge);

    gb_printer_reset(&gb->printer);
}


void gb_delete(gb_t* gb){
    gb_apu_free(&gb->apu);
    free(gb);
}