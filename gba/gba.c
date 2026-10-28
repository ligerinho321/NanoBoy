#include "gba.h"

gba_t* gba_new(){
    gba_t* gba = malloc(sizeof(gba_t));

    if(!gba){
        gba_printf_errno(malloc);
        return NULL;
    }

    memset(gba,0x00,sizeof(gba_t));

    gba_cpu_init(&gba->cpu,gba);
    gba_ppu_init(&gba->ppu,gba);
    gba_apu_init(&gba->apu,gba);

    gba_interrupt_init(&gba->interrupt,gba);
    gba_timer_init(&gba->timer,gba);
    gba_memory_init(&gba->memory,gba);
    gba_dma_init(&gba->dma,gba);
    gba_keypad_init(&gba->keypad,gba);
    gba_serial_init(&gba->serial,gba);
    gba_cartridge_init(&gba->cartridge,gba);
    gba_boot_init(&gba->boot,gba);
    
    return gba;
}


bool gba_insert_cartridge(gba_t* gba,const char* path){
    
    uint8_t* data = NULL;
    size_t length = 0;

    if(!gba_load_file(path,(void**)&data,&length)){
        gba_remove_cartridge(gba);
        return false;
    }

    return gba_insert_cartridge_from_memory(gba,data,length);
}

bool gba_insert_cartridge_from_memory(gba_t* gba,uint8_t* rom,size_t length){
    gba_remove_cartridge(gba);

    if(!gba_cartridge_load(&gba->cartridge,rom,length)){
        return false;
    }

    gba->cartridge_inserted = true;

    gba_reset(gba);

    return true;
}

void gba_remove_cartridge(gba_t* gba){
    gba_cartridge_unload(&gba->cartridge);
    gba->cartridge_inserted = false;
}


void gba_pause(gba_t* gba,bool paused){
    if(gba->paused == paused) return;

    gba->paused = paused;

    if(gba->paused){
        gba_timer_update(&gba->timer);
        gba_timer_schedule_next_event(&gba->timer);

        gba_ppu_update(&gba->ppu);
        gba_ppu_schedule_next_event(&gba->ppu);
    }
    else{

    }
}


void gba_clock(gba_t* gba,int clocks){
    uint64_t target_cycle = gba->cycles + clocks * 4;

    while(gba->cycles < target_cycle){

        uint64_t next_cycle = target_cycle;

        if(gba->timer.next_schedule_event < next_cycle){
            next_cycle = gba->timer.next_schedule_event;
        }
        if(gba->ppu.next_schedule_event < next_cycle){
            next_cycle = gba->ppu.next_schedule_event;
        }

        assert(next_cycle >= gba->cycles);
        
        gba->cycles = next_cycle;

        if(gba->cycles >= gba->timer.next_schedule_event){
            gba_timer_update(&gba->timer);
            gba_timer_schedule_next_event(&gba->timer);
        }
        if(gba->cycles >= gba->ppu.next_schedule_event){
            gba_ppu_update(&gba->ppu);
            gba_ppu_schedule_next_event(&gba->ppu);
        }
    }
}


void gba_execute_frame(gba_t* gba){
    if(!gba->cartridge_inserted || gba->paused) return;

    uint64_t frame = gba->ppu.frame_count;
    gba_cpu_t* cpu = &gba->cpu;

    while(frame == gba->ppu.frame_count){
        gba_cpu_execute(cpu);
    }
}


void gba_reset(gba_t* gba){

    gba_boot_update(&gba->boot);

    gba->cycles = 0;

    gba_cpu_reset(&gba->cpu);
    gba_ppu_reset(&gba->ppu);
    gba_apu_reset(&gba->apu);

    gba_interrupt_reset(&gba->interrupt);
    gba_timer_reset(&gba->timer);
    gba_memory_reset(&gba->memory);
    gba_dma_reset(&gba->dma);
    gba_keypad_reset(&gba->keypad);
    gba_serial_reset(&gba->serial);
    gba_cartridge_reset(&gba->cartridge);
    gba_boot_reset(&gba->boot);

    gba_cpu_reload_pipeline(&gba->cpu);
}


void gba_delete(gba_t* gba){

    gba_remove_cartridge(gba);

    free(gba);
}