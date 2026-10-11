#pragma once

#include "utils.h"

typedef struct _gba_cpu_t gba_cpu_t;

typedef void (*gba_cpu_arm_instruction_callback_t)(gba_cpu_t*,uint32_t);

extern gba_cpu_arm_instruction_callback_t arm_table[4096];

#ifdef __cplusplus
extern "C" {
#endif

void gba_cpu_arm_init_table();

void gba_cpu_arm_data_processing(gba_cpu_t* cpu,uint32_t opcode);
void gba_cpu_arm_msr(gba_cpu_t* cpu,uint32_t opcode);
void gba_cpu_arm_mrs(gba_cpu_t* cpu,uint32_t opcode);

void gba_cpu_arm_multiply(gba_cpu_t* cpu,uint32_t opcode);
void gba_cpu_arm_multiply_long(gba_cpu_t* cpu,uint32_t opcode);

void gba_cpu_arm_single_data_swap_byte(gba_cpu_t* cpu,uint32_t opcode);
void gba_cpu_arm_single_data_swap_word(gba_cpu_t* cpu,uint32_t opcode);

void gba_cpu_arm_single_data_transfer(gba_cpu_t* cpu,uint32_t opcode);
void gba_cpu_arm_block_data_transfer(gba_cpu_t* cpu,uint32_t opcode);
void gba_cpu_arm_halfword_data_transfer(gba_cpu_t* cpu,uint32_t opcode);

void gba_cpu_arm_branch_exchange(gba_cpu_t* cpu,uint32_t opcode);
void gba_cpu_arm_branch_and_branch_link(gba_cpu_t* cpu,uint32_t opcode);

void gba_cpu_arm_undefined(gba_cpu_t* cpu,uint32_t opcode);

void gba_cpu_arm_swi(gba_cpu_t* cpu,uint32_t opcode);

#ifdef _cplusplus
}
#endif