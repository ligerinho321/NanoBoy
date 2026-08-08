#include "timer.h"
#include "gb.h"

static const uint16_t div_bit[4] = {0x200,0x08,0x20,0x80};

void gb_timer_init(gb_timer_t* timer,gb_t* gb){
    timer->gb = gb;
    
    timer->register_handler = (gb_memory_handler_t){
        gb_timer_write_register,
        gb_timer_read_register,
        timer
    };
}


static inline void gb_timer_tima_reload(gb_timer_t* timer){
    timer->tima_reload_request = false;
    timer->tima_reloaded = true;
    timer->tima = timer->tma;
    timer->gb->interrupt.flag |= gb_interrupt_timer_flag;
}


void gb_timer_set_div(gb_timer_t* timer,uint16_t new_div){

    uint16_t timer_div_bit = div_bit[timer->clock_select];

    if(timer->enabled && (timer->div & timer_div_bit) && !(new_div & timer_div_bit)){
        if(++timer->tima == 0x00){
            timer->tima_reload_request = true;
        }
    }

    uint16_t apu_div_bit = timer->gb->double_speed ? 0x2000 : 0x1000;

    if((timer->div & apu_div_bit) && !(new_div & apu_div_bit)){
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
            uint16_t old_div_bit = div_bit[timer->clock_select];
            bool old_enabled = timer->enabled;

            timer->clock_select = value & 0x03;
            timer->enabled = value & 0x04;

            uint16_t new_div_bit = div_bit[timer->clock_select];
            bool new_enabled = timer->enabled;

            if((old_enabled && (timer->div & old_div_bit)) && !(new_enabled && (timer->div & new_div_bit))){
                if(++timer->tima == 0x00){
                    gb_timer_tima_reload(timer);
                }
            }

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
    gb_memory_map_in_range(&timer->gb->memory,&timer->register_handler,0xFF04,0xFF07);
}


void gb_timer_reset(gb_timer_t* timer){
    timer->div = 0x00;
    timer->tima = 0x00;
    timer->tma = 0x00;
    timer->clock_select = 0x00;
    timer->enabled = false;
    timer->tima_reload_request = false;
    timer->tima_reloaded = false;
}

void gb_timer_skip_boot(gb_timer_t* timer){
    if(timer->gb->is_cgb){
        
        if(timer->gb->cgb_mode){
            //Value based on the Shantae ROM
            timer->div = 0x1CA0;
        }
        else{
            //Value based on the Pokemon Red ROM
            timer->div = 0x2C00;
        }
    }
    else{
        timer->div = 0xABC4;
    }
}


void gb_timer_save_state(gb_timer_t* timer,gb_state_t* state){
    gb_state_write(state,timer->div);
    gb_state_write(state,timer->tima);
    gb_state_write(state,timer->tma);
    gb_state_write(state,timer->clock_select);
    gb_state_write(state,timer->enabled);
    gb_state_write(state,timer->tima_reload_request);
    gb_state_write(state,timer->tima_reload_request);
}

void gb_timer_load_state(gb_timer_t* timer,gb_state_t* state){
    gb_state_read(state,timer->div);
    gb_state_read(state,timer->tima);
    gb_state_read(state,timer->tma);
    gb_state_read(state,timer->clock_select);
    gb_state_read(state,timer->enabled);
    gb_state_read(state,timer->tima_reload_request);
    gb_state_read(state,timer->tima_reload_request);
}