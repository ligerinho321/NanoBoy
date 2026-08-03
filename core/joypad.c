#include "joypad.h"
#include "gb.h"

static const char* button_names[] = {
    "Down",
    "Up",
    "Left",
    "Right",
    "Start",
    "Select",
    "B",
    "A"
};

void gb_joypad_init(gb_joypad_t* joypad,gb_t* gb){
    joypad->gb = gb;

    joypad->register_handler = (gb_memory_handler_t){
        gb_joypad_write_register,
        gb_joypad_read_register,
        joypad
    };
}

const char* gb_joypad_get_button_name(int button){
    if(button < 0 || button >= gb_button_count){
        return "";
    }
    return button_names[button];
}

void gb_joypad_set_callback(gb_joypad_t* joypad,gb_joypad_callback_t callback,void* data){
    joypad->callback = callback;
    joypad->callback_data = data;
}

void gb_joypad_remove_callback(gb_joypad_t* joypad){
    joypad->callback = NULL;
    joypad->callback_data = NULL;
}


void gb_joypad_update(gb_joypad_t* joypad){
    if(!joypad->callback) return;

    gb_joypad_state_t new_state = {0};

    joypad->callback(joypad->callback_data,&new_state);

    bool new_edge = false;

    if(joypad->select_buttons){
        new_edge |= new_state.start || new_state.select || new_state.b || new_state.a;
    }
    if(joypad->select_directions){
        new_edge |= new_state.down || new_state.up || new_state.left || new_state.right;
    }

    if(!joypad->current_edge && new_edge){
        joypad->gb->interrupt.flag |= gb_interrupt_joypad_flag;
    }

    joypad->state = new_state;
    joypad->current_edge = new_edge;
}


void gb_joypad_write_register(void* data,uint8_t value,uint16_t address){
    gb_joypad_t* joypad = (gb_joypad_t*)data;
    joypad->select_buttons = !(value & 0x20);
    joypad->select_directions = !(value & 0x10);
}

uint8_t gb_joypad_read_register(void* data,uint16_t address){
    gb_joypad_t* joypad = (gb_joypad_t*)data;

    uint8_t value = 0xFF;

    if(joypad->select_buttons){
        value &= ~(
            0x20 | 
            (joypad->state.start ? 0x08 : 0x00) | 
            (joypad->state.select ? 0x04 : 0x00) | 
            (joypad->state.b ? 0x02 : 0x00) | 
            (joypad->state.a ? 0x01 : 0x00)
        );
    }

    if(joypad->select_directions){
        value &= ~(
            0x10 |
            (joypad->state.down ? 0x08 : 0x00) |
            (joypad->state.up ? 0x04 : 0x00) |
            (joypad->state.left ? 0x02 : 0x00) |
            (joypad->state.right ? 0x01 : 0x00)
        );
    }

    return value;
}


bool gb_joypad_is_any_button_pressed(gb_joypad_t* joypad){
    bool p = false;
    if(joypad->select_buttons){
        p |= joypad->state.start || joypad->state.select || joypad->state.b || joypad->state.a;
    }
    if(joypad->select_directions){
        p |= joypad->state.down || joypad->state.up || joypad->state.left || joypad->state.right;
    }
    return p;
}


void gb_joypad_map_registers(gb_joypad_t* joypad){
    gb_memory_map(&joypad->gb->memory,&joypad->register_handler,0xFF00);
}


void gb_joypad_reset(gb_joypad_t* joypad){
    joypad->select_buttons = false;
    joypad->select_directions = false;
    
    joypad->state.down = false;
    joypad->state.up = false;
    joypad->state.left = false;
    joypad->state.right = false;

    joypad->state.start = false;
    joypad->state.select = false;
    joypad->state.b = false;
    joypad->state.a = false;

    joypad->current_edge = false;
}


void gb_joypad_save_state(gb_joypad_t* joypad,gb_state_t* state){
    gb_state_write(state,joypad->select_buttons);
    gb_state_write(state,joypad->select_directions);
    gb_state_write(state,joypad->state);
    gb_state_write(state,joypad->current_edge);
}

void gb_joypad_load_state(gb_joypad_t* joypad,gb_state_t* state){
    gb_state_read(state,joypad->select_buttons);
    gb_state_read(state,joypad->select_directions);
    gb_state_read(state,joypad->state);
    gb_state_read(state,joypad->current_edge);
}