#include "dma.h"
#include "gb.h"

void gb_dma_init(gb_dma_t* dma,gb_t* gb){
    dma->gb = gb;

    dma->oam_register_descriptor = (gb_memory_descriptor_t){
        gb_oam_dma_write_register,
        gb_oam_dma_read_register,
        dma
    };
    
    dma->vram_register_descriptor = (gb_memory_descriptor_t){
        gb_vram_dma_write_register,
        gb_vram_dma_read_register,
        dma
    };
}


void gb_oam_dma_clock(gb_dma_t* dma){
    
    gb_dma_state_t* state = &dma->state;

    switch(state->oam_state){
        case gb_oam_dma_state_delay:{
            state->oam_state = gb_oam_dma_state_setup;
            break;
        }
        case gb_oam_dma_state_setup:{
            state->oam_byte = gb_memory_oam_dma_read(&dma->gb->memory,state->oam_hi_addr | state->oam_counter);
            state->oam_state = gb_oam_dma_state_transfer;
            break;
        }
        case gb_oam_dma_state_transfer:{

            dma->gb->ppu.state.oam[state->oam_counter] = state->oam_byte;

            if(dma->gb->event_manager.enabled){
                gb_event_manager_io(dma->gb,gb_event_write_flag,state->oam_byte,0xFE00 | state->oam_counter);
            }
            
            if(++state->oam_counter >= 0xA0){
                state->oam_state = gb_oam_dma_state_none;
            }
            else{
                state->oam_byte = gb_memory_oam_dma_read(&dma->gb->memory,state->oam_hi_addr | state->oam_counter);
            }
            break;
        }
    }
}


bool gb_oam_dma_bus_conflict(gb_dma_t* dma,uint16_t address){
    uint8_t src = dma->state.oam_src;

    if(dma->gb->state.is_cgb){
        return (
            //ROM and RAM
            ((src <= 0x7F || (src >= 0xA0 && src <= 0xBF)) && (address <= 0x7FFF || (address >= 0xA000 && address <= 0xBFFF))) ||
            //VRAM
            (src >= 0x80 && src <= 0x9F && address >= 0x8000 && address <= 0x9FFF) ||
            //WRAM
            (src >= 0xC0 && src <= 0xFD && address >= 0xC000 && address <= 0xFDFF) 
        );
    }
    else{
        return (
            //ROM,RAM and WRAM
            ((src <= 0x7F || (src >= 0xA0 && src <= 0xFD)) && (address <= 0x7FFF || (address >= 0xA000 && address <= 0xFDFF))) ||
            //VRAM
            (src >= 0x80 && src <= 0x9F && address >= 0x8000 && address <= 0x9FFF)
        );
    }
}


void gb_oam_dma_write_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);
    
    gb_dma_t* dma = (gb_dma_t*)data;

    dma->state.oam_src = value;

    dma->state.oam_state = gb_oam_dma_state_delay;

    dma->state.oam_hi_addr = value << 0x08;
    if(dma->state.oam_hi_addr > 0xFD00){
        dma->state.oam_hi_addr &= ~0x2000;
    }
    dma->state.oam_counter = 0x00;
}

uint8_t gb_oam_dma_read_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_dma_t* dma = (gb_dma_t*)data;

    return dma->state.oam_src;
}


void gb_vram_hblank_dma(gb_dma_t* dma){
    
    gb_dma_state_t* state = &dma->state;

    state->vram_hblank_pending = false;
    
    gb_t* gb = dma->gb;

    gb_memory_t* memory = &dma->gb->memory;

    for(uint8_t i = 0x00; i < 0x10; ++i){

        if(gb->state.double_speed){
            gb_machine_cycle(gb);
        }
        else{
            gb_half_machine_cycle(gb);
        }

        uint8_t byte = gb_memory_vram_dma_read(memory,state->vram_src++);

        gb_memory_vram_dma_write(memory,byte,0x8000 | (state->vram_dst & 0x1FFF));

        if(++state->vram_dst == 0x00){
            break;
        }
    }

    state->vram_length = (state->vram_length - 0x01) & 0x7F;

    if((state->vram_length == 0x7F) || !state->vram_dst){
        state->vram_hblank_running = false;
    }
}

void gb_vram_general_dma(gb_dma_t* dma){

    gb_dma_state_t* state = &dma->state;
    gb_t* gb = dma->gb;
    gb_memory_t* memory = &dma->gb->memory;

    gb_machine_cycle(gb);

    uint16_t len = ((state->vram_length & 0x7F) + 0x01) << 0x04;
    
    while(len > 0){

        if(gb->state.double_speed){
            gb_machine_cycle(gb);
        }
        else{
            gb_half_machine_cycle(gb);
        }
        
        uint8_t byte = gb_memory_vram_dma_read(memory,state->vram_src++);

        gb_memory_vram_dma_write(memory,byte,0x8000 | (state->vram_dst & 0x1FFF));

        --len;

        if(++state->vram_dst == 0x00){
            break;
        }
    }

    state->vram_length = ((len >> 0x04) - 0x01) & 0x7F;
}


void gb_vram_dma_write_register(void* data,uint8_t value,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;
    gb_dma_state_t* state = &dma->state;

    switch(address){
        //Src msb
        case 0xFF51:
            state->vram_src = (value << 0x08) | (state->vram_src & 0xF0);
            break;
        //Src lsb
        case 0xFF52:
            state->vram_src = (state->vram_src & 0xFF00) | (value & 0xF0);
            break;
        //Dst msb
        case 0xFF53:
            state->vram_dst = (value << 0x08) | (state->vram_dst & 0xF0);
            break;
        //Dst lsb
        case 0xFF54:
            state->vram_dst = (state->vram_dst & 0xFF00) | (value & 0xF0);
            break;
        //Control
        case 0xFF55:
            state->vram_length = value & 0x7F;

            //Hblank
            if(value & 0x80){
                state->vram_hblank_running = true;

                if(dma->gb->ppu.state.status.mode == gb_ppu_hblank_mode){
                    gb_vram_hblank_dma(dma);
                }
            }
            //General
            else{
                if(state->vram_hblank_running){
                    state->vram_hblank_running = false;
                    return;
                }

                gb_vram_general_dma(dma);
            }

            break;
    }
}

uint8_t gb_vram_dma_read_register(void* data,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;
    gb_dma_state_t* state = &dma->state;

    uint8_t value = 0xFF;

    if(address == 0xFF55){
        value = (state->vram_hblank_running ? 0x00 : 0x80) | (state->vram_length & 0x7F);
    }

    return value;
}


void gb_dma_map(gb_dma_t* dma){
    gb_memory_t* memory = &dma->gb->memory;
    gb_memory_map(memory,&dma->oam_register_descriptor,0xFF46);
}


void gb_dma_reset(gb_dma_t* dma){
    gb_dma_state_t* state = &dma->state;

    state->oam_state = gb_oam_dma_state_none;

    if(dma->gb->state.is_cgb){
        state->oam_src = 0x00;
    }
    else{
        state->oam_src = 0xFF;
    }
    
    state->oam_hi_addr = 0x00;
    state->oam_counter = 0x00;
    state->oam_byte = 0x00;

    state->vram_src = 0x00;
    state->vram_dst = 0x00;
    state->vram_length = 0x7F;
    state->vram_hblank_running = false;
    state->vram_hblank_pending = false;
}

void gb_dma_skip_boot(gb_dma_t* dma){
    if(dma->gb->state.is_cgb){
        dma->state.vram_src = 0xD430;
        dma->state.vram_dst = 0x99D0;
    }
}


void gb_dma_save_state(gb_dma_t* dma,gb_snapshot_t* snapshot){
    snapshot->dma = dma->state;
}

void gb_dma_load_state(gb_dma_t* dma,gb_snapshot_t* snapshot){
    dma->state = snapshot->dma;
}