#pragma once

#include "utils.h"

typedef enum _gb_cpu_state_t {
    gb_cpu_running_state,
    gb_cpu_halted_state,
    gb_cpu_stopped_state,
    gb_cpu_state_count,
} gb_cpu_state;

extern const char* gb_cpu_state_names[3];

typedef enum _gb_cpu_flag_t {
    gb_cpu_carry_flag = 0x10,       // C
    gb_cpu_half_carry_flag = 0x20,  // H
    gb_cpu_subtraction_flag = 0x40, // N
    gb_cpu_zero_flag = 0x80,        // Z
    
    gb_cpu_nh_flag = gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,
    gb_cpu_znh_flag = gb_cpu_zero_flag | gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,
    gb_cpu_nhc_flag = gb_cpu_subtraction_flag | gb_cpu_half_carry_flag | gb_cpu_carry_flag
} gb_cpu_flag_t;

typedef struct _gb_cpu_t {
    gb_t* gb;
    
    uint8_t state;

    uint8_t opcode;
    
    bool halt_bug;
    uint32_t halt_cycles;

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
    uint16_t instruction_pc;

    void (*execute)(struct _gb_cpu_t*);
} gb_cpu_t;


#ifdef __cplusplus
extern "C" {
#endif

void gb_cpu_init(gb_cpu_t* cpu,gb_t* gb);

void gb_cpu_set_state(gb_cpu_t* cpu,uint8_t state);

void gb_cpu_reset(gb_cpu_t* cpu);
void gb_cpu_skip_boot(gb_cpu_t* cpu);

void gb_cpu_save_state(gb_cpu_t* cpu,gb_state_t* state);
void gb_cpu_load_state(gb_cpu_t* cpu,gb_state_t* state);

#ifdef __cplusplus
}
#endif