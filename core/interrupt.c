#include "interrupt.h"
#include "gb.h"

void gb_interrupt_init(gb_interrupt_t* interrupt,gb_t* gb){
    interrupt->gb = gb;

    interrupt->flag_register_handler = (gb_memory_handler_t){
        gb_interrupt_write_flag_register,
        gb_interrupt_read_flag_register,
        interrupt
    };

    interrupt->enable_register_handler = (gb_memory_handler_t){
        gb_interrupt_write_enable_register,
        gb_interrupt_read_enable_register,
        interrupt
    };
}


void gb_interrupt_write_flag_register(void* data,uint8_t value,uint16_t address){
    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;
    interrupt->flag = value & 0x1F;
}

uint8_t gb_interrupt_read_flag_register(void* data,uint16_t address){
    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;
    return 0xE0 | (interrupt->flag & 0x1F);
}


void gb_interrupt_write_enable_register(void* data,uint8_t value,uint16_t address){
    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;
    interrupt->enable = value;
}

uint8_t gb_interrupt_read_enable_register(void* data,uint16_t address){
    gb_interrupt_t* interrupt = (gb_interrupt_t*)data;
    return interrupt->enable;
}


uint8_t gb_interrupt_get_vector(gb_interrupt_t* interrupt){
    uint8_t vector = 0x00;

    for(uint8_t bit = gb_interrupt_vblank_flag; bit <= gb_interrupt_joypad_flag; bit <<= 0x01){
        
        if(!(interrupt->enable & bit) || !(interrupt->flag & bit)) continue;

        switch(bit){
            case gb_interrupt_vblank_flag: vector = gb_interrupt_vblank_vector; break;
            case gb_interrupt_lcd_flag:    vector = gb_interrupt_lcd_vector; break;
            case gb_interrupt_timer_flag:  vector = gb_interrupt_timer_vector; break;
            case gb_interrupt_serial_flag: vector = gb_interrupt_serial_vector; break;
            case gb_interrupt_joypad_flag: vector = gb_interrupt_joypad_vector; break;
        }

        interrupt->flag &= ~bit;

        break;
    }

    return vector;
}


void gb_interrupt_map_registers(gb_interrupt_t* interrupt){
    gb_memory_t* memory = &interrupt->gb->memory;
    gb_memory_map(memory,&interrupt->flag_register_handler,0xFF0F);
    gb_memory_map(memory,&interrupt->enable_register_handler,0xFFFF);
}


void gb_interrupt_reset(gb_interrupt_t* interrupt){
    interrupt->enable = 0x00;
    interrupt->flag = 0x00;
}