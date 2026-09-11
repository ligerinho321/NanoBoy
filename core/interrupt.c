#include "interrupt.h"
#include "gb.h"

void gb_interrupt_init(gb_interrupt_t* interrupt,gb_t* gb){
    interrupt->gb = gb;

    interrupt->flag_register_descriptor = (gb_memory_descriptor_t){
        gb_interrupt_write_flag_register,
        gb_interrupt_read_flag_register,
        interrupt
    };

    interrupt->enable_register_descriptor = (gb_memory_descriptor_t){
        gb_interrupt_write_enable_register,
        gb_interrupt_read_enable_register,
        interrupt
    };
}


void gb_interrupt_write_flag_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;
    
    interrupt->state.flag = value & 0x1F;
}

uint8_t gb_interrupt_read_flag_register(void* data,uint16_t address){
    gb_unused(address);

    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;
    
    return 0xE0 | (interrupt->state.flag & 0x1F);
}


void gb_interrupt_write_enable_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;
    
    interrupt->state.enable = value;
}

uint8_t gb_interrupt_read_enable_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;

    return interrupt->state.enable;
}


uint8_t gb_interrupt_get_vector(gb_interrupt_t* interrupt){
    
    gb_interrupt_state_t* state = &interrupt->state;

    uint8_t vector = 0x00;

    for(uint8_t bit = gb_interrupt_vblank_flag; bit <= gb_interrupt_joypad_flag; bit <<= 0x01){
        
        if(!(state->enable & bit) || !(state->flag & bit)) continue;

        switch(bit){
            case gb_interrupt_vblank_flag: vector = gb_interrupt_vblank_vector; break;
            case gb_interrupt_lcd_flag:    vector = gb_interrupt_lcd_vector; break;
            case gb_interrupt_timer_flag:  vector = gb_interrupt_timer_vector; break;
            case gb_interrupt_serial_flag: vector = gb_interrupt_serial_vector; break;
            case gb_interrupt_joypad_flag: vector = gb_interrupt_joypad_vector; break;
        }

        state->flag &= ~bit;

        break;
    }

    return vector;
}


void gb_interrupt_map_registers(gb_interrupt_t* interrupt){
    gb_memory_t* memory = &interrupt->gb->memory;
    gb_memory_map(memory,&interrupt->flag_register_descriptor,0xFF0F);
    gb_memory_map(memory,&interrupt->enable_register_descriptor,0xFFFF);
}


void gb_interrupt_reset(gb_interrupt_t* interrupt){
    interrupt->state.enable = 0x00;
    interrupt->state.flag = 0x00;
}

void gb_interrupt_skip_boot(gb_interrupt_t* interrupt){
    interrupt->state.flag = 0x01;
}



void gb_interrupt_save_state(gb_interrupt_t* interrupt,gb_snapshot_t* snapshot){
    snapshot->interrupt = interrupt->state;
}

void gb_interrupt_load_state(gb_interrupt_t* interrupt,gb_snapshot_t* snapshot){
    interrupt->state = snapshot->interrupt;
}