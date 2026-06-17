#include "dma.h"
#include "gb.h"

void gb_dma_init(gb_dma_t* dma,gb_t* gb){
    dma->gb = gb;

    gb_oam_dma_map_registers(dma);
    
    dma->vram_register_handler = (gb_memory_handler_t){
        gb_vram_dma_write_register,
        gb_vram_dma_read_register,
        dma
    };
}


void gb_oam_dma_clock(gb_dma_t* dma){
    if(dma->oam_state == gb_oam_dma_state_none || dma->gb->cpu.halted) return;
    
    switch(dma->oam_state){
        case gb_oam_dma_state_delay:{
            dma->oam_state = gb_oam_dma_state_setup;
            break;
        }
        case gb_oam_dma_state_setup:{
            dma->oam_byte = gb_memory_oam_dma_read(&dma->gb->memory,dma->oam_hi_addr | dma->oam_counter);
            dma->oam_state = gb_oam_dma_state_transfer;
            dma->oam_running = true;
            break;
        }
        case gb_oam_dma_state_transfer:{
            dma->gb->ppu.oam[dma->oam_counter] = dma->oam_byte;

            if(++dma->oam_counter >= 0xA0){
                dma->oam_state = gb_oam_dma_state_none;
                dma->oam_running = false;
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

    if(dma->gb->type == gb_cgb){
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
    
    if(!dma->vram_hblank_running || dma->gb->cpu.halted) return;
    
    gb_t* gb = dma->gb;

    if(dma->oam_running){
        printf("vram hbalnk dma running during oam dma\n");
    }

    gb_memory_t* memory = &dma->gb->memory;

    uint16_t len = ((dma->vram_length & 0x7F) + 0x01) << 0x04;

    printf("vram hblank dma src: %04x dst: %04x len: %d\n",dma->vram_src,dma->vram_dst,len);

    for(uint8_t i = 0x00; i < 0x10; ++i){

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

    printf("vram hblank dma remaining bytes %d\n",len);

    dma->vram_length = (dma->vram_length - 0x01) & 0x7F;

    if((dma->vram_length == 0x7F) || !dma->vram_dst){
        dma->vram_hblank_running = false;
    }
}


void gb_vram_dma_write_register(void* data,uint8_t value,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;

    switch(address){
        //Src msb
        case 0xFF51:
            dma->vram_src = (value << 0x08) | (dma->vram_src & 0xF0);
            //printf("write hdma1 %02x\n",value);
            break;
        //Src lsb
        case 0xFF52:
            dma->vram_src = (dma->vram_src & 0xFF00) | (value & 0xF0);
            //printf("write hdma2 %02x\n",value);
            break;
        //Dst msb
        case 0xFF53:
            dma->vram_dst = (value << 0x08) | (dma->vram_dst & 0xF0);
            //printf("write hdma3 %02x\n",value);
            break;
        //Dst lsb
        case 0xFF54:
            dma->vram_dst = (dma->vram_dst & 0xFF00) | (value & 0xF0);
            //printf("write hdma4 %02x\n",value);
            break;
        //Control
        case 0xFF55:
            //printf("write hdma5 %02x\n",value);

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

                if(dma->oam_running){
                    printf("vram hbalnk dma running during oam dma\n");
                }

                //printf("vram general dma src: %04x dst: %04x len: %d\n",dma->vram_src,dma->vram_dst,len);

                gb_t* gb = dma->gb;
                gb_memory_t* memory = &dma->gb->memory;

                gb_machine_cycle(gb);

                uint16_t len = ((value & 0x7F) + 0x01) << 0x04;
                
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

                //printf("vram general dma remaining bytes %d\n",len);

                dma->vram_length = ((len >> 0x04) - 0x01) & 0x7F;
            }

            break;
    }
}

uint8_t gb_vram_dma_read_register(void* data,uint16_t address){
    gb_dma_t* dma = (gb_dma_t*)data;

    uint8_t value = 0xFF;

    if(address == 0xFF55){
        value = (dma->vram_hblank_running ? 0x00 : 0x80) | (dma->vram_length & 0x7F);
        printf("read hdma5 %02x\n",value);
    }

    return value;
}


void gb_oam_dma_map_registers(gb_dma_t* dma){

    dma->oam_register_handler = (gb_memory_handler_t){
        gb_oam_dma_write_register,
        gb_oam_dma_read_register,
        dma
    };

    gb_memory_handler_t** bus = dma->gb->memory.bus;

    bus[0xFF46] = &dma->oam_register_handler;
}


void gb_vram_dma_map_registers(gb_dma_t* dma){
    gb_memory_handler_t** bus = dma->gb->memory.bus;
    bus[0xFF51] = &dma->vram_register_handler;
    bus[0xFF52] = &dma->vram_register_handler;
    bus[0xFF53] = &dma->vram_register_handler;
    bus[0xFF54] = &dma->vram_register_handler;
    bus[0xFF55] = &dma->vram_register_handler;
}

void gb_vram_dma_unmap_registers(gb_dma_t* dma){
    gb_memory_handler_t** bus = dma->gb->memory.bus;
    bus[0xFF51] = NULL;
    bus[0xFF52] = NULL;
    bus[0xFF53] = NULL;
    bus[0xFF54] = NULL;
    bus[0xFF55] = NULL;
}


void gb_dma_reset(gb_dma_t* dma){
    dma->oam_state = gb_oam_dma_state_none;
    dma->oam_running = false;

    dma->oam_src = 0x00;
    dma->oam_hi_addr = 0x00;
    dma->oam_counter = 0x00;
    dma->oam_byte = 0x00;

    dma->vram_src = 0x00;
    dma->vram_dst = 0x00;
    dma->vram_length = 0xFF;
    dma->vram_hblank_running = 0x00;
}