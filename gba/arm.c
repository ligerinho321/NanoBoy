#include "arm.h"
#include "cpu.h"

gba_cpu_arm_instruction_callback_t arm_table[4096] = {gba_cpu_arm_undefined};


void gba_cpu_arm_init_table(){

    // Data Processing
    // ----_00xx_xxxx_----_----_----_xxxx_----
    for(int i = 0; i <= 0x3FF; ++i){
        arm_table[i] = gba_cpu_arm_data_processing;
    }

    // Undefined
    // ----_000x_xxxx_----_----_----_1xx1_----
    for(int i = 0; i <= 0x7F; ++i){
        arm_table[((i & 0x7C) << 0x02) | 0x08 | ((i & 0x03) << 0x01) | 0x01] = gba_cpu_arm_undefined;
    }

    // Undefined (TST,TEQ,CMP,CMN !S)
    // ----_00x1_0xx0_----_----_----_xxxx_----
    for(int i = 0; i <= 0x7F; ++i){
        arm_table[((i & 0x40) << 0x03) | 0x100 | ((i & 0x30) << 0x01) | (i & 0x0F)] = gba_cpu_arm_undefined;
    }

    // MRS
    // ----_0001_0x00_----_----_----_0000_----
    arm_table[0x100] = gba_cpu_arm_mrs;
    arm_table[0x140] = gba_cpu_arm_mrs;

    // MSR Register
    // ----_0001_0x10_----_----_----_0000_----
    arm_table[0x120] = gba_cpu_arm_msr;
    arm_table[0x160] = gba_cpu_arm_msr;

    // MSR Immediate
    // ----_0011_0x10_----_----_----_xxxx_----
    for(int i = 0; i <= 0x1F; ++i){
        arm_table[0x320 | ((i & 0x10) << 0x02) | (i & 0x0F)] = gba_cpu_arm_msr;
    }

    // Multiply
    // ----_0000_00xx_----_----_----_1001_----
    for(int i = 0; i <= 0x03; ++i){
        arm_table[(i << 0x04) | 0x09] = gba_cpu_arm_multiply;
    }
    
    //Multiply Long
    // ----_0000_1xxx_----_----_----_1001_----
    for(int i = 0; i <= 0x07; ++i){
        arm_table[0x80 | (i << 0x04) | 0x09] = gba_cpu_arm_multiply_long;
    }

    // Single Data Swap Word
    // ----_0001_0000_----_----_----_1001_----
    arm_table[0x109] = gba_cpu_arm_single_data_swap_word;

    // Single Data Swap Byte
    // ----_0001_0100_----_----_----_1001_----
    arm_table[0x149] = gba_cpu_arm_single_data_swap_byte;


    // Branch Exchange
    // ----_0001_0010_----_----_----_0001_----
    arm_table[0x121] = gba_cpu_arm_branch_exchange;


    // Halfword Data Transfer S = 0 H = 1
    // ----_000x_xxxx_----_----_----_1011_----
    for(int i = 0; i <= 0x1F; ++i){
        arm_table[(i << 4) | 0x0B] = gba_cpu_arm_halfword_data_transfer;
    }
    // Halfword Data Transfer S = 1 H = 0
    // ----_000x_xxxx_----_----_----_1101_----
    for(int i = 0; i <= 0x1F; ++i){
        arm_table[(i << 4) | 0x0D] = gba_cpu_arm_halfword_data_transfer;
    }
    // Halfword Data Transfer S = 1 H = 1
    // ----_000x_xxxx_----_----_----_1111_----
    for(int i = 0; i <= 0x1F; ++i){
        arm_table[(i << 4) | 0x0F] = gba_cpu_arm_halfword_data_transfer;
    }


    //Single Data Transfer Immediate
    // ----_010x_xxxx_----_----_----_xxxx_----
    for(int i = 0; i <= 0x1FF; ++i){
        arm_table[0x400 | i] = gba_cpu_arm_single_data_transfer;
    }

    //Single Data Transfer Register
    // ----_011x_xxxx_----_----_----_xxx0_----
    for(int i = 0; i <= 0xFF; ++i){
        arm_table[0x600 | (i << 0x01)] = gba_cpu_arm_single_data_transfer;
    }

    // Block Data Transfer
    // ----_100x_xxxx_----_----_----_xxxx_----
    for(int i = 0; i <= 0x1FF; ++i){
        arm_table[0x800 | i] = gba_cpu_arm_block_data_transfer;
    }

    // Branch and Branch Link
    // ----_101x_xxxx_----_----_----_xxxx_----
    for(int i = 0; i <= 0x1FF; ++i){
        arm_table[0xA00 | i] = gba_cpu_arm_branch_and_branch_link;
    }

    // SWI
    // ----_1111_xxxx_----_----_----_xxxx_----
    for(int i = 0; i <= 0xFF; ++i){
        arm_table[0xF00 | i] = gba_cpu_arm_swi;
    }
}


void gba_cpu_arm_data_processing(gba_cpu_t* cpu,uint32_t opcode){
    bool immediate = opcode & (1 << 25);
    uint8_t operation = (opcode >> 21) & 15;
    bool set_flags = opcode & (1 << 20);

    uint8_t rn = (opcode >> 16) & 15;
    uint8_t rd = (opcode >> 12) & 15;
    
    uint32_t op1 = cpu->r[rn];
    uint32_t op2 = 0;

    bool carry = cpu->cpsr & gba_cpu_carry_flag;

    if(immediate){
        uint32_t shift = (opcode >> 7) & 30;

        uint32_t value = opcode & 255;

        if(shift){
            op2 = (value >> shift) | (value << (32 - shift));
            carry = op2 & (1 << 31);
        }
    }
    else{
        uint8_t rm = opcode & 15;

        op2 = cpu->r[rm];

        bool shift_register = opcode & 16;

        uint8_t shift_type = (opcode >> 5) & 3;

        uint8_t shift = 0x00;

        if(shift_register){
            uint8_t rs = (opcode >> 8) & 15;
            shift = cpu->r[rs];

            // TODO: 1 I-cycle if shifted by register

            if(rn == 15){
                op1 += 4;
            }
            if(rm == 15){
                op2 += 4;
            }
        }
        else{
            shift = (opcode >> 7) & 31;
        }
        
        switch(shift_type){
            case gba_cpu_logical_shift_left:
                op2 = gba_cpu_lsl(op2,shift,&carry);
                break;
            case gba_cpu_logical_shift_right:
                op2 = gba_cpu_lsr(op2,(!shift_register && !shift) ? 32 : shift,&carry);
                break;
            case gba_cpu_arithmetic_shift_right:
                op2 = gba_cpu_asr(op2,(!shift_register && !shift) ? 32 : shift,&carry);
                break;
            case gba_cpu_rotate_shift_right:
                op2 = (!shift_register && !shift) ? gba_cpu_rrx(op2,&carry) : gba_cpu_ror(op2,shift,&carry);
                break;
        }
    }

    switch(operation){
        // Rd = Op1 & Op2
        case gba_cpu_and_operation: cpu->r[rd] = gba_cpu_alu_and(cpu,op1,op2,carry,set_flags); break;
        // Rd = Op1 ^ Op2
        case gba_cpu_eor_operation: cpu->r[rd] = gba_cpu_alu_xor(cpu,op1,op2,carry,set_flags); break;
        // Rd = Op1 - Op2
        case gba_cpu_sub_operation: cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,op1,~op2,1,set_flags); break;
        // Rd = Op2 - Op1
        case gba_cpu_rsb_operation: cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,op2,~op1,1,set_flags); break;
        // Rd = Op1 + Op2
        case gba_cpu_add_operation: cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,op1,op2,0,set_flags); break;
        // Rd = Op1 + Op2 + C
        case gba_cpu_adc_operation: cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,op1,op2,cpu->cpsr & gba_cpu_carry_flag,set_flags); break;
        // Rd = Op1 - Op2 + C - 1
        case gba_cpu_sbc_operation: cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,op1,~op2,cpu->cpsr & gba_cpu_carry_flag,set_flags); break;
        // Rd = Op2 - Op2 + C - 1
        case gba_cpu_rsc_operation: cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,op2,~op1,cpu->cpsr & gba_cpu_carry_flag,set_flags); break;
        // Op1 & Op2
        case gba_cpu_tst_operation: gba_cpu_alu_and(cpu,op1,op2,carry,true); break;
        // Op1 ^ Op2
        case gba_cpu_teq_operation: gba_cpu_alu_xor(cpu,op1,op2,carry,true); break;
        // Op1 - Op2
        case gba_cpu_cmp_operation: gba_cpu_alu_arithmetic(cpu,op1,~op2,1,true); break;
        // Op1 + Op2
        case gba_cpu_cmn_operation: gba_cpu_alu_arithmetic(cpu,op1,op2,0,true); break;
        // Rd = Op1 | Op2
        case gba_cpu_orr_operation: cpu->r[rd] = gba_cpu_alu_orr(cpu,op1,op2,carry,set_flags); break;
        // Rd = Op2
        case gba_cpu_mov_operation: cpu->r[rd] = gba_cpu_alu_mov(cpu,op1,op2,carry,set_flags); break;
        // Rd = Op1 & ~Op2
        case gba_cpu_bic_operation: cpu->r[rd] = gba_cpu_alu_bic(cpu,op1,op2,carry,set_flags); break;
        // Rd = ~Op2
        case gba_cpu_mvn_operation: cpu->r[rd] = gba_cpu_alu_mvn(cpu,op1,op2,carry,set_flags); break;
    }

    if(rd == 15){
        uint8_t mode = cpu->cpsr & gba_cpu_mode_flag;

        // TODO: What will be written to CPSR if the mode is User/System?
        if(set_flags && mode != gba_cpu_user_mode && mode != gba_cpu_system_mode){

            uint32_t new_cpsr = cpu->spsr[gba_cpu_mode_bank[mode]];

            // TODO: What happens when writing an invalid mode to CPSR?

            gba_cpu_switch_mode_bank(cpu,new_cpsr & gba_cpu_mode_flag);

            cpu->cpsr = new_cpsr;
        }

        if(operation < gba_cpu_teq_operation || operation > gba_cpu_cmn_operation){
            gba_cpu_reload_pipeline(cpu);
        }
        else{
            cpu->r[15] += 4;
        }
    }
    else{
        cpu->r[15] += 4;
    }
}

void gba_cpu_arm_msr(gba_cpu_t* cpu,uint32_t opcode){
    bool immediate = opcode & (1 << 25);
    bool is_spsr = opcode & (1 << 22);

    uint32_t mask = 0;

    // Unused PSR bits cannot be modified.

    //Flags
    if(opcode & (1 << 19)) mask |= 0xF0000000;

    // Status
    // if(opcode & (1 << 18)) mask |= 0x00FF0000;

    // Extended
    // if(opcode & (1 << 17)) mask |= 0x0000FF00;

    //Control
    if(opcode & (1 << 16)) mask |= 0x000000FF;

    uint32_t value = 0;

    if(immediate){
        uint8_t shift = (opcode >> 7) & 30;

        value = opcode & 255;

        if(shift){
            value = (value >> shift) | (value << (32 - shift));
        }
    }
    else{
        // R15 cannot be used as the source register.
        uint8_t rm = opcode & 15;
        value = cpu->r[rm];
    }

    // TODO: What happens when writing an invalid mode to CPSR or SPSR?

    uint8_t mode = cpu->cpsr & gba_cpu_mode_flag;

    if(is_spsr && mode != gba_cpu_user_mode && mode != gba_cpu_system_mode){
        uint8_t mode_bank = gba_cpu_mode_bank[mode];

        cpu->spsr[mode_bank] = (value & mask) & (cpu->spsr[mode_bank] & ~mask);
    }
    else if(!is_spsr){
        // In user mode, the CPSR control bits cannot be modified.
        if(mode == gba_cpu_user_mode){
            mask &= ~0x000000FF;
        }
        else{
            // The Thumb bit cannot be changed in the CPSR.
            mask &= ~gba_cpu_thumb_flag;
        }
        
        uint32_t new_cpsr = (value & mask) | (cpu->cpsr & ~mask);

        gba_cpu_switch_mode_bank(cpu,new_cpsr & gba_cpu_mode_flag);

        cpu->cpsr = new_cpsr;
    }

    cpu->r[15] += 4;
}

void gba_cpu_arm_mrs(gba_cpu_t* cpu,uint32_t opcode){
    bool is_spsr = opcode & (1 << 22);

    // R15 cannot be used as the destination register.
    uint8_t rd = (opcode >> 12) & 15;

    // TODO: What will be written to Rd if the mode is User/System?

    if(is_spsr){
        uint8_t mode = cpu->cpsr & gba_cpu_mode_flag;

        cpu->r[rd] = cpu->spsr[gba_cpu_mode_bank[mode]];
    }
    else{
        cpu->r[rd] = cpu->cpsr;
    }

    if(rd == 15){
        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 4;
    }
}


void gba_cpu_arm_multiply(gba_cpu_t* cpu,uint32_t opcode){
    bool accumulate = opcode & (1 << 21);
    bool set_flags = opcode & (1 << 20);

    // Rd may not be same as Rm
    // Rd,Rn,Rs,Rm may not be R15
    uint8_t rd = (opcode >> 16) & 15;
    uint8_t rn = (opcode >> 12) & 15;
    uint8_t rs = (opcode >> 8) & 15;
    uint8_t rm = opcode & 15;

    uint32_t result = cpu->r[rm] * cpu->r[rs];
    
    if(accumulate){
        result += cpu->r[rn];
        // TODO: I-cycle
    }

    uint32_t v = cpu->r[rs];
    uint32_t mask = 0xFFFFFF00;
    while(true){
        // TODO: I-cycle
        v &= mask;
        if(v == 0 || v == mask) break;
        mask <<= 8;
    }

    if(set_flags){
        gba_cpu_set_flag(gba_cpu_zero_flag,!result);
        gba_cpu_set_flag(gba_cpu_negative_flag,result & (1 << 31));
        // TODO: The C flag is set to a meaningless value, but how is that value calculated?
    }

    cpu->r[rd] = result;

    if(rd == 15){
        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 4;
    }
}

void gba_cpu_arm_multiply_long(gba_cpu_t* cpu,uint32_t opcode){
    bool _signed = opcode & (1 << 22);
    bool accumulate = opcode & (1 << 21);
    bool set_flags = opcode & (1 << 20);

    // R15 must not be used as an operand or as a destination register
    // RdHi, RdLo, and Rm must all specify different registers
    uint8_t rdhi = (opcode >> 16) & 15;
    uint8_t rdlo = (opcode >> 12) & 15;
    uint8_t rs = (opcode >> 8) & 15;
    uint8_t rm = opcode & 15;

    uint64_t result = 0;
    
    if(_signed){
        result = (int64_t)(int32_t)cpu->r[rm] * (int64_t)(int32_t)cpu->r[rs];
    }
    else{
        result = (uint64_t)cpu->r[rm] * (uint64_t)cpu->r[rs];
    }

    // TODO: I-cycle

    if(accumulate){
        result += ((uint64_t)cpu->r[rdhi] << 32) | cpu->r[rdlo];
        // TODO: I-cycle
    }

    uint32_t v = cpu->r[rs];
    uint32_t mask = 0xFFFFFF00;
    while(true){
        // TODO: I-cycle
        v &= mask;
        if(v == 0 || (_signed && v == mask)) break;
        mask <<= 8;
    }

    if(set_flags){
        gba_cpu_set_flag(gba_cpu_zero_flag,!result);
        gba_cpu_set_flag(gba_cpu_negative_flag,result & ((uint64_t)1 << 63));
        // TODO: The C and V flags are defined with meaningless values, but how are these values ​​calculated?
    }

    cpu->r[rdlo] = result;
    cpu->r[rdhi] = result >> 32;

    if(rdlo == 15 || rdhi == 15){
        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 4;
    }
}


void gba_cpu_arm_single_data_swap_byte(gba_cpu_t* cpu,uint32_t opcode){
    // Do not use R15 as an operand (Rn, Rd or Rm) in a SWP instruction
    uint8_t rn = (opcode >> 16) & 15;
    uint8_t rd = (opcode >> 12) & 15;
    uint8_t rm = opcode & 15;

    // TODO: What is the value of Rn or Rm if R15 is used? Instruction +8 or +12?

    uint32_t address = cpu->r[rn];

    uint32_t value = gba_cpu_read_byte(cpu,address);

    gba_cpu_write_byte(cpu,cpu->r[rm],address);

    // TODO: I-cycle

    cpu->r[rd] = value;

    if(rd == 15){
        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 4;
    }
}

void gba_cpu_arm_single_data_swap_word(gba_cpu_t* cpu,uint32_t opcode){
    // Do not use R15 as an operand (Rn, Rd or Rm) in a SWP instruction
    uint8_t rn = (opcode >> 16) & 15;
    uint8_t rd = (opcode >> 12) & 15;
    uint8_t rm = opcode & 15;

    // TODO: What is the value of Rn or Rm if R15 is used? Instruction +8 or +12?

    uint32_t address = cpu->r[rn];

    uint32_t value = gba_cpu_read_word_rotate(cpu,address);

    //address & ~3
    gba_cpu_write_word(cpu,cpu->r[rm],address);

    // TODO: I-cycle

    cpu->r[rd] = value;

    if(rd == 15){
        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 4;
    }
}


void gba_cpu_arm_single_data_transfer(gba_cpu_t* cpu,uint32_t opcode){
    bool offset_register = opcode & (1 << 25);
    bool pre = opcode & (1 << 24);
    bool add_offset = opcode & (1 << 23);
    bool byte = opcode & (1 << 22);
    bool write_back = opcode & (1 << 21);
    bool load = opcode & (1 << 20);

    uint8_t rn = (opcode >> 16) & 15;
    uint8_t rd = (opcode >> 12) & 15;

    uint32_t offset = 0;

    if(offset_register){
        // R15 must not be specified as the register offset (Rm)
        uint8_t rm = opcode & 15;
        offset = cpu->r[rm];

        uint8_t shift_type = (opcode >> 5) & 3;

        uint8_t shift = (opcode >> 7) & 31;

        switch(shift_type){
            case gba_cpu_logical_shift_left:{
                offset <<= shift;
                break;
            }
            case gba_cpu_logical_shift_right:{
                if(shift){
                    offset >>= shift;
                }
                else{
                    offset = 0;
                }
                break;
            }
            case gba_cpu_arithmetic_shift_right:{
                if(shift){
                    offset = (int32_t)offset >> shift;
                }
                else{
                    offset = (int32_t)offset >> 31;
                }
                break;
            }
            case gba_cpu_rotate_shift_right:{
                if(shift){
                    offset = (offset >> shift) | (offset << (32 - shift));
                }
                else{
                    bool carry_in = cpu->cpsr & gba_cpu_carry_flag;
                    offset = (offset >> 1) | (carry_in << 31);
                }
                break;
            }
        }
    }
    else{
        offset = opcode & 0x00000FFF;
    }

    // When using R15 as the base register you must remember it contains an address 8 bytes on
    // from the address of the current instruction
    uint32_t address = cpu->r[rn];

    if(!add_offset) offset = -offset;

    if(pre) address += offset;

    // LDR
    if(load){
        uint32_t value = 0;

        if(byte){
            value = gba_cpu_read_byte(cpu,address);
        }
        else{
            value = gba_cpu_read_word_rotate(cpu,address);
        }

        // TODO: I-Cycle

        if(write_back || !pre){
            cpu->r[rn] += offset;
        }

        cpu->r[rd] = value;
    }
    // STR
    else{
        // When R15 is the source register (Rd) of a register store (STR) instruction, the stored
        // value will be address of the instruction plus 12.
        uint32_t value = cpu->r[rd] + ((rd == 15) ? 4 : 0);

        if(byte){
            gba_cpu_write_byte(cpu,value,address);
        }
        else{
            //address & ~3
            gba_cpu_write_word(cpu,value,address);
        }

        if(write_back || !pre){
            cpu->r[rn] += offset;
        }
    }

    // TODO: What happens if Rn is R15 with write-back or post-indexing specified?

    if((rd == 15 && load) || (rn == 15 && (write_back || !pre))){
        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 4;
    }
}

void gba_cpu_arm_block_data_transfer(gba_cpu_t* cpu,uint32_t opcode){
    bool pre = opcode & (1 << 24);
    bool add_offset = opcode & (1 << 23);
    bool s = opcode & (1 << 22);
    bool write_back = opcode & (1 << 21);
    bool load = opcode & (1 << 20);

    // R15 should not be used as the base register in any LDM or STM instruction
    uint8_t rn = (opcode >> 16) & 15;

    uint16_t list = opcode & 0xFFFF;

    uint32_t bytes = 0;

    if(list){
        for(int i = 0; i < 16; ++i){
            if(list & (1 << i)){
                bytes += 4;
            }
        }
    }
    else{
        // Empty list: R15 loaded/stored, and Rn=Rn+/-64
        list = 1 << 15;
        bytes = 64;
    }

    bool r15_in_list = list & (1 << 15);

    uint8_t register_mode = cpu->cpsr & gba_cpu_mode_flag;

    if(s && (!r15_in_list || !load)){
        register_mode = gba_cpu_user_mode;
    }

    uint32_t address = cpu->r[rn];

    uint32_t write_back_address = address + (add_offset ? bytes : -bytes);

    if(add_offset){
        if(pre) address += 4;
    }
    else{
        address -= bytes;
        if(!pre) address += 4;
    }

    // Whenever R15 is stored to memory the stored value is the address of the STM
    // instruction plus 12.
    cpu->r[15] += 4;
    
    if(load){
        //A LDM will always overwrite the updated base if the base is in the list.
        if(write_back){
            cpu->r[rn] = write_back_address;
        }

        for(int i = 0; i < 16; ++i){
            
            if(!(list & (1 << i))) continue;

            uint32_t* dst = gba_cpu_get_register(cpu,i,register_mode);

            //address & ~3
            *dst = gba_cpu_read_word(cpu,address);
            
            address += 4;
        }

        // TODO: I-cycle

        if(s && r15_in_list){

            uint8_t mode = cpu->cpsr & gba_cpu_mode_flag;

            if(mode != gba_cpu_user_mode && mode != gba_cpu_system_mode){
                
                uint32_t new_cpsr = cpu->spsr[gba_cpu_mode_bank[mode]];
                
                // TODO: What happens when writing an invalid mode to CPSR?

                gba_cpu_switch_mode_bank(cpu,new_cpsr & gba_cpu_mode_flag);

                cpu->cpsr = new_cpsr;
            }
        }
    }
    else{
        bool first = true;

        for(int i = 0; i < 16; ++i){
            
            if(!(list & (1 << i))) continue;

            uint32_t* src = gba_cpu_get_register(cpu,i,register_mode);

            // address & ~3
            gba_cpu_write_word(cpu,*src,address);

            address += 4;

            // When write-back is specified, the base is written back at the end
            // of the second cycle of the instruction.
            if(first && write_back){
                cpu->r[rn] = write_back_address;
            }

            first = false;
        }
    }

    if((r15_in_list && load) || (rn == 15 && write_back)){
        gba_cpu_reload_pipeline(cpu);
    }
}

void gba_cpu_arm_halfword_data_transfer(gba_cpu_t* cpu,uint32_t opcode){
    bool pre = opcode & (1 << 24);
    bool add_address = opcode & (1 << 23);
    bool immediate_offset = opcode & (1 << 22);
    bool write_back = opcode & (1 << 21);
    bool load = opcode & (1 << 20);
    uint8_t rn = (opcode >> 16) & 15;
    uint8_t rd = (opcode >> 12) & 15;
    uint8_t operation = (opcode >> 5) & 3;

    uint32_t offset = 0;

    if(immediate_offset){
        offset = ((opcode >> 4) & 240) | (opcode & 15);
    }
    else{
        // R15 should not be specified as the register offset (Rm)
        uint8_t rm = opcode & 15;
        offset = cpu->r[rm];
    }

    // When using R15 as the base register you must remember it contains an address 8 bytes on
    // from the address of the current instruction
    uint32_t address = cpu->r[rn];

    if(!add_address) offset = -offset;

    if(pre) address += offset;

    if(load){
        uint32_t value = 0;

        switch(operation){
            //LDRH
            case 1: value = gba_cpu_read_halfword_rotate(cpu,address); break;
            //LDRSB
            case 2: value = gba_cpu_read_signed_byte(cpu,address); break;
            //LDRSH
            case 3: value = gba_cpu_read_signed_halfword(cpu,address); break;
        }

        // TODO: I-cycle
        
        if(write_back || !pre){
            cpu->r[rn] += offset;
        }

        cpu->r[rd] = value;
    }
    else{
        // When R15 is the source register (Rd) of a Half-word store (STRH) instruction, the
        // stored address will be address of the instruction plus 12.
        uint32_t value = cpu->r[rd] + ((rd == 15) ? 4 : 0);

        switch(operation){
            //STRH
            case 1: gba_cpu_write_halfword(cpu,value,address); break;
            //TODO: Whats?
            case 2: break;
            //TODO: Whats?
            case 3: break;
        }

        if(write_back || !pre){
            cpu->r[rn] += offset;
        }
    }

    if((rd == 15 && load) || (rn == 15 && (write_back || !pre))){
        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 4;
    }
}


void gba_cpu_arm_branch_exchange(gba_cpu_t* cpu,uint32_t opcode){
    uint32_t rn = cpu->r[opcode & 15];

    cpu->r[15] = rn;

    if(rn & 1){
        cpu->cpsr |= gba_cpu_thumb_flag;
    }

    gba_cpu_reload_pipeline(cpu);
}

void gba_cpu_arm_branch_and_branch_link(gba_cpu_t* cpu,uint32_t opcode){
    bool link = opcode & (1 << 24);

    uint32_t offset = opcode & 0x00FFFFFF;

    if(offset & 0x00800000){
        offset |= 0xFF000000;
    }

    offset <<= 2;

    if(link){
        cpu->r[14] = cpu->r[15] - 4;
    }

    cpu->r[15] += offset;

    gba_cpu_reload_pipeline(cpu);
}


void gba_cpu_arm_undefined(gba_cpu_t* cpu,uint32_t opcode){
    gba_unused(opcode);

    //TODO: I-cycle

    cpu->spsr[gba_cpu_undefined_bank] = cpu->cpsr;

    gba_cpu_switch_mode_bank(cpu,gba_cpu_undefined_mode);

    cpu->r[14] = cpu->r[15] - 4;

    cpu->cpsr &= ~gba_cpu_mode_flag;
    
    cpu->cpsr |= gba_cpu_undefined_mode | gba_cpu_irq_flag;

    cpu->r[15] = gba_cpu_undefined_vector;

    gba_cpu_reload_pipeline(cpu);
}


void gba_cpu_arm_swi(gba_cpu_t* cpu,uint32_t opcode){
    gba_unused(opcode);

    cpu->spsr[gba_cpu_supervisor_bank] = cpu->cpsr;

    gba_cpu_switch_mode_bank(cpu,gba_cpu_supervisor_mode);

    cpu->r[14] = cpu->r[15] - 4;

    cpu->cpsr &= ~gba_cpu_mode_flag;

    cpu->cpsr |= gba_cpu_supervisor_mode | gba_cpu_irq_flag;

    cpu->r[15] = gba_cpu_software_interrupt_vector;

    gba_cpu_reload_pipeline(cpu);
}