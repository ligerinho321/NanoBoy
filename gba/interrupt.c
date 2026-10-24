#include "interrupt.h"
#include "gba.h"

void gba_interrupt_init(gba_interrupt_t* interrupt,gba_t* gba){
    interrupt->gba = gba;
}

void gba_interrupt_reset(gba_interrupt_t* interrupt){
    interrupt->enable.v = 0x0000;
    interrupt->flag.v = 0x0000;
    interrupt->master_enable = false;
}