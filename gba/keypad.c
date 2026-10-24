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


void gba_keypad_write_register_raw(gba_keypad_t* keypad,uint8_t value,uint32_t address){
    switch(address){
        // Keypad Status
        case 0x04000130: break;
        case 0x04000131: break;
        // Keypad Control
        case 0x04000132:{
            gba_keypad_control_t* control = &keypad->control;
            control->a = (value & 0x01) != 0x00;
            control->b = (value & 0x02) != 0x00;
            control->select = (value & 0x04) != 0x00;
            control->start = (value & 0x08) != 0x00;
            control->right = (value & 0x10) != 0x00;
            control->left = (value & 0x20) != 0x00;
            control->up = (value & 0x40) != 0x00;
            control->down = (value & 0x80) != 0x00;
            break;
        }
        case 0x04000133:{
            gba_keypad_control_t* control = &keypad->control;
            control->r = (value & 0x01) != 0x00;
            control->l = (value & 0x02) != 0x00;
            control->irq_enabled = (value & 0x40) != 0x00;
            control->irq_condition = (value & 0x80) != 0x00;
            break;
        }
    }
}

uint8_t gba_keypad_read_register_raw(gba_keypad_t* keypad,uint32_t address){

    uint8_t value = keypad->gba->memory.bus.b[address & 0x03];

    switch(address){
        // Keypad Status
        case 0x04000130:{
            gba_keypad_status_t* status = &keypad->status;
            value = (
                (status->a ? 0x00 : 0x01) |
                (status->b ? 0x00 : 0x02) |
                (status->select ? 0x00 : 0x04) |
                (status->start ? 0x00 : 0x08) |
                (status->right ? 0x00 : 0x10) |
                (status->left ? 0x00 : 0x20) |
                (status->up ? 0x00 : 0x40) |
                (status->down ? 0x00 : 0x80)
            );
            break;
        }
        case 0x04000131:{
            gba_keypad_status_t* status = &keypad->status;
            value = (
                (status->r ? 0x00 : 0x01) |
                (status->l ? 0x00 : 0x02)
            );
            break;
        }
        // Keypad Control
        case 0x04000132:{
            gba_keypad_control_t* control = &keypad->control;
            value = (
                (control->a ? 0x01 : 0x00) |
                (control->b ? 0x02 : 0x00) |
                (control->select ? 0x04 : 0x00) |
                (control->start ? 0x08 : 0x00) |
                (control->right ? 0x10 : 0x00) |
                (control->left ? 0x20 : 0x00) |
                (control->up ? 0x40 : 0x00) |
                (control->down ? 0x80 : 0x00)
            );
            break;
        }
        case 0x04000133:{
            gba_keypad_control_t* control = &keypad->control;
            value = (
                (control->r ? 0x01 : 0x00) |
                (control->l ? 0x02 : 0x00) |
                (control->irq_enabled ? 0x40 : 0x00) |
                (control->irq_condition ? 0x80 : 0x00)
            );
            break;
        }
    }

    return value;
}


void gba_keypad_reset(gba_keypad_t* keypad){

}