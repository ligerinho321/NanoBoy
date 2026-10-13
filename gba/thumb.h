#pragma once

#include "utils.h"

typedef struct _gba_cpu_t gba_cpu_t;

typedef void (*gba_cpu_thumb_instruction_callback_t)(gba_cpu_t*,uint16_t);

extern gba_cpu_thumb_instruction_callback_t thumb_table[512];

#ifdef __cplusplus
extern "C" {
#endif

void gba_cpu_thumb_init_table();

void gba_cpu_thumb_move_shifted_register(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_add_subtract(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_mov_cmp_add_sub_immediate(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_alu_operations(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_hi_register_and_branch_exchange(gba_cpu_t* cpu,uint16_t opcode);

void gba_cpu_thumb_pc_relative_load(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_load_store_with_register_offset(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_load_store_sign_extended_byte_halfword(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_load_store_with_immediate_offset(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_load_store_halfword(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_sp_relative_load_store(gba_cpu_t* cpu,uint16_t opcode);

void gba_cpu_thumb_load_address(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_add_offset_to_stack_pointer(gba_cpu_t* cpu,uint16_t opcode);

void gba_cpu_thumb_push_pop_registers(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_multiple_load_store(gba_cpu_t* cpu,uint16_t opcode);

void gba_cpu_thumb_conditional_branch(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_swi(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_unconditional_branch(gba_cpu_t* cpu,uint16_t opcode);
void gba_cpu_thumb_long_branch_with_link(gba_cpu_t* cpu,uint16_t opcode);

void gba_cpu_thumb_undefined(gba_cpu_t* cpu,uint16_t opcode);

#ifdef __cplusplus
}
#endif