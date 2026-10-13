#pragma once

#include "utils.h"

typedef enum _gba_cpu_flag_t {
    gba_cpu_mode_flag     = 0x0000001F,
    gba_cpu_thumb_flag    = 0x00000020,
    gba_cpu_fiq_flag      = 0x00000040,
    gba_cpu_irq_flag      = 0x00000080,
    gba_cpu_overflow_flag = 0x10000000, // V
    gba_cpu_carry_flag    = 0x20000000, // C
    gba_cpu_zero_flag     = 0x40000000, // Z
    gba_cpu_negative_flag = 0x80000000  // N
} gba_cpu_flag_t;

typedef enum _gba_cpu_mode_t {
    gba_cpu_user_mode       = 0x10,
    gba_cpu_fiq_mode        = 0x11,
    gba_cpu_irq_mode        = 0x12,
    gba_cpu_supervisor_mode = 0x13,
    gba_cpu_abort_mode      = 0x17,
    gba_cpu_undefined_mode  = 0x1B,
    gba_cpu_system_mode     = 0x1F
} gba_cpu_mode_t;

typedef enum _gba_cpu_mode_bank_t {
    gba_cpu_none_bank,
    gba_cpu_fiq_bank,
    gba_cpu_irq_bank,
    gba_cpu_supervisor_bank,
    gba_cpu_abort_bank,
    gba_cpu_undefined_bank,
    gba_cpu_invalid_bank
} gba_cpu_mode_bank_t;

typedef enum _gba_cpu_exception_vector_t {
                                        // Vector         Mode        Priority
    gba_cpu_reset_vector                 = 0x00000000, // Supervisor  1
    gba_cpu_undefined_vector             = 0x00000004, // Undefined   6
    gba_cpu_software_interrupt_vector    = 0x00000008, // Supervisor  6
    gba_cpu_prefetch_abort_vector        = 0x0000000C, // Abort       5
    gba_cpu_data_abort_vector            = 0x00000010, // Abort       2
    gba_cpu_irq_vector                   = 0x00000018, // IRQ         4
    gba_cpu_fiq_vector                   = 0x0000001C  // FIQ         3
} gba_cpu_exception_vector_t;

typedef enum _gba_cpu_condition_t {
    gba_cpu_eq_condition, // Z
    gba_cpu_ne_condition, // !Z
    gba_cpu_cs_condition, // C
    gba_cpu_cc_condition, // !C
    gba_cpu_mi_condition, // N
    gba_cpu_pl_condition, // !N
    gba_cpu_vs_condition, // V
    gba_cpu_vc_condition, // !V
    gba_cpu_hi_condition, // C && !Z
    gba_cpu_ls_condition, // !C && Z
    gba_cpu_ge_condition, // N == V
    gba_cpu_lt_condition, // N != V
    gba_cpu_gt_condition, // !Z && (N == V),
    gba_cpu_le_condition, // Z || (N != V),
    gba_cpu_al_condition, // always
    gba_cpu_re_condition, // reserved
} gba_cpu_condition_t;

typedef enum _gba_cpu_alu_operation_t {
    gba_cpu_and_operation,
    gba_cpu_eor_operation,
    gba_cpu_sub_operation,
    gba_cpu_lsl_operation = gba_cpu_sub_operation,
    gba_cpu_rsb_operation,
    gba_cpu_lsr_operation = gba_cpu_rsb_operation,
    gba_cpu_add_operation,
    gba_cpu_asr_operation = gba_cpu_add_operation,
    gba_cpu_adc_operation,
    gba_cpu_sbc_operation,
    gba_cpu_rsc_operation,
    gba_cpu_ror_operation = gba_cpu_rsc_operation,
    gba_cpu_tst_operation,
    gba_cpu_teq_operation,
    gba_cpu_neg_operation = gba_cpu_teq_operation,
    gba_cpu_cmp_operation,
    gba_cpu_cmn_operation,
    gba_cpu_orr_operation,
    gba_cpu_mov_operation,
    gba_cpu_mul_operation = gba_cpu_mov_operation,
    gba_cpu_bic_operation,
    gba_cpu_mvn_operation
} gba_cpu_alu_operation_t;

typedef enum _gba_cpu_shift_t {
    gba_cpu_logical_shift_left,
    gba_cpu_logical_shift_right,
    gba_cpu_arithmetic_shift_right,
    gba_cpu_rotate_shift_right
} gba_cpu_shift_t;

typedef struct _gba_cpu_pipeline_t {
    uint32_t fetch;
    uint32_t decode;
    uint32_t execute;
} gba_cpu_pipeline_t;

typedef struct _gba_cpu_t {
    gba_t* gba;

    uint32_t r8_r12[2][5];
    uint32_t r13[6];       // Stack Pointer (SP)
    uint32_t r14[6];       // Link Register (LR)

    uint32_t cpsr;
    uint32_t spsr[6];

    uint32_t r[16];

    gba_cpu_pipeline_t pipeline;
} gba_cpu_t;

extern const bool gba_cpu_condition_table[256];

extern const uint8_t gba_cpu_mode_bank[32];


#define gba_cpu_set_flag(flag,condition)\
    ((condition) ? (cpu->cpsr |= flag) : (cpu->cpsr &= ~flag))

#define gba_cpu_set_nz_flag(result)\
    gba_cpu_set_flag(gba_cpu_zero_flag,!result);\
    gba_cpu_set_flag(gba_cpu_negative_flag,result & (1 << 31));\

#define gba_cpu_set_logical_flag(result,carry)\
    gba_cpu_set_flag(gba_cpu_carry_flag,carry);\
    gba_cpu_set_nz_flag(result)\


#ifdef __cplusplus
extern "C" {
#endif

void gba_cpu_init(gba_cpu_t* cpu,gba_t* gba);

void gba_cpu_write_byte(gba_cpu_t* cpu,uint8_t value,uint32_t address);
uint8_t gba_cpu_read_byte(gba_cpu_t* cpu,uint32_t address);
uint32_t gba_cpu_read_signed_byte(gba_cpu_t* cpu,uint32_t address);

void gba_cpu_write_halfword(gba_cpu_t* cpu,uint16_t value,uint32_t address);
uint16_t gba_cpu_read_halfword(gba_cpu_t* cpu,uint32_t address);
uint32_t gba_cpu_read_halfword_rotate(gba_cpu_t* cpu,uint32_t address);
uint32_t gba_cpu_read_signed_halfword(gba_cpu_t* cpu,uint32_t address);

void gba_cpu_write_word(gba_cpu_t* cpu,uint32_t value,uint32_t address);
uint32_t gba_cpu_read_word(gba_cpu_t* cpu,uint32_t address);
uint32_t gba_cpu_read_word_rotate(gba_cpu_t* cpu,uint32_t address);


void gba_cpu_switch_mode_bank(gba_cpu_t* cpu,uint8_t new_mode);

void gba_cpu_reload_pipeline(gba_cpu_t* cpu);

uint32_t* gba_cpu_get_register(gba_cpu_t* cpu,uint8_t index,uint8_t mode);

bool gba_cpu_valid_mode(uint8_t mode);

uint32_t gba_cpu_alu_and(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags);
uint32_t gba_cpu_alu_xor(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags);
uint32_t gba_cpu_alu_orr(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags);
uint32_t gba_cpu_alu_mov(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags);
uint32_t gba_cpu_alu_bic(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags);
uint32_t gba_cpu_alu_mvn(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags);
uint32_t gba_cpu_alu_arithmetic(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags);


uint32_t gba_cpu_lsl(uint32_t value,uint8_t shift,bool* carry);
uint32_t gba_cpu_lsr(uint32_t value,uint8_t shift,bool* carry);
uint32_t gba_cpu_asr(uint32_t value,uint8_t shift,bool* carry);
uint32_t gba_cpu_ror(uint32_t value,uint8_t shift,bool* carry);
uint32_t gba_cpu_rrx(uint32_t value,bool* carry);

void gba_cpu_multiply_clock(uint32_t value,bool _signed);

#ifdef __cplusplus
}
#endif
