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
    if(dma->oam_state == gb_oam_dma_state_none || dma->gb->cpu.state == gb_cpu_halted_state) return;
    
    switch(dma->oam_state){
        case gb_oam_dma_state_delay:{
            dma->oam_state = gb_oam_dma_state_setup;
            break;
        }
        case gb_oam_dma_state_setup:{
            dma->oam_byte = gb_memory_oam_dma_read(&dma->gb->memory,dma->oam_hi_addr | dma->oam_counter);
            dma->oam_state = gb_oam_dma_state_transfer;
            break;
        }
        case gb_oam_dma_state_transfer:{
            dma->gb->ppu.oam[dma->oam_counter] = dma->oam_byte;

            if(++dma->oam_counter >= 0xA0){
                dma->oam_state = gb_oam_dma_state_none;
            }
            else{
                dma->oam_byte = gb_memory_oam_dma_read(&dma->gb->memory,dma->oam_hi_addr | dma->oam_counter);
            }
            break;
        }
    }
}


bool gb_oam_dma_bus_conflict(gb_dma_t* dma,uint16_t address){
    uint8_t src = dma->oam_src;

    if(dma->gb->is_cgb){
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
    gb_dma_t* dma = (gb_dma_t*)data;

    dma->oam_src = value;

    dma->oam_state = gb_oam_dma_state_delay;

    dma->oam_hi_addr = value << 0x08;
    if(dma->oam_hi_addr > 0xFD00){
        dma->oam_hi_addr &= ~0x2000;
    }
    dma->oam_counter = 0x00;
}

uint8_t gb_oam_dma_read_register(void* data,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;
    return dma->oam_src;
}


void gb_vram_hblank_dma(gb_dma_t* dma){
    
    if(!dma->vram_hblank_running || dma->gb->cpu.state == gb_cpu_halted_state) return;
    
    gb_t* gb = dma->gb;

    gb_memory_t* memory = &dma->gb->memory;

    for(uint8_t i = 0x00; i < 0x10; ++i){

        if(gb->double_speed){
            gb_machine_cycle(gb);
        }
        else{
            gb_half_machine_cycle(gb);
        }

        uint8_t byte = gb_memory_vram_dma_read(memory,dma->vram_src++);

        gb_memory_vram_dma_write(memory,byte,0x8000 | (dma->vram_dst & 0x1FFF));

        if(++dma->vram_dst == 0x00){
            break;
        }
    }

    dma->vram_length = (dma->vram_length - 0x01) & 0x7F;

    if((dma->vram_length == 0x7F) || !dma->vram_dst){
        dma->vram_hblank_running = false;
    }
}

void gb_vram_general_dma(gb_dma_t* dma){

    gb_t* gb = dma->gb;
    gb_memory_t* memory = &dma->gb->memory;

    gb_machine_cycle(gb);

    uint16_t len = ((dma->vram_length & 0x7F) + 0x01) << 0x04;
    
    while(len > 0){

        if(gb->double_speed){
            gb_machine_cycle(gb);
        }
        else{
            gb_half_machine_cycle(gb);
        }
        
        uint8_t byte = gb_memory_vram_dma_read(memory,dma->vram_src++);

        gb_memory_vram_dma_write(memory,byte,0x8000 | (dma->vram_dst & 0x1FFF));

        --len;

        if(++dma->vram_dst == 0x00){
            break;
        }
    }

    dma->vram_length = ((len >> 0x04) - 0x01) & 0x7F;
}


void gb_vram_dma_write_register(void* data,uint8_t value,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;
    gb_t* gb = dma->gb;

    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return;

    switch(address){
        //Src msb
        case 0xFF51:
            dma->vram_src = (value << 0x08) | (dma->vram_src & 0xF0);
            break;
        //Src lsb
        case 0xFF52:
            dma->vram_src = (dma->vram_src & 0xFF00) | (value & 0xF0);
            break;
        //Dst msb
        case 0xFF53:
            dma->vram_dst = (value << 0x08) | (dma->vram_dst & 0xF0);
            break;
        //Dst lsb
        case 0xFF54:
            dma->vram_dst = (dma->vram_dst & 0xFF00) | (value & 0xF0);
            break;
        //Control
        case 0xFF55:
            dma->vram_length = value & 0x7F;

            //Hblank
            if(value & 0x80){
                dma->vram_hblank_running = true;

                if(dma->gb->ppu.status.mode == gb_ppu_hblank_mode){
                    gb_vram_hblank_dma(dma);
                }
            }
            //General
            else{
                if(dma->vram_hblank_running){
                    dma->vram_hblank_running = false;
                    return;
                }

                gb_vram_general_dma(dma);
            }

            break;
    }
}

uint8_t gb_vram_dma_read_register(void* data,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;
    gb_t* gb = dma->gb;
    
    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return 0xFF;
    
    uint8_t value = 0xFF;

    if(address == 0xFF55){
        value = (dma->vram_hblank_running ? 0x00 : 0x80) | (dma->vram_length & 0x7F);
    }

    return value;
}


void gb_dma_map(gb_dma_t* dma){
    gb_memory_t* memory = &dma->gb->memory;
    
    gb_memory_map(memory,&dma->oam_register_descriptor,0xFF46);

    gb_memory_map_in_range(memory,&dma->vram_register_descriptor,0xFF51,0xFF55);
}


void gb_dma_reset(gb_dma_t* dma){
    dma->oam_state = gb_oam_dma_state_none;

    if(dma->gb->is_cgb){
        dma->oam_src = 0x00;
    }
    else{
        dma->oam_src = 0xFF;
    }
    
    dma->oam_hi_addr = 0x00;
    dma->oam_counter = 0x00;
    dma->oam_byte = 0x00;

    dma->vram_src = 0x00;
    dma->vram_dst = 0x00;
    dma->vram_length = 0x7F;
    dma->vram_hblank_running = false;
}

void gb_dma_skip_boot(gb_dma_t* dma){
    if(dma->gb->is_cgb){
        dma->vram_src = 0xD430;
        dma->vram_dst = 0x99D0;
    }
}


void gb_dma_save_state(gb_dma_t* dma,gb_state_t* state){
    gb_state_write(state,dma->oam_state);
    gb_state_write(state,dma->oam_src);
    gb_state_write(state,dma->oam_hi_addr);
    gb_state_write(state,dma->oam_counter);
    gb_state_write(state,dma->oam_byte);

    gb_state_write(state,dma->vram_src);
    gb_state_write(state,dma->vram_dst);
    gb_state_write(state,dma->vram_length);
    gb_state_write(state,dma->vram_hblank_running);
}

void gb_dma_load_state(gb_dma_t* dma,gb_state_t* state){
    gb_state_read(state,dma->oam_state);
    gb_state_read(state,dma->oam_src);
    gb_state_read(state,dma->oam_hi_addr);
    gb_state_read(state,dma->oam_counter);
    gb_state_read(state,dma->oam_byte);

    gb_state_read(state,dma->vram_src);
    gb_state_read(state,dma->vram_dst);
    gb_state_read(state,dma->vram_length);
    gb_state_read(state,dma->vram_hblank_running);
}