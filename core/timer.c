#include "timer.h"
#include "gb.h"

static const uint16_t div_bit[4] = {0x200,0x08,0x20,0x80};

void gb_timer_init(gb_timer_t* timer,gb_t* gb){
    timer->gb = gb;
    
    timer->register_descriptor = (gb_memory_descriptor_t){
        gb_timer_write_register,
        gb_timer_read_register,
        timer
    };
}


void gb_timer_schedule_next_event(gb_timer_t* timer){

    gb_timer_state_t *state = &timer->state;

    state->last_schedule_event = timer->gb->state.cycle;

    state->next_schedule_event = timer->gb->state.cycle;

    if(state->tima_reload_request){
        state->next_schedule_event += 4;    
    }
    else{
        uint16_t tima_cycles = (uint16_t)-1;
        uint16_t frame_sequencer_cycles = (uint16_t)-1;

        if(state->enabled){

            uint16_t tima_half_period = div_bit[state->clock_select];
            uint16_t tima_period = tima_half_period << 0x01;
            uint16_t tima_mod = state->div % tima_period;
            
            if(tima_mod < tima_half_period){
                tima_cycles = tima_half_period - tima_mod;
            }
            else{
                tima_cycles = tima_period - tima_mod;
            }
        }
        
        uint16_t frame_sequencer_half_period = timer->gb->state.double_speed ? 0x2000 : 0x1000;
        uint16_t frame_sequencer_period = timer->gb->state.double_speed ? 0x4000 : 0x2000;
        uint16_t frame_sequencer_mod = state->div % frame_sequencer_period;

        if(frame_sequencer_mod < frame_sequencer_half_period){
            frame_sequencer_cycles = frame_sequencer_half_period - frame_sequencer_mod;
        }
        else{
            frame_sequencer_cycles = frame_sequencer_period - frame_sequencer_mod;
        }

        state->next_schedule_event += gb_min(tima_cycles,frame_sequencer_cycles);
    }
}


void gb_timer_set_div(gb_timer_t* timer,uint16_t new_div){

    gb_timer_state_t* state = &timer->state;

    uint16_t timer_div_bit = div_bit[state->clock_select];

    if(state->enabled && (state->div & timer_div_bit) && !(new_div & timer_div_bit)){
        if(++state->tima == 0x00){
            state->tima_reload_request = true;
        }
    }

    uint16_t apu_div_bit = timer->gb->state.double_speed ? 0x2000 : 0x1000;

    if((state->div & apu_div_bit) && !(new_div & apu_div_bit)){
        gb_apu_frame_sequencer_clock(&timer->gb->apu);
    }

    state->div = new_div;
}


static inline void gb_timer_tima_reload(gb_timer_t* timer){
    gb_timer_state_t* state = &timer->state;

    state->tima_reload_request = false;
    state->tima_reloaded = true;
    state->tima = state->tma;

    timer->gb->interrupt.state.flag |= gb_interrupt_timer_flag;
}


void gb_timer_update(gb_timer_t* timer){

    gb_timer_state_t* state = &timer->state;

    uint64_t cycles = timer->gb->state.cycle - state->last_schedule_event;

    if(!cycles) return;

    state->tima_reloaded = false;

    if(state->tima_reload_request){
        gb_timer_tima_reload(timer);
    }

    gb_timer_set_div(timer,state->div + cycles);
}


void gb_timer_write_register(void* data,uint8_t value,uint16_t address){
    gb_timer_t* timer = (gb_timer_t*)data;
    gb_timer_state_t* state = &timer->state;

    gb_timer_update(timer);

    switch(address){
        //DIV
        case 0xFF04:{
            gb_timer_set_div(timer,0x00);
            break;
        }
        //TIMA
        case 0xFF05:{
            state->tima_reload_request = false;
            if(!state->tima_reloaded){
                state->tima = value;
            }
            break;
        }
        //TMA
        case 0xFF06:{
            state->tma = value;
            if(state->tima_reloaded){
                state->tima = value;
            }
            break;
        }
        //TAC
        case 0xFF07:{
            uint16_t old_div_bit = div_bit[state->clock_select];
            bool old_enabled = state->enabled;

            state->clock_select = value & 0x03;
            state->enabled = value & 0x04;

            uint16_t new_div_bit = div_bit[state->clock_select];
            bool new_enabled = state->enabled;

            if((old_enabled && (state->div & old_div_bit)) && !(new_enabled && (state->div & new_div_bit))){
                if(++state->tima == 0x00){
                    gb_timer_tima_reload(timer);
                }
            }

            break;
        }
    }

    gb_timer_schedule_next_event(timer);
}

uint8_t gb_timer_read_register(void* data,uint16_t address){
    gb_timer_t* timer = (gb_timer_t*)data;
    gb_timer_state_t* state = &timer->state;

    gb_timer_update(timer);

    uint8_t value = 0xFF;

    switch(address){
        //DIV
        case 0xFF04: value = state->div >> 0x08; break;
        //TIMA
        case 0xFF05: value = state->tima; break;
        //TMA
        case 0xFF06: value = state->tma; break;
        //TAC
        case 0xFF07: value = 0xF8 | (state->enabled ? 0x04 : 0x00) | state->clock_select; break;
    }

    gb_timer_schedule_next_event(timer);

    return value;
}


void gb_timer_map_registers(gb_timer_t* timer){
    gb_memory_map_in_range(&timer->gb->memory,&timer->register_descriptor,0xFF04,0xFF07);
}


void gb_timer_reset(gb_timer_t* timer){
    memset(&timer->state,0x00,sizeof(timer->state));
    
    gb_timer_schedule_next_event(timer);
}

void gb_timer_skip_boot(gb_timer_t* timer){
    if(timer->gb->state.is_cgb){
        
        if(timer->gb->state.cgb_mode){
            //Value based on the Shantae ROM
            timer->state.div = 0x1CA0;
        }
        else{
            //Value based on the Pokemon Red ROM
            timer->state.div = 0x2C00;
        }
    }
    else{
        timer->state.div = 0xABC4;
    }

    gb_timer_schedule_next_event(timer);
}

void gb_timer_save_state(gb_timer_t* timer,gb_snapshot_t* snapshot){
    snapshot->timer = timer->state;
}

void gb_timer_load_state(gb_timer_t* timer,gb_snapshot_t* snapshot){
    timer->state = snapshot->timer;
}