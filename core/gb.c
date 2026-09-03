#include "gb.h"

gb_t* gb_new(){
    gb_t* gb = (gb_t*)malloc(sizeof(gb_t));
    
    if(!gb){
        gb_printf_errno(malloc);
        return NULL;
    }

    memset(gb,0x00,sizeof(gb_t));

    gb->is_cgb = true;
    gb->is_cgb_pending = gb->is_cgb;
    gb->speed = 1.0f;
    
    gb->key0_register_descriptor = (gb_memory_descriptor_t){
        gb_write_key0_register,
        gb_memory_read_empty,
        gb
    };

    gb->key1_register_descriptor = (gb_memory_descriptor_t){
        gb_write_key1_register,
        gb_read_key1_register,
        gb
    };

    gb->opri_register_descriptor = (gb_memory_descriptor_t){
        gb_write_opri_register,
        gb_read_opri_register,
        gb
    };

    gb->undocumented_register_descriptor = (gb_memory_descriptor_t){
        gb_write_undocumented_register,
        gb_read_undocumented_register,
        gb
    };

    gb_cpu_init(&gb->cpu,gb);
    gb_ppu_init(&gb->ppu,gb);
    gb_apu_init(&gb->apu,gb);
    gb_joypad_init(&gb->joypad,gb);
    gb_interrupt_init(&gb->interrupt,gb);
    gb_timer_init(&gb->timer,gb);
    gb_dma_init(&gb->dma,gb);
    gb_palette_init(&gb->palette,gb);
    gb_serial_init(&gb->serial,gb);
    gb_infrared_init(&gb->infrared,gb);
    gb_boot_init(&gb->boot,gb);
    gb_memory_init(&gb->memory,gb);
    gb_cartridge_init(&gb->cartridge,gb);
    gb_printer_init(&gb->printer,gb);
    gb_frame_timer_init(&gb->frame_timer);
    gb_breakpoint_manager_init(&gb->breakpoint_manager,gb);
    gb_event_manager_init(&gb->event_manager,gb);

    gb_map(gb);

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

    gb_cartridge_remove(&gb->cartridge);

    gb->cartridge_inserted = false;

    gb_frame_timer_stop(&gb->frame_timer);
}


void gb_set_speed(gb_t* gb,float new_speed){
    if(new_speed == gb->speed || new_speed < gb_speed_min || new_speed > gb_speed_max) return;

    gb->speed = new_speed;
    
    gb_apu_update_rates(&gb->apu);
}


void gb_pause(gb_t* gb,bool paused){
    if(gb->paused == paused) return;

    gb->paused = paused;

    if(gb->paused){
        gb_frame_timer_stop(&gb->frame_timer);
    }
    else{
        if(gb->breakpoint_manager.enabled){
            gb->breakpoint_manager.last_check_address = gb->cpu.pc;
        }

        gb_frame_timer_start(&gb->frame_timer);
    }
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

    gb->apu.cycle += cycles;

    gb_ppu_clock(&gb->ppu,cycles);

    if(gb->cycle >= gb->timer.next_schedule_event){
        gb_timer_update(&gb->timer);
        gb_timer_schedule_next_event(&gb->timer);
    }

    if(gb->printer.status & gb_printer_currently_printing_status){
        gb_printer_clock(&gb->printer,cycles);
    }

    if(!(gb->cycle & 0x03)){

        if(gb->serial.transfer_enabled){
            gb_serial_clock(&gb->serial);
        }

        if(gb->dma.oam_state != gb_oam_dma_state_none && gb->cpu.state != gb_cpu_halted_state){
            gb_oam_dma_clock(&gb->dma);
        }

        if(gb->dma.vram_hblank_pending && gb->dma.vram_hblank_running && gb->cpu.state != gb_cpu_halted_state){
            gb_vram_hblank_dma(&gb->dma);
        }
    }
}

void gb_machine_cycle(gb_t* gb){
    gb->cycle += 4;

    int cycles = gb->double_speed ? 2 : 4;

    gb->apu.cycle += cycles;

    gb_ppu_clock(&gb->ppu,cycles);
    
    if(gb->cycle >= gb->timer.next_schedule_event){
        gb_timer_update(&gb->timer);
        gb_timer_schedule_next_event(&gb->timer);
    }

    if(gb->printer.status & gb_printer_currently_printing_status){
        gb_printer_clock(&gb->printer,cycles);
    }

    if(gb->serial.transfer_enabled){
        gb_serial_clock(&gb->serial);
    }

    if(gb->dma.oam_state != gb_oam_dma_state_none && gb->cpu.state != gb_cpu_halted_state){
        gb_oam_dma_clock(&gb->dma);
    }

    if(gb->dma.vram_hblank_pending && gb->dma.vram_hblank_running && gb->cpu.state != gb_cpu_halted_state){
        gb_vram_hblank_dma(&gb->dma);
    }
}


void gb_execute_frame(gb_t* gb){
    if(!gb->cartridge_inserted || gb->paused || gb->multi_thread) return;

    uint64_t frame = gb->ppu.frame_count;
    gb_cpu_t* cpu = &gb->cpu;
    gb_ppu_t* ppu = &gb->ppu;
    gb_breakpoint_manager_t* breakpoint_manager = &gb->breakpoint_manager;

    if(!breakpoint_manager->enabled){
        while(frame == ppu->frame_count){
            cpu->execute(cpu);
        }
    }
    else{
        while(frame == ppu->frame_count){

            if(breakpoint_manager->last_check_address != cpu->pc){

                breakpoint_manager->last_check_address = cpu->pc;

                if(breakpoint_manager->breakpoints && gb_breakpoint_manager_check(breakpoint_manager,cpu->pc)){
                    return;
                }
            }

            cpu->execute(cpu);
        }
    }
}

void gb_execute_step(gb_t* gb){
    if(!gb->cartridge_inserted || gb->multi_thread) return;

    gb_pause(gb,true);

    gb->cpu.execute(&gb->cpu);
}


#ifdef _WIN32
DWORD WINAPI gb_thread_function(void* data){
    gb_t* gb = (gb_t*)data;
    gb_cpu_t* cpu = &gb->cpu;
    
    while(gb_atomic_load_explicit(&gb->thread_running,gb_memory_order_relaxed)){
        cpu->execute(cpu);
    }

    return 0;
}
#else
void* gb_thread_function(void* data){
    gb_t* gb = (gb_t*)data;
    gb_cpu_t* cpu = &gb->cpu;
    
    while(gb_atomic_load_explicit(&gb->thread_running,gb_memory_order_relaxed)){
        cpu->execute(cpu);
    }

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

    gb_apu_update(&gb->apu);

    gb_timer_update(&gb->timer);
    gb_timer_set_div(&gb->timer,0);

    gb_cartridge_update_rtc_timer(&gb->cartridge);

    gb->speed_switch_needed = false;
    gb->double_speed = !gb->double_speed;

    gb_timer_schedule_next_event(&gb->timer);
}


void gb_write_key0_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_t* gb = (gb_t*)data;

    gb->cgb_mode = !(value & 0x0C);
}


void gb_write_key1_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_t* gb = (gb_t*)data;

    gb->speed_switch_needed = value & 0x01;
}

uint8_t gb_read_key1_register(void* data,uint16_t address){
    gb_unused(address);

    gb_t* gb = (gb_t*)data;

    return (gb->double_speed ? 0x80 : 0x00) | 0x7E | gb->speed_switch_needed;
}


void gb_write_opri_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_t* gb = (gb_t*)data;

    gb->obj_priority_mode = value & 0x01;
}

uint8_t gb_read_opri_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_t* gb = (gb_t*)data;

    return 0xFE | gb->obj_priority_mode;
}


void gb_write_undocumented_register(void* data,uint8_t value,uint16_t address){
    gb_t* gb = (gb_t*)data;

    switch(address){
        case 0xFF72: case 0xFF73: case 0xFF74:
            gb->undocumented_registers[address - 0xFF72] = value;
            break;
        case 0xFF75:
            gb->undocumented_registers[0x03] = value & 0x70;
            break;
    }
}

uint8_t gb_read_undocumented_register(void* data,uint16_t address){
    gb_t* gb = (gb_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF72: case 0xFF73: case 0xFF74:
            value = gb->undocumented_registers[address - 0xFF72];
            break;
        case 0xFF75:
            value = 0x8F | (gb->undocumented_registers[0x03] & 0x70);
            break;
    }

    return value;
}


void gb_map(gb_t* gb){
    gb_ppu_map(&gb->ppu);

    gb_apu_map_registers(&gb->apu);

    gb_joypad_map_registers(&gb->joypad);

    gb_interrupt_map_registers(&gb->interrupt);

    gb_timer_map_registers(&gb->timer);

    gb_dma_map(&gb->dma);

    gb_palette_map(&gb->palette);

    gb_serial_map_registers(&gb->serial);

    gb_memory_map_wram(&gb->memory);

    gb_memory_map_hram(&gb->memory);

    gb_cartridge_map(&gb->cartridge);
}

void gb_update_mapping(gb_t* gb){

    gb_memory_t* memory = &gb->memory;

    if(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped)){

        gb_memory_map(memory,&gb->key1_register_descriptor,0xFF4D);

        gb_memory_map(memory,&gb->ppu.vbk_register_descriptor,0xFF4F);

        gb_memory_map_in_range(memory,&gb->dma.vram_register_descriptor,0xFF51,0xFF55);

        gb_memory_map(memory,&gb->infrared.register_descriptor,0xFF56);

        gb_memory_map_in_range(memory,&gb->palette.cgb_register_descriptor,0xFF68,0xFF6B);

        gb_memory_map(memory,&gb->opri_register_descriptor,0xFF6C);

        gb_memory_map(memory,&memory->wbk_register_descriptor,0xFF70);

        gb_memory_map_in_range(memory,&gb->undocumented_register_descriptor,0xFF72,0xFF75);

        gb_memory_map(memory,&gb->apu.pcm12_register_descriptor,0xFF76);
        gb_memory_map(memory,&gb->apu.pcm34_register_descriptor,0xFF77);
    }
    else{
        gb_memory_unmap(memory,0xFF4D);

        gb_memory_unmap(memory,0xFF4F);

        gb_memory_unmap_in_range(memory,0xFF51,0xFF55);

        gb_memory_unmap(memory,0xFF56);

        gb_memory_unmap_in_range(memory,0xFF68,0xFF6B);

        gb_memory_unmap(memory,0xFF6C);

        gb_memory_unmap(memory,0xFF70);

        gb_memory_unmap_in_range(memory,0xFF72,0xFF75);

        gb_memory_unmap(memory,0xFF76);
        gb_memory_unmap(memory,0xFF77);
    }

    if(gb->is_cgb && gb->boot.mapped){
        gb_memory_map(memory,&gb->key0_register_descriptor,0xFF4C);
    }
    else{
        gb_memory_unmap(memory,0xFF4C);
    }

    if(gb->boot.mapped){
        gb_memory_map(memory,&gb->boot.bank_register_descriptor,0xFF50);
    }
    else{
        gb_memory_unmap(memory,0xFF50);
    }
    
    gb_event_manager_update_mapping(&gb->event_manager);
}


void gb_reset(gb_t* gb){

    gb->is_cgb = gb->is_cgb_pending;

    gb_boot_update_roms(&gb->boot);

    bool skip_boot = gb->boot.skip_enabled || (gb->is_cgb && !gb->boot.cgb_rom_inserted) || (!gb->is_cgb && !gb->boot.dmg_rom_inserted);

    if(gb->is_cgb && (!skip_boot || gb_cartridge_cgb_flag(&gb->cartridge))){
        gb->cgb_mode = true;
        gb->obj_priority_mode = false;
    }
    else{
        gb->cgb_mode = false;
        gb->obj_priority_mode = true;
    }

    gb->double_speed = false;
    gb->speed_switch_needed = false;

    memset(gb->undocumented_registers,0x00,sizeof(gb->undocumented_registers));

    gb->cycle = 0;

    gb_cpu_reset(&gb->cpu);
    gb_ppu_reset(&gb->ppu);
    gb_apu_reset(&gb->apu,true);
    gb_joypad_reset(&gb->joypad);
    gb_interrupt_reset(&gb->interrupt);
    gb_timer_reset(&gb->timer);
    gb_dma_reset(&gb->dma);
    gb_palette_reset(&gb->palette);
    gb_serial_reset(&gb->serial);
    gb_infrared_reset(&gb->infrared);
    gb_memory_reset(&gb->memory);
    gb_cartridge_reset(&gb->cartridge);
    gb_printer_reset(&gb->printer);
    
    gb_breakpoint_manager_reset(&gb->breakpoint_manager);
    gb_event_manager_reset(&gb->event_manager);

    if(skip_boot){
        gb_boot_unmap(&gb->boot);

        gb_cpu_skip_boot(&gb->cpu);
        gb_ppu_skip_boot(&gb->ppu);
        gb_apu_skip_boot(&gb->apu);
        gb_joypad_skip_boot(&gb->joypad);
        gb_interrupt_skip_boot(&gb->interrupt);
        gb_timer_skip_boot(&gb->timer);
        gb_dma_skip_boot(&gb->dma);
        gb_palette_skip_boot(&gb->palette);
        gb_memory_skip_boot(&gb->memory);
    }
    else{
        gb_boot_map(&gb->boot);
    }

    gb_update_mapping(gb);
}


void gb_delete(gb_t* gb){

    gb_thread_stop(gb);

    gb_remove_cartridge(gb);
    
    gb_apu_free(&gb->apu);

    gb_event_manager_free(&gb->event_manager);
    
    free(gb);
}