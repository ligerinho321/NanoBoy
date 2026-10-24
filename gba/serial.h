#pragma once

#include "utils.h"

typedef union _gba_serial_infrared_t {
    struct {
        bool signal_sent : 1;
        bool read_enabled : 1;
        bool signal_received : 1;
        bool amp_operation : 1;
        bool irq_enabled : 1;
        uint8_t unused : 3;
    };
    uint8_t v;
} gba_serial_infrared_t;

typedef struct _gba_serial_t {
    gba_t* gba;

    gba_halfword_t multiplayer_data[4];
    gba_halfword_t control;
    gba_halfword_t send_data;
    gba_halfword_t selection_mode;
    
    gba_serial_infrared_t infrared;

    uint8_t joy_bus_control;
    gba_word_t joy_receive_data;
    gba_word_t joy_send_data;
    uint8_t joy_status;

} gba_serial_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_serial_init(gba_serial_t* serial,gba_t* gba);

void gba_serial_write_register1_raw(gba_serial_t* serial,uint8_t value,uint32_t address);
uint8_t gba_serial_read_register1_raw(gba_serial_t* serial,uint32_t address);

void gba_serial_write_register2_raw(gba_serial_t* serial,uint8_t value,uint32_t address);
uint8_t gba_serial_read_register2_raw(gba_serial_t* serial,uint32_t address);

void gba_serial_reset(gba_serial_t* serial);

#ifdef __cplusplus
}
#endif