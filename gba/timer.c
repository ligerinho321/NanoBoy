#include "timer.h"
#include "gba.h"

uint8_t gba_timer_divisor[4] = {
    24,
    18,
    16,
    14
};

uint32_t gba_timer_mask[4] = {
    0x00FFFFF,
    0x003FFFF,
    0x000FFFF,
    0x0003FFF
};


void gba_timer_init(gba_timer_t* timer,gba_t* gba){
    timer->gba = gba;
}


void gba_timer_schedule_next_event(gba_timer_t* timer){

    timer->last_schedule_event = timer->gba->cycles;

    uint64_t cycles = (uint64_t)-1;

    for(int i = 0; i < 4; ++i){

        gba_timer_counter_t* counter = timer->counter + i;

        if(!counter->control.enabled) continue;

        if(i == 0 || !counter->control.countup){
            
            uint64_t overflow_cycles = 0x10000 - counter->value;
            overflow_cycles <<= gba_timer_divisor[counter->control.countup];
            overflow_cycles -= counter->accumulate;
            
            if(cycles < overflow_cycles){
                cycles = overflow_cycles;
            }
        }
    }

    if(cycles < (uint64_t)-1){
        timer->next_schedule_event = timer->gba->cycles + cycles;
    }
    else{
        timer->next_schedule_event = (uint64_t)-1;
    }
}

void gba_timer_update(gba_timer_t* timer){
    uint64_t elapsed_cycles = timer->gba->cycles - timer->last_schedule_event;

    if(!elapsed_cycles) return;

    bool previous_overflow = false;

    for(int i = 0; i < 4; ++i){

        gba_timer_counter_t* counter = timer->counter + i;

        if(!counter->control.enabled){
            previous_overflow = false;
            continue;
        }

        if(i > 0 && counter->control.countup){
            if(previous_overflow && ++counter->value == 0x0000){
                
                counter->value = counter->reload_value.v;

                timer->gba->interrupt.flag.v |= gba_interrupt_timer0_flag << i;
            }
            else{
                previous_overflow = false;
            }
        }
        else{
            uint64_t cycles = elapsed_cycles + counter->accumulate;

            uint64_t steps = cycles >> gba_timer_divisor[counter->control.prescaler];

            if(steps > 0xFFFF || counter->value > (0xFFFF - steps)){
                previous_overflow = true;

                counter->value = counter->reload_value.v;

                timer->gba->interrupt.flag.v |= gba_interrupt_timer0_flag << i;
            }
            else{
                previous_overflow = false;

                counter->value += steps;
            }
            
            counter->accumulate = cycles & gba_timer_mask[counter->control.prescaler];
        }
    }
}


static inline void gba_timer_write_counter_lsb(gba_timer_counter_t* counter,uint8_t value){
    gba_timer_control_t* control = &counter->control;

    control->prescaler = value & 0x03;
    control->countup = value & 0x04;
    control->irq_enabled = value & 0x40;

    bool new_enabled = value & 0x80;
    
    if(!control->enabled && new_enabled){
        counter->accumulate = 0;
        counter->value = counter->reload_value.v;
    }

    control->enabled = new_enabled;
}

static inline uint8_t gba_timer_read_counter_lsb(gba_timer_counter_t* counter){
    gba_timer_control_t* control = &counter->control;
    return (
        (control->enabled << 0x07) |
        (control->irq_enabled << 0x06) |
        (control->countup << 0x02) |
        (control->prescaler & 0x03)
    );
}


void gba_timer_write_register_raw(gba_timer_t* timer,uint8_t value,uint32_t address){

    gba_timer_update(timer);

    switch(address){
        case 0x04000100: timer->counter[0].reload_value.b[0] = value; break;
        case 0x04000101: timer->counter[0].reload_value.b[1] = value; break;

        case 0x04000102: gba_timer_write_counter_lsb(timer->counter + 0,value); break;
        case 0x04000103: break;

        case 0x04000104: timer->counter[1].reload_value.b[0] = value; break;
        case 0x04000105: timer->counter[1].reload_value.b[1] = value; break;

        case 0x04000106: gba_timer_write_counter_lsb(timer->counter + 1,value); break;
        case 0x04000107: break;

        case 0x04000108: timer->counter[2].reload_value.b[0] = value; break;
        case 0x04000109: timer->counter[2].reload_value.b[1] = value; break;

        case 0x0400010A: gba_timer_write_counter_lsb(timer->counter + 2,value); break;
        case 0x0400010B: break;

        case 0x0400010C: timer->counter[3].reload_value.b[0] = value; break;
        case 0x0400010D: timer->counter[3].reload_value.b[1] = value; break;
        
        case 0x4000010E: gba_timer_write_counter_lsb(timer->counter + 3,value); break;
        case 0x0400010F: break;
    }

    gba_timer_schedule_next_event(timer);
}

uint8_t gba_timer_read_register_raw(gba_timer_t* timer,uint32_t address){

    gba_timer_update(timer);

    uint8_t value = timer->gba->memory.bus.b[address & 0x03];

    switch(address){
        case 0x04000100: value = timer->counter[0].reload_value.b[0]; break;
        case 0x04000101: value = timer->counter[0].reload_value.b[1]; break;

        case 0x04000102: value = gba_timer_read_counter_lsb(timer->counter + 0); break;
        case 0x04000103: value = 0x00; break;

        case 0x04000104: value = timer->counter[1].reload_value.b[0]; break;
        case 0x04000105: value = timer->counter[1].reload_value.b[1]; break;

        case 0x04000106: value = gba_timer_read_counter_lsb(timer->counter + 1); break;
        case 0x04000107: value = 0x00; break;

        case 0x04000108: value = timer->counter[2].reload_value.b[0]; break;
        case 0x04000109: value = timer->counter[2].reload_value.b[1]; break;

        case 0x0400010A: value = gba_timer_read_counter_lsb(timer->counter + 2); break;
        case 0x0400010B: value = 0x00; break;

        case 0x0400010C: value = timer->counter[3].reload_value.b[0]; break;
        case 0x0400010D: value = timer->counter[3].reload_value.b[1]; break;
        
        case 0x4000010E: value = gba_timer_read_counter_lsb(timer->counter + 3); break;
        case 0x0400010F: value = 0x00; break;
    }

    gba_timer_schedule_next_event(timer);
    
    return value;
}


void gba_timer_reset(gba_timer_t* timer){
    memset(timer->counter,0,sizeof(timer->counter));
    gba_timer_schedule_next_event(timer);
}