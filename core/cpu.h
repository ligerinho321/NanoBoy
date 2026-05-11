#pragma once

#include "./utils.h"

typedef struct _gb_t gb_t;

typedef enum _gb_cpu_flag_t {
    gb_cpu_carry_flag = 0x10,       // C
    gb_cpu_half_carry_flag = 0x20,  // H
    gb_cpu_subtraction_flag = 0x40, // N
    gb_cpu_zero_flag = 0x80         // Z
} gb_cpu_flag_t;

typedef struct _gb_cpu_t {
    gb_t* gb;
    
    uint8_t opcode;
    
    bool halted;
    bool halt_fetch;

    bool ime_pending;
    bool ime;

    union{
        struct{
            uint16_t af;
            uint16_t bc;
            uint16_t de;
            uint16_t hl;
        };
        struct{
            uint8_t f;
            uint8_t a;
            uint8_t c;
            uint8_t b;
            uint8_t e;
            uint8_t d;
            uint8_t l;
            uint8_t h;
        };
    };

    uint16_t sp;
    uint16_t pc;
} gb_cpu_t;

void gb_cpu_init(gb_cpu_t* cpu,gb_t* gb);

void gb_cpu_execute(gb_cpu_t* cpu);

void gb_cpu_reset(gb_cpu_t* cpu);