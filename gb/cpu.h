#pragma once

#include "utils.h"

typedef enum _gb_cpu_mode_t {
    gb_cpu_running_mode,
    gb_cpu_halted_mode,
    gb_cpu_stopped_mode,
    gb_cpu_mode_count,
} gb_cpu_mode_t;

extern const char* gb_cpu_mode_names[3];

typedef enum _gb_cpu_flag_t {
    gb_cpu_carry_flag = 0x10,       // C
    gb_cpu_half_carry_flag = 0x20,  // H
    gb_cpu_subtraction_flag = 0x40, // N
    gb_cpu_zero_flag = 0x80,        // Z
    
    gb_cpu_nh_flag = gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,
    gb_cpu_znh_flag = gb_cpu_zero_flag | gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,
    gb_cpu_nhc_flag = gb_cpu_subtraction_flag | gb_cpu_half_carry_flag | gb_cpu_carry_flag
} gb_cpu_flag_t;

typedef struct _gb_cpu_state_t {
    uint8_t mode;
    
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

} gb_cpu_state_t;

typedef struct _gb_cpu_t {
    gb_t* gb;

    gb_cpu_state_t state;

    uint16_t instruction_pc;

    void (*execute)(struct _gb_cpu_t*);
} gb_cpu_t;


#ifdef __cplusplus
extern "C" {
#endif

void gb_cpu_init(gb_cpu_t* cpu,gb_t* gb);

void gb_cpu_set_mode(gb_cpu_t* cpu,uint8_t mode);

void gb_cpu_reset(gb_cpu_t* cpu);
void gb_cpu_skip_boot(gb_cpu_t* cpu);

void gb_cpu_save_state(gb_cpu_t* cpu,gb_snapshot_t* snapshot);
void gb_cpu_load_state(gb_cpu_t* cpu,gb_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif