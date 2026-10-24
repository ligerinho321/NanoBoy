#include "serial.h"
#include "gba.h"

void gba_serial_init(gba_serial_t* serial,gba_t* gba){
    serial->gba = gba;
}


static inline void gba_serial_write_infrared_lsb(gba_serial_infrared_t* infrared,uint8_t value){
    infrared->signal_sent = (value & 0x01) != 0x00;
    infrared->read_enabled = (value & 0x02) != 0x00;
    infrared->amp_operation = (value & 0x08) != 0x00;
    infrared->irq_enabled = (value & 0x10) != 0x00;
}

static inline uint8_t gba_serial_read_infrared_lsb(gba_serial_infrared_t* infrared){
    return (
        (infrared->signal_sent ? 0x01 : 0x00) |
        (infrared->read_enabled ? 0x02 : 0x00) |
        (infrared->signal_received ? 0x04 : 0x00) |
        (infrared->amp_operation ? 0x08 : 0x00) |
        (infrared->irq_enabled ? 0x10 : 0x00)
    );
}


void gba_serial_write_register1_raw(gba_serial_t* serial,uint8_t value,uint32_t address){
    switch(address){
        // Multi-Player Data 0
        case 0x04000120: serial->multiplayer_data[0].b[0] = value; break;
        case 0x04000121: serial->multiplayer_data[0].b[1] = value; break;
        // Multi-Player Data 1
        case 0x04000122: serial->multiplayer_data[1].b[0] = value; break;
        case 0x04000123: serial->multiplayer_data[1].b[1] = value; break;

        // Multi-Player Data 2
        case 0x04000124: serial->multiplayer_data[2].b[0] = value; break;
        case 0x04000125: serial->multiplayer_data[2].b[1] = value; break;
        // Multi-Player Data 3
        case 0x04000126: serial->multiplayer_data[3].b[0] = value; break;
        case 0x04000127: serial->multiplayer_data[3].b[1] = value; break;

        // Control
        case 0x04000128: serial->control.b[0] = value; break;
        case 0x04000129: serial->control.b[1] = value; break;
        // Send Data
        case 0x0400012A: serial->send_data.b[0] = value; break;
        case 0x0400012B: serial->send_data.b[1] = value; break;

        //Unused
        case 0x0400012C: break;
        case 0x0400012D: break;
        case 0x0400012E: break;
        case 0x0400012F: break;
    }
}

uint8_t gba_serial_read_register1_raw(gba_serial_t* serial,uint32_t address){
    uint8_t value = serial->gba->memory.bus.b[address & 0x03];

    switch(address){
        // Multi-Player Data 0
        case 0x04000120: value = serial->multiplayer_data[0].b[0]; break;
        case 0x04000121: value = serial->multiplayer_data[0].b[1]; break;
        // Multi-Player Data 1
        case 0x04000122: value = serial->multiplayer_data[1].b[0]; break;
        case 0x04000123: value = serial->multiplayer_data[1].b[1]; break;

        // Multi-Player Data 2
        case 0x04000124: value = serial->multiplayer_data[2].b[0]; break;
        case 0x04000125: value = serial->multiplayer_data[2].b[1]; break;
        // Multi-Player Data 3
        case 0x04000126: value = serial->multiplayer_data[3].b[0]; break;
        case 0x04000127: value = serial->multiplayer_data[3].b[1]; break;

        // Control
        case 0x04000128: value = serial->control.b[0]; break;
        case 0x04000129: value = serial->control.b[1]; break;
        // Send Data
        case 0x0400012A: value = serial->send_data.b[0]; break;
        case 0x0400012B: value = serial->send_data.b[1]; break;
    }

    return value;
}


void gba_serial_write_register2_raw(gba_serial_t* serial,uint8_t value,uint32_t address){
    switch(address){
        // Selection Mode
        case 0x04000134: serial->selection_mode.b[0] = value; break;
        case 0x04000135: serial->selection_mode.b[1] = value & 0xC1; break;
        // Infrared
        case 0x04000136: gba_serial_write_infrared_lsb(&serial->infrared,value); break;
        case 0x04000137: break;

        // JOY Bus Control
        case 0x04000140: serial->joy_bus_control = value & 0x47; break;
        case 0x04000141: break;
        case 0x04000142: break;
        case 0x04000143: break;

        // JOY Receive Data
        case 0x04000150: serial->joy_receive_data.b[0] = value; break;
        case 0x04000151: serial->joy_receive_data.b[1] = value; break;
        case 0x04000152: serial->joy_receive_data.b[2] = value; break;
        case 0x04000153: serial->joy_receive_data.b[3] = value; break;

        // JOY Send Data
        case 0x04000154: serial->joy_send_data.b[0] = value; break;
        case 0x04000155: serial->joy_send_data.b[1] = value; break;
        case 0x04000156: serial->joy_send_data.b[2] = value; break;
        case 0x04000157: serial->joy_send_data.b[3] = value; break;

        // JOY Status
        case 0x04000158: serial->joy_status = (serial->joy_status & ~0x30) | (value & 0x30); break;
        case 0x04000159: break;
        case 0x0400015A: break;
        case 0x0400015B: break;
    }
}

uint8_t gba_serial_read_register2_raw(gba_serial_t* serial,uint32_t address){
    uint8_t value = serial->gba->memory.bus.b[address & 0x03];

    switch(address){
        // Selection Mode
        case 0x04000134: value = serial->selection_mode.b[0]; break;
        case 0x04000135: value = serial->selection_mode.b[1] & 0xC1; break;
        // Infrared
        case 0x04000136: value = gba_serial_read_infrared_lsb(&serial->infrared); break;
        case 0x04000137: value = 0x00; break;

        // JOY Bus Control
        case 0x04000140: value = serial->joy_bus_control & 0x47; break;
        case 0x04000141: value = 0x00; break;
        case 0x04000142: value = 0x00; break;
        case 0x04000143: value = 0x00; break;

        // JOY Receive Data
        case 0x04000150: value = serial->joy_receive_data.b[0]; break;
        case 0x04000151: value = serial->joy_receive_data.b[1]; break;
        case 0x04000152: value = serial->joy_receive_data.b[2]; break;
        case 0x04000153: value = serial->joy_receive_data.b[3]; break;

        // JOY Send Data
        case 0x04000154: value = serial->joy_send_data.b[0]; break;
        case 0x04000155: value = serial->joy_send_data.b[1]; break;
        case 0x04000156: value = serial->joy_send_data.b[2]; break;
        case 0x04000157: value = serial->joy_send_data.b[3]; break;

        // JOY Status
        case 0x04000158: value = serial->joy_status & 0x3A; break;
        case 0x04000159: value = 0x00; break;
        case 0x0400015A: value = 0x00; break;
        case 0x0400015B: value = 0x00; break;
    }

    return value;
}


void gba_serial_reset(gba_serial_t* serial){
    memset(serial->multiplayer_data,0x00,sizeof(serial->multiplayer_data));
    
    serial->control.v = 0x00;
    serial->send_data.v = 0x00;
    serial->selection_mode.v = 0x00;

    serial->infrared.v = 0x00;

    serial->joy_bus_control = 0x00;
    serial->joy_receive_data.v = 0x00;
    serial->joy_send_data.v = 0x00;
    serial->joy_status = 0x00;
}