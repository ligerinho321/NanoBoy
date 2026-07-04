#include "timer.h"
#include "gb.h"

void gb_timer_init(gb_timer_t* timer,gb_t* gb){
    timer->gb = gb;
    
    gb_timer_map_registers(timer);
}


static inline void gb_timer_tima_reload(gb_timer_t* timer){
    timer->tima_reload_request = false;
    timer->tima_reloaded = true;
    timer->tima = timer->tma;
    timer->gb->interrupt.flag |= gb_interrupt_timer_flag;
}


void gb_timer_set_div(gb_timer_t* timer,uint16_t new_div){

    if(timer->enabled && (timer->div & timer->div_bit) && !(new_div & timer->div_bit)){
        if(++timer->tima == 0x00){
            timer->tima_reload_request = true;
        }
    }

    uint16_t bit = timer->gb->double_speed ? 0x2000 : 0x1000;

    if((timer->div & bit) && !(new_div & bit)){
        gb_apu_frame_sequencer_clock(&timer->gb->apu);
    }

    timer->div = new_div;
}

void gb_timer_clock(gb_timer_t* timer){

    timer->tima_reloaded = false;

    if(timer->tima_reload_request){
        gb_timer_tima_reload(timer);
    }

    gb_timer_set_div(timer,timer->div + 4);
}


void gb_timer_write_register(void* data,uint8_t value,uint16_t address){
    gb_timer_t* timer = (gb_timer_t*)data;

    switch(address){
        //DIV
        case 0xFF04:{
            gb_timer_set_div(timer,0x00);
            break;
        }
        //TIMA
        case 0xFF05:{
            timer->tima_reload_request = false;
            if(!timer->tima_reloaded){
                timer->tima = value;
            }
            break;
        }
        //TMA
        case 0xFF06:{
            timer->tma = value;
            if(timer->tima_reloaded){
                timer->tima = value;
            }
            break;
        }
        //TAC
        case 0xFF07:{
            timer->clock_select = value & 0x03;
            
            bool new_enabled = value & 0x04;
            uint16_t new_div_bit = 0x00;

            switch(timer->clock_select){
                case 0x00: new_div_bit = 0x200; break;
                case 0x01: new_div_bit = 0x08; break;
                case 0x02: new_div_bit = 0x20; break;
                case 0x03: new_div_bit = 0x80; break;
            }

            if((timer->enabled && (timer->div & timer->div_bit)) && !(new_enabled && (timer->div & new_div_bit))){
                if(++timer->tima == 0x00){
                    gb_timer_tima_reload(timer);
                }
            }

            timer->enabled = new_enabled;
            timer->div_bit = new_div_bit;

            break;
        }
    }
}

uint8_t gb_timer_read_register(void* data,uint16_t address){
    gb_timer_t* timer = (gb_timer_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        //DIV
        case 0xFF04: value = timer->div >> 0x08; break;
        //TIMA
        case 0xFF05: value = timer->tima; break;
        //TMA
        case 0xFF06: value = timer->tma; break;
        //TAC
        case 0xFF07: value = 0xF8 | (timer->enabled ? 0x04 : 0x00) | (timer->clock_select & 0x03); break;
    }

    return value;
}


void gb_timer_map_registers(gb_timer_t* timer){
    
    timer->register_handler = (gb_memory_handler_t){
        gb_timer_write_register,
        gb_timer_read_register,
        timer
    };

    gb_memory_handler_t** bus = timer->gb->memory.bus;

    bus[0xFF04] = &timer->register_handler;
    bus[0xFF05] = &timer->register_handler;
    bus[0xFF06] = &timer->register_handler;
    bus[0xFF07] = &timer->register_handler;
}


void gb_timer_reset(gb_timer_t* timer){
    timer->div = 0x00;
    timer->tima = 0x00;
    timer->tma = 0x00;
    timer->enabled = false;
    timer->clock_select = 0x00;
    timer->div_bit = 0x200;
    timer->tima_reload_request = false;
    timer->tima_reloaded = false;
}