#include "joypad.h"
#include "gb.h"

const char* gb_joypad_button_names[8] = {
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

    joypad->register_descriptor = (gb_memory_descriptor_t){
        gb_joypad_write_register,
        gb_joypad_read_register,
        joypad
    };
}


void gb_joypad_set_callback(gb_t* gb,gb_joypad_callback_t callback,void* data){
    gb->joypad.callback = callback;
    gb->joypad.callback_data = data;
}

void gb_joypad_remove_callback(gb_t* gb){
    gb->joypad.callback = NULL;
    gb->joypad.callback_data = NULL;
}


void gb_joypad_update(gb_joypad_t* joypad){
    if(!joypad->callback) return;

    gb_joypad_state_t* state = &joypad->state;

    gb_joypad_button_state_t new_state = {0};

    joypad->callback(joypad->callback_data,&new_state);

    bool new_edge = false;

    if(state->select_buttons){
        new_edge |= new_state.start || new_state.select || new_state.b || new_state.a;
    }
    if(state->select_directions){
        new_edge |= new_state.down || new_state.up || new_state.left || new_state.right;
    }

    if(!state->current_edge && new_edge){
        joypad->gb->interrupt.state.flag |= gb_interrupt_joypad_flag;
    }

    state->button_state = new_state;
    state->current_edge = new_edge;
}


void gb_joypad_write_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_joypad_t* joypad = (gb_joypad_t*)data;

    joypad->state.select_buttons = !(value & 0x20);
    joypad->state.select_directions = !(value & 0x10);
}

uint8_t gb_joypad_read_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_joypad_t* joypad = (gb_joypad_t*)data;
    gb_joypad_button_state_t* button_state = &joypad->state.button_state;

    uint8_t value = 0xFF;

    if(joypad->state.select_buttons){
        value &= ~(
            0x20 | 
            (button_state->start ? 0x08 : 0x00) | 
            (button_state->select ? 0x04 : 0x00) | 
            (button_state->b ? 0x02 : 0x00) | 
            (button_state->a ? 0x01 : 0x00)
        );
    }

    if(joypad->state.select_directions){
        value &= ~(
            0x10 |
            (button_state->down ? 0x08 : 0x00) |
            (button_state->up ? 0x04 : 0x00) |
            (button_state->left ? 0x02 : 0x00) |
            (button_state->right ? 0x01 : 0x00)
        );
    }

    return value;
}


bool gb_joypad_is_any_button_pressed(gb_joypad_t* joypad){
    gb_joypad_button_state_t* button_state = &joypad->state.button_state;

    bool p = false;

    if(joypad->state.select_buttons){
        p |= button_state->start || button_state->select || button_state->b || button_state->a;
    }
    if(joypad->state.select_directions){
        p |= button_state->down || button_state->up || button_state->left || button_state->right;
    }
    return p;
}


void gb_joypad_map_registers(gb_joypad_t* joypad){
    gb_memory_map(&joypad->gb->memory,&joypad->register_descriptor,0xFF00);
}


void gb_joypad_reset(gb_joypad_t* joypad){
    
    memset(&joypad->state,0x00,sizeof(joypad->state));

    joypad->state.select_buttons = true;
}

void gb_joypad_skip_boot(gb_joypad_t* joypad){
    
    if(joypad->gb->state.is_cgb && !joypad->gb->state.cgb_mode){
        joypad->state.select_buttons = false;
        joypad->state.select_directions = false;
    }
    else{
        joypad->state.select_buttons = true;
        joypad->state.select_directions = true;
    }
}


void gb_joypad_save_state(gb_joypad_t* joypad,gb_snapshot_t* snapshot){
    snapshot->joypad = joypad->state;
}

void gb_joypad_load_state(gb_joypad_t* joypad,gb_snapshot_t* snapshot){
    joypad->state = snapshot->joypad;
}