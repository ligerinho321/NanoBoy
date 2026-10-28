#include "keypad.h"
#include "gba.h"

const char* gba_keypad_button_names[gba_button_count] = {
    "A",
    "B",
    "Select",
    "Start",
    "Right",
    "Left",
    "Up",
    "Down",
    "R",
    "L"
};


void gba_keypad_init(gba_keypad_t* keypad,gba_t* gba){
    keypad->gba = gba;
}


void gba_keypad_set_callback(gba_t* gba,gba_keypad_callback_t callback,void* data){
    gba->keypad.callback = callback;
    gba->keypad.callback_data = data;
}

void gba_keypad_remove_callback(gba_t* gba){
    gba->keypad.callback = NULL;
    gba->keypad.callback_data = NULL;

    memset(gba->keypad.status,0x00,sizeof(gba->keypad.status));
}


void gba_keypad_update(gba_keypad_t* keypad){

    if(!keypad->callback) return;

    keypad->callback(keypad->callback_data,keypad->status);

    if(!keypad->irq_enabled) return;

    bool irq = false;

    if(keypad->irq_condition){
        for(int i = 0; i < gba_button_count; ++i){

            if(!keypad->select[i]) continue;

            if(keypad->status[i]){
                irq = true;
            }
            else{
                irq = false;
                break;
            }
        }
    }
    else{
        for(int i = 0; i < gba_button_count; ++i){
            
            if(!keypad->select[i]) continue;

            if(keypad->status[i]){
                irq = true;
                break;
            }
        }
    }

    if(irq){
        keypad->gba->interrupt.flag.v |= gba_interrupt_keypad_flag;
    }
}


void gba_keypad_write_register_raw(gba_keypad_t* keypad,uint8_t value,uint32_t address){
    switch(address){
        // Keypad Status
        case 0x04000130: break;
        case 0x04000131: break;
        // Keypad Control
        case 0x04000132:{
            uint8_t* select = keypad->select;
            select[gba_button_a] = (value & 0x01) != 0x00;
            select[gba_button_b] = (value & 0x02) != 0x00;
            select[gba_button_select] = (value & 0x04) != 0x00;
            select[gba_button_start] = (value & 0x08) != 0x00;
            select[gba_button_right] = (value & 0x10) != 0x00;
            select[gba_button_left] = (value & 0x20) != 0x00;
            select[gba_button_up] = (value & 0x40) != 0x00;
            select[gba_button_down] = (value & 0x80) != 0x00;
            break;
        }
        case 0x04000133:{
            uint8_t* select = keypad->select;
            select[gba_button_r] = (value & 0x01) != 0x00;
            select[gba_button_l] = (value & 0x02) != 0x00;
            keypad->irq_enabled = (value & 0x40) != 0x00;
            keypad->irq_condition = (value & 0x80) != 0x00;
            break;
        }
    }
}

uint8_t gba_keypad_read_register_raw(gba_keypad_t* keypad,uint32_t address){

    uint8_t value = keypad->gba->memory.bus.b[address & 0x03];

    switch(address){
        // Keypad Status
        case 0x04000130:{
            uint8_t* status = keypad->status;
            value = (
                (status[gba_button_a] ? 0x00 : 0x01) |
                (status[gba_button_b] ? 0x00 : 0x02) |
                (status[gba_button_select] ? 0x00 : 0x04) |
                (status[gba_button_start] ? 0x00 : 0x08) |
                (status[gba_button_right] ? 0x00 : 0x10) |
                (status[gba_button_left] ? 0x00 : 0x20) |
                (status[gba_button_up] ? 0x00 : 0x40) |
                (status[gba_button_down] ? 0x00 : 0x80)
            );
            break;
        }
        case 0x04000131:{
            uint8_t* status = keypad->status;
            value = (
                (status[gba_button_r] ? 0x00 : 0x01) |
                (status[gba_button_l] ? 0x00 : 0x02)
            );
            break;
        }
        // Keypad Control
        case 0x04000132:{
            uint8_t* select = keypad->select;
            value = (
                (select[gba_button_a] ? 0x01 : 0x00) |
                (select[gba_button_b] ? 0x02 : 0x00) |
                (select[gba_button_select] ? 0x04 : 0x00) |
                (select[gba_button_start] ? 0x08 : 0x00) |
                (select[gba_button_right] ? 0x10 : 0x00) |
                (select[gba_button_left] ? 0x20 : 0x00) |
                (select[gba_button_up] ? 0x40 : 0x00) |
                (select[gba_button_down] ? 0x80 : 0x00)
            );
            break;
        }
        case 0x04000133:{
            uint8_t* select = keypad->select;
            value = (
                (select[gba_button_r] ? 0x01 : 0x00) |
                (select[gba_button_l] ? 0x02 : 0x00) |
                (keypad->irq_enabled ? 0x40 : 0x00) |
                (keypad->irq_condition ? 0x80 : 0x00)
            );
            break;
        }
    }

    return value;
}


void gba_keypad_reset(gba_keypad_t* keypad){
    memset(keypad->status,0x00,sizeof(keypad->status));
    memset(keypad->select,0x00,sizeof(keypad->select));
    keypad->irq_enabled = false;
    keypad->irq_condition = false;
}