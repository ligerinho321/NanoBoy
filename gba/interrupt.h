#pragma once

#include "utils.h"

typedef enum _gba_interrupt_flag_t {
    gba_interrupt_lcd_vblank_flag = 0x0001,
    gba_interrupt_lcd_hblank_flag = 0x0002,
    gba_interrupt_lcd_vcount_flag = 0x0004,
    gba_interrupt_timer0_flag     = 0x0008,
    gba_interrupt_timer1_flag     = 0x0010,
    gba_interrupt_timer2_flag     = 0x0020,
    gba_interrupt_timer3_flag     = 0x0040,
    gba_interrupt_serial_flag     = 0x0080,
    gba_interrupt_dma0_flag       = 0x0100,
    gba_interrupt_dma1_flag       = 0x0200,
    gba_interrupt_dma2_flag       = 0x0400,
    gba_interrupt_dma3_flag       = 0x0800,
    gba_interrupt_keypad_flag     = 0x1000,
    gba_interrupt_gamepak_flag    = 0x2000
} gba_interrupt_flag_t;

typedef struct _gba_interrupt_t {
    gba_t* gba;

    gba_halfword_t enable;
    gba_halfword_t flag;
    bool master_enable;
} gba_interrupt_t;

#ifdef __cplusplus
extern "C" {
#endif

void gba_interrupt_init(gba_interrupt_t* interrupt,gba_t* gba);

void gba_interrupt_reset(gba_interrupt_t* interrupt);

#ifdef __cplusplus
}
#endif