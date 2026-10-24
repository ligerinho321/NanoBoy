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
    gba_bios_init(&gba->bios,gba);
    
    return gba;
}

bool gba_insert_cartridge(gba_t* gba,const char* path){
    
    gba_remove_cartridge(gba);

    if(!gba_cartridge_load(&gba->cartridge,path)){
        return false;
    }

    gba->cartridge_inserted = true;
}

void gba_remove_cartridge(gba_t* gba){
    gba_cartridge_unload(&gba->cartridge);
    gba->cartridge_inserted = false;
}

void gba_clock(gba_t* gba,int clocks){
    while(clocks-- > 0){
        gba->cycles += 4;

        if(gba->cycles >= gba->timer.next_schedule_event){
            gba_timer_update(&gba->timer);
            gba_timer_schedule_next_event(&gba->timer);
        }
    }
}

void gba_reset(gba_t* gba){

    gba_bios_update(&gba->bios);

    gba->cycles = 0;

    gba_ppu_reset(&gba->ppu);
    gba_apu_reset(&gba->apu);
    gba_interrupt_reset(&gba->interrupt);
    gba_timer_reset(&gba->timer);
    gba_memory_reset(&gba->memory);
    gba_dma_reset(&gba->dma);
    gba_keypad_reset(&gba->keypad);
    gba_serial_reset(&gba->serial);
    gba_cartridge_reset(&gba->cartridge);
}

void gba_delete(gba_t* gba){

    gba_remove_cartridge(gba);

    free(gba);
}