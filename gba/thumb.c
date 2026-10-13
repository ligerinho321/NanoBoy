#include "thumb.h"
#include "cpu.h"

gba_cpu_thumb_instruction_callback_t thumb_table[512] = {NULL};

void gba_cpu_thumb_init_table(){

    for(int i = 0; i < 512; ++i){
        thumb_table[i] = gba_cpu_thumb_undefined;
    }

    // Move Shifted Register
    // 000x_xxxx_x--_----
    for(int i = 0; i <= 0x3F; ++i){
        thumb_table[i] = gba_cpu_thumb_move_shifted_register;
    }

    // Add/Subtract
    // 0001_1xxx_x--_----
    for(int i = 0; i <= 0x0F; ++i){
        thumb_table[0x30 | i] = gba_cpu_thumb_add_subtract;
    }

    // Move/Compare/Add/Subtract immediate
    // 001x_xxxx_x--_----
    for(int i = 0; i <= 0x3F; ++i){
        thumb_table[0x40 | i] = gba_cpu_thumb_mov_cmp_add_sub_immediate;
    }

    // ALU operations
    // 0100_00xx_x--_----
    for(int i = 0; i <= 7; ++i){
        thumb_table[0x80 | i] = gba_cpu_thumb_alu_operations;
    }

    // Hi Register Operations/Branch Exchange
    // 0100_01xx_x--_----
    for(int i = 0; i <= 7; ++i){
        thumb_table[0x88 | i] = gba_cpu_thumb_hi_register_and_branch_exchange;
    }


    // PC-Relative Load
    // 0100_1xxx_x--_----
    for(int i = 0; i <= 0x0F; ++i){
        thumb_table[0x90 | i] = gba_cpu_thumb_pc_relative_load;
    }

    // Load/Store With Register Offset
    // 0101_xx0x_x--_----
    for(int i = 0; i <= 0x0F; ++i){
        thumb_table[0xA0 | ((i & 0x0C) << 0x01) | (i & 0x03)] = gba_cpu_thumb_load_store_with_register_offset;
    }

    // Load/Store Sign-Extended Byte/Halfword
    // 0101_xx1x_x--_----
    for(int i = 0; i <= 0x0F; ++i){
        thumb_table[0xA0 | ((i & 0x0C) << 0x01) | 0x04 | (i & 3)] = gba_cpu_thumb_load_store_sign_extended_byte_halfword;
    }

    // Load/Store With Immediate Offset
    // 011x_xxxx_x--_----
    for(int i = 0; i <= 0x3F; ++i){
        thumb_table[0xC0 | i] = gba_cpu_thumb_load_store_with_immediate_offset;
    }

    // Load/Store Halfword
    // 1000_xxxx_x--_----
    for(int i = 0; i <= 0x1F; ++i){
        thumb_table[0x100 | i] = gba_cpu_thumb_load_store_halfword;
    }

    // SP-Relative Load/Store
    // 1001_xxxx_x--_----
    for(int i = 0; i <= 0x1F; ++i){
        thumb_table[0x120 | i] = gba_cpu_thumb_sp_relative_load_store;
    }


    // Load Address
    // 1010_xxxx_x--_----
    for(int i = 0; i <= 0x1F; ++i){
        thumb_table[0x140 | i] = gba_cpu_thumb_load_address;
    }

    // Add Offset To Stack Pointer
    // 1011_0000_x--_----
    thumb_table[0x160] = gba_cpu_thumb_add_offset_to_stack_pointer;
    thumb_table[0x161] = gba_cpu_thumb_add_offset_to_stack_pointer;


    // Push/Pop Registers
    // 1011_x10x_x--_----
    for(int i = 0; i <= 7; ++i){
        thumb_table[0x160 | ((i & 4) << 2) | 0x08 | (i & 3)] = gba_cpu_thumb_push_pop_registers;
    }

    // Multiple Load/Store
    // 1100_xxxx_x--_----
    for(int i = 0; i <= 0x1F; ++i){
        thumb_table[0x180 | i] = gba_cpu_thumb_multiple_load_store;
    }


    // Conditional Branch
    // 1101_xxxx_x--_----
    for(int i = 0; i <= 0x1F; ++i){
        thumb_table[0x1A0 | i] = gba_cpu_thumb_conditional_branch;
    }

    // Undefined Conditional Branch Opcode E
    // 1101_1110_x--_----
    thumb_table[0x1BC] = gba_cpu_thumb_undefined;
    thumb_table[0x1BD] = gba_cpu_thumb_undefined;

    // SWI
    // 1101_1111_x--_----
    thumb_table[0x1BE] = gba_cpu_thumb_swi;
    thumb_table[0x1BF] = gba_cpu_thumb_swi;

    // Unconditional Branch
    // 1110_0xxx_x--_----
    for(int i = 0; i <= 0x0F; ++i){
        thumb_table[0x1C0 | i] = gba_cpu_thumb_unconditional_branch;
    }
    
    // Long Branch With Link
    // 1111_xxxx_x--_----
    for(int i = 0; i <= 0x1F; ++i){
        thumb_table[0x1E0 | i] = gba_cpu_thumb_long_branch_with_link;
    }
}


void gba_cpu_thumb_move_shifted_register(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 11) & 3;
    uint8_t shift = (opcode >> 6) & 31;
    uint8_t rs = (opcode >> 3) & 7;
    uint8_t rd = opcode & 7;

    bool carry = cpu->cpsr & gba_cpu_carry_flag;

    uint32_t result = 0;

    switch(operation){
        //LSL Rd, Rs, #Offset5 
        case gba_cpu_logical_shift_left:
            result = gba_cpu_lsl(cpu->r[rs],shift,&carry);
            break;
        //LSR Rd, Rs, #Offset5
        case gba_cpu_logical_shift_right:
            result = gba_cpu_lsr(cpu->r[rs],!shift ? 32 : shift,&carry);
            break;
        //ASR Rd, Rs, #Offset5
        case gba_cpu_arithmetic_shift_right:
            result = gba_cpu_asr(cpu->r[rs],!shift ? 32 : shift,&carry);
            break;
    }

    gba_cpu_set_flag(gba_cpu_zero_flag,!result);
    gba_cpu_set_flag(gba_cpu_negative_flag,result & (1 << 31));
    gba_cpu_set_flag(gba_cpu_carry_flag,carry);

    cpu->r[rd] = result;

    cpu->r[15] += 2;
}

void gba_cpu_thumb_add_subtract(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 9) & 3;
    uint8_t rn = (opcode >> 6) & 7;
    uint8_t rs = (opcode >> 3) & 7;
    uint8_t rd = opcode & 7;

    switch(operation){
        //ADD Rd, Rs, Rn
        case 0:
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rs],cpu->r[rn],0,true);
            break;
        //SUB Rd, Rs, Rn
        case 1:
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rs],~cpu->r[rn],1,true);
            break;
        //ADD Rd, Rs, #Offset3 
        case 2:
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rs],rn,0,true);
            break;
        //SUB Rd, Rs, #Offset3
        case 3:
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rs],~rn,1,true);
            break;
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_mov_cmp_add_sub_immediate(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 11) & 3;
    uint8_t rd = (opcode >> 8) & 7;
    uint8_t offset = opcode & 255;

    switch(operation){
        //MOV Rd, #Offset8
        case 0:
            cpu->r[rd] = offset;
            gba_cpu_set_nz_flag(offset);
            break;
        //CMP Rd, #Offset8
        case 1:
            gba_cpu_alu_arithmetic(cpu,cpu->r[rd],~offset,1,true);
            break;
        //ADD Rd, #Offset8
        case 2:
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rd],offset,0,true);
        //SUB Rd, #Offset8
        case 3:
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rd],~offset,1,true);
            break;
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_alu_operations(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 6) & 15;
    uint8_t rs = (opcode >> 3) & 7;
    uint8_t rd = opcode & 7;

    switch(operation){
        case gba_cpu_and_operation:{
            uint32_t result = cpu->r[rd] & cpu->r[rs];
            
            gba_cpu_set_nz_flag(result);

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_eor_operation:{
            uint32_t result = cpu->r[rd] ^ cpu->r[rs];
            
            gba_cpu_set_nz_flag(result);

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_lsl_operation:{
            bool carry = cpu->cpsr & gba_cpu_carry_flag;
            
            uint32_t result = gba_cpu_lsl(cpu->r[rd],cpu->r[rs] & 255,&carry);
            
            gba_cpu_set_logical_flag(result,carry);

            // TODO: I-cycle

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_lsr_operation:{
            bool carry = cpu->cpsr & gba_cpu_carry_flag;
            
            uint32_t result = gba_cpu_lsr(cpu->r[rd],cpu->r[rs] & 255,&carry);
            
            gba_cpu_set_logical_flag(result,carry);

            // TODO: I-cycle

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_asr_operation:{
            bool carry = cpu->cpsr & gba_cpu_carry_flag;
            
            uint32_t result = gba_cpu_asr(cpu->r[rd],cpu->r[rs] & 255,&carry);
            
            gba_cpu_set_logical_flag(result,carry);

            // TODO: I-cycle

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_adc_operation:{
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rd],cpu->r[rs],cpu->cpsr & gba_cpu_carry_flag,true);
            break;
        }
        case gba_cpu_sbc_operation:{
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rd],~cpu->r[rs],cpu->cpsr & gba_cpu_carry_flag,true);
            break;
        }
        case gba_cpu_ror_operation:{
            bool carry = cpu->cpsr & gba_cpu_carry_flag;
            
            uint32_t result = gba_cpu_ror(cpu->r[rd],cpu->r[rs] & 255,&carry);
            
            gba_cpu_set_logical_flag(result,carry);
            
            // TODO: I-cycle

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_tst_operation:{
            uint32_t result = cpu->r[rd] & cpu->r[rs];
            
            gba_cpu_set_nz_flag(result);
            break;
        }
        case gba_cpu_neg_operation:{
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,0,~cpu->r[rs],1,true);
            break;
        }
        case gba_cpu_cmp_operation:{
            gba_cpu_alu_arithmetic(cpu,cpu->r[rd],~cpu->r[rs],1,true);
            break;
        }
        case gba_cpu_cmn_operation:{
            gba_cpu_alu_arithmetic(cpu,cpu->r[rd],cpu->r[rs],0,true);
            break;
        }
        case gba_cpu_orr_operation:{
            uint32_t result = cpu->r[rd] | cpu->r[rs];
            
            gba_cpu_set_nz_flag(result);

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_mul_operation:{
            uint32_t result = cpu->r[rd] * cpu->r[rs];
            
            gba_cpu_multiply_clock(cpu->r[rd],true);

            gba_cpu_set_nz_flag(result);
            // TODO: The C flag is set to a meaningless value, but how is that value calculated?

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_bic_operation:{
            uint32_t result = cpu->r[rd] & ~cpu->r[rs];

            gba_cpu_set_nz_flag(result);

            cpu->r[rd] = result;
            break;
        }
        case gba_cpu_mvn_operation:{
            uint32_t result = ~cpu->r[rs];

            gba_cpu_set_nz_flag(result);

            cpu->r[rd] = result;
            break;
        }
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_hi_register_and_branch_exchange(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 8) & 3;

    uint8_t rs = ((opcode & (1 << 6)) >> 3) | ((opcode >> 3) & 7);
    uint8_t rd = ((opcode & (1 << 7)) >> 4) | ((opcode >> 0) & 7);

    switch(operation){
        // ADD Rd, Rs
        case 0:{
            cpu->r[rd] = gba_cpu_alu_arithmetic(cpu,cpu->r[rd],cpu->r[rs],0,true);

            if(rd == 15){
                gba_cpu_reload_pipeline(cpu);
            }
            else{
                cpu->r[15] += 2;
            }
            break;
        }
        // CMP Rd, Rs
        case 1:{
            gba_cpu_alu_arithmetic(cpu,cpu->r[rd],~cpu->r[rs],1,true);

            cpu->r[15] += 2;
            break;
        }
        // MOV Rd, Rs
        case 2:{
            cpu->r[rd] = cpu->r[rs];

            gba_cpu_set_nz_flag(cpu->r[rd]);

            if(rd == 15){
                gba_cpu_reload_pipeline(cpu);
            }
            else{
                cpu->r[15] += 2;
            }
            break;
        }
        // BX Rs
        case 3:{
            cpu->r[15] = cpu->r[rs];

            if(!(cpu->r[rs] & 1)){
                cpu->cpsr &= ~gba_cpu_thumb_flag;    
            }

            gba_cpu_reload_pipeline(cpu);
            break;
        }
    }
}


void gba_cpu_thumb_pc_relative_load(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t rd = (opcode >> 8) & 7;
    uint32_t offset = (opcode & 0xFF) << 2;

    // The value of the PC will be 4 bytes greater than the address of this instruction, but bit
    // 1 of the PC is forced to 0 to ensure it is word aligned.
    uint32_t address = (cpu->r[15] & ~2) + offset;

    cpu->r[rd] = gba_cpu_read_word(cpu,address);

    // TODO: I-cycle

    cpu->r[15] += 2;
}

void gba_cpu_thumb_load_store_with_register_offset(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 10) & 3;
    uint8_t ro = (opcode >> 6) & 7;
    uint8_t rb = (opcode >> 3) & 7;
    uint8_t rd = opcode & 7;

    uint32_t address = cpu->r[rb] + cpu->r[ro];

    switch(operation){
        // STR Rd, [Rb, Ro]
        case 0:
            gba_cpu_write_word(cpu,cpu->r[rd],address);
            break;
        // STRB Rd, [Rb, Ro]
        case 1:
            gba_cpu_write_byte(cpu,cpu->r[rd],address);
            break;
        // LDR Rd, [Rb, Ro]
        case 2:
            cpu->r[rd] = gba_cpu_read_word_rotate(cpu,address);

            // TODO: I-cycle
            break;
        // LDRB Rd, [Rb, Ro]
        case 3:
            cpu->r[rd] = gba_cpu_read_byte(cpu,address);

            // TODO: I-cycle
            break;
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_load_store_sign_extended_byte_halfword(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 10) & 3;
    uint8_t ro = (opcode >> 6) & 7;
    uint8_t rb = (opcode >> 3) & 7;
    uint8_t rd = opcode & 7;

    uint32_t address = cpu->r[rb] + cpu->r[ro];

    switch(operation){
        // STRH Rd, [Rb, Ro]
        case 0:
            gba_cpu_write_halfword(cpu,cpu->r[rd],address);
            break;
        // LDSB Rd, [Rb, Ro]
        case 1:
            cpu->r[rd] = gba_cpu_read_signed_byte(cpu,address);
            // TODO: I-cycle
            break;
        // LDRH Rd, [Rb, Ro]
        case 2:
            cpu->r[rd] = gba_cpu_read_halfword_rotate(cpu,address);
            // TODO: I-cycle
            break;
        // LDSH Rd, [Rb, Ro]
        case 3:
            cpu->r[rd] = gba_cpu_read_signed_halfword(cpu,address);
            // TODO: I-cycle
            break;
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_load_store_with_immediate_offset(gba_cpu_t* cpu,uint16_t opcode){
    uint8_t operation = (opcode >> 11) & 3;
    uint8_t offset = (opcode >> 6) & 31;
    uint8_t rb = (opcode >> 3) & 7;
    uint8_t rd = opcode & 7;

    switch(operation){
        // STR Rd, [Rb, #Imm]
        case 0:
            gba_cpu_write_word(cpu,cpu->r[rd],cpu->r[rb] + (offset << 2));
            break;
        // LDR Rd, [Rb, #Imm]
        case 1:
            cpu->r[rd] = gba_cpu_read_word_rotate(cpu,cpu->r[rb] + (offset << 2));

            // TODO: I-cycle
            break;
        // STRB Rd, [Rb, #Imm]
        case 2:
            gba_cpu_write_byte(cpu,cpu->r[rd],cpu->r[rb] + offset);
            break;
        // LDRB Rd, [Rb, #Imm]
        case 3:
            cpu->r[rd] = gba_cpu_read_byte(cpu,cpu->r[rb] + offset);

            // TODO: I-cycle
            break;
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_load_store_halfword(gba_cpu_t* cpu,uint16_t opcode){
    bool load = opcode & (1 << 11);
    uint8_t offset = ((opcode >> 5) & 0x3E);
    uint8_t rb = (opcode >> 3) & 7;
    uint8_t rd = opcode & 7;

    uint32_t address = cpu->r[rb] + offset;

    // LDRH Rd, [Rb, #Imm]
    if(load){
        cpu->r[rd] = gba_cpu_read_halfword_rotate(cpu,address);

        // TODO: I-cycle
    }
    // STRH Rd, [Rb, #Imm] 
    else{
        gba_cpu_write_halfword(cpu,cpu->r[rd],address);
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_sp_relative_load_store(gba_cpu_t* cpu,uint16_t opcode){
    bool load = opcode & (1 << 11);
    uint8_t rd = (opcode >> 8) & 7;
    uint32_t offset = (opcode & 0xFF) << 2;

    uint32_t address = cpu->r[13] + offset;

    // LDR Rd, [SP, #Imm]
    if(load){
        cpu->r[rd] = gba_cpu_read_word_rotate(cpu,address);
        
        // TODO: I-cycle
    }
    // STR Rd, [SP, #Imm]
    else{
        gba_cpu_write_word(cpu,cpu->r[rd],address);
    }

    cpu->r[15] += 2;
}


void gba_cpu_thumb_load_address(gba_cpu_t* cpu,uint16_t opcode){
    bool source = opcode & (1 << 11);
    uint8_t rd = (opcode >> 8) & 7;
    uint32_t offset = (opcode & 0xFF) << 2;

    // ADD SP, #Imm
    if(source){
        cpu->r[rd] = cpu->r[13] + offset;
    }
    // ADD PC, #Imm
    else{
        // The value of the PC will be 4 bytes greater than the address of the instruction
        // before bit 1 is forced to 0.
        cpu->r[rd] = (cpu->r[15] & ~2) + offset;
    }

    cpu->r[15] += 2;
}

void gba_cpu_thumb_add_offset_to_stack_pointer(gba_cpu_t* cpu,uint16_t opcode){
    bool subtract = opcode & (1 << 7);
    uint32_t offset = (opcode & 0x7F) << 2;

    // ADD SP, #-Imm
    if(subtract){
        cpu->r[13] -= offset;
    }
    // ADD SP, #Imm
    else{
        cpu->r[13] += offset;
    }

    cpu->r[15] += 2;
}


void gba_cpu_thumb_push_pop_registers(gba_cpu_t* cpu,uint16_t opcode){
    bool load = opcode & (1 << 11);
    bool r = opcode & (1 << 8);
    uint8_t list = opcode & 0xFF;

    // Whenever R15 is stored to memory the stored value is the address of the STM
    // instruction plus 6.
    cpu->r[15] += 2;

    // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+40h (ARMv4-v5).
    if(!list && !r){
        if(load){
            cpu->r[15] = gba_cpu_read_word(cpu,cpu->r[13]);

            cpu->r[13] += 64;

            // TODO: I-cycle

            gba_cpu_reload_pipeline(cpu);
        }
        else{
            cpu->r[13] -= 64;
            
            gba_cpu_write_word(cpu,cpu->r[15],cpu->r[13]);
        }

        return;
    }

    uint8_t bytes = (__builtin_popcount(list) + r) * 4;

    // POP
    if(load){
        uint32_t address = cpu->r[13];

        cpu->r[13] += bytes;

        for(int i = 0; i < 8; ++i){

            if(!(list & (1 << i))) continue;

            cpu->r[i] = gba_cpu_read_word(cpu,address);

            address += 4;
        }

        // TODO: I-cycle

        if(r){
            cpu->r[15] = gba_cpu_read_word(cpu,address);

            gba_cpu_reload_pipeline(cpu);
        }
    }
    // PUSH
    else{
        cpu->r[13] -= bytes;

        uint32_t address = cpu->r[13];

        for(int i = 0; i < 8; ++i){

            if(!(list & (1 << i))) continue;

            gba_cpu_write_word(cpu,cpu->r[i],address);

            address += 4;
        }

        if(r){
            gba_cpu_write_word(cpu,cpu->r[14],address);
        }
    }
}

void gba_cpu_thumb_multiple_load_store(gba_cpu_t* cpu,uint16_t opcode){
    bool load = opcode & (1 << 11);
    uint8_t rb = (opcode >> 8) & 7;
    uint8_t list = opcode & 0xFF;

    // Whenever R15 is stored to memory the stored value is the address of the STM
    // instruction plus 6.
    cpu->r[15] += 2;

    // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+40h (ARMv4-v5).
    if(!list){
        if(load){
            cpu->r[15] = gba_cpu_read_word(cpu,cpu->r[rb]);

            // TODO: I-cycle

            gba_cpu_reload_pipeline(cpu);
        }
        else{
            gba_cpu_write_word(cpu,cpu->r[15],cpu->r[rb]);
        }

        cpu->r[rb] += 64;

        return;
    }

    uint32_t address = cpu->r[rb];

    uint32_t bytes = __builtin_popcount(list) * 4;

    if(load){
        // A LDM will always overwrite the updated base if the base is in the list.
        cpu->r[rb] += bytes;

        for(int i = 0; i < 8; ++i){

            if(!(list & (1 << i))) continue;

            cpu->r[i] = gba_cpu_read_word(cpu,address);

            address += 4;
        }

        // TODO: I-cycle
    }
    else{
        bool first = true;

        for(int i = 0; i < 8; ++i){

            if(!(list & (1 << i))) continue;

            gba_cpu_write_word(cpu,cpu->r[i],address);

            address += 4;

            // The base is written back at the end of the second cycle of the instruction
            if(first){
                cpu->r[rb] += bytes;
                first = false;
            }
        }
    }
}


void gba_cpu_thumb_conditional_branch(gba_cpu_t* cpu,uint16_t opcode){

    if(gba_cpu_condition_table[((opcode >> 4) & 0xF0) | (cpu->cpsr >> 28)]){
        uint32_t offset = opcode & 0xFF;

        if(offset & 0x00000080){
            offset |= 0xFFFFFF00;
        }

        offset <<= 1;

        cpu->r[15] += offset;

        gba_cpu_reload_pipeline(cpu);
    }
    else{
        cpu->r[15] += 2;
    }
}

void gba_cpu_thumb_swi(gba_cpu_t* cpu,uint16_t opcode){
    gba_unused(opcode);

    cpu->spsr[gba_cpu_supervisor_bank] = cpu->cpsr;

    gba_cpu_switch_mode_bank(cpu,gba_cpu_supervisor_mode);

    cpu->r[14] = cpu->r[15] - 2;

    cpu->cpsr &= ~(gba_cpu_thumb_flag | gba_cpu_mode_flag);
    cpu->cpsr |= gba_cpu_supervisor_mode | gba_cpu_irq_flag;

    cpu->r[15] = gba_cpu_software_interrupt_vector;

    gba_cpu_reload_pipeline(cpu);
}

void gba_cpu_thumb_unconditional_branch(gba_cpu_t* cpu,uint16_t opcode){
    uint32_t offset = opcode & 0x07FF;

    if(offset & 0x0400){
        offset |= 0xFFFFF800;
    }

    offset <<= 1;

    cpu->r[15] += offset;

    gba_cpu_reload_pipeline(cpu);
}

void gba_cpu_thumb_long_branch_with_link(gba_cpu_t* cpu,uint16_t opcode){
    bool low_offset = opcode & (1 << 11);
    uint32_t offset = opcode & 0x07FF;
    
    if(low_offset){
        offset = cpu->r[14] + (offset << 1);
        
        cpu->r[14] = (cpu->r[15] - 2) | 1;

        cpu->r[15] = offset;

        gba_cpu_reload_pipeline(cpu);
    } 
    else{
        offset <<= 12;

        if(offset & 0x400000){
            offset |= 0xFF800000;
        }

        cpu->r[14] = cpu->r[15] + offset;

        cpu->r[15] += 2;
    }
}


void gba_cpu_thumb_undefined(gba_cpu_t* cpu,uint16_t opcode){
    gba_unused(opcode);

    //TODO: I-cycle

    cpu->spsr[gba_cpu_undefined_bank] = cpu->cpsr;

    gba_cpu_switch_mode_bank(cpu,gba_cpu_undefined_mode);

    cpu->r[14] = cpu->r[15] - 2;

    cpu->cpsr &= ~(gba_cpu_thumb_flag | gba_cpu_mode_flag);
    cpu->cpsr |= gba_cpu_undefined_mode | gba_cpu_irq_flag;

    cpu->r[15] = gba_cpu_undefined_vector;

    gba_cpu_reload_pipeline(cpu);
}