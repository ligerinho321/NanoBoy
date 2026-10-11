#include "cpu.h"
#include "arm.h"
#include "thumb.h"

static const bool gba_cpu_condition_table[256] = {
    0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
    1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,
    0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,
    1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,
    0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
    0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
    0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,
    0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,
    1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,
    0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,
    1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,0,
    0,1,0,1,1,1,1,1,1,0,1,0,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

const uint8_t gba_cpu_mode_bank[32] = {
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,1,2,3,6,6,6,4,6,6,6,5,6,6,6,0
};


void gba_cpu_init(gba_cpu_t* cpu,gba_t* gba){
    cpu->gba = gba;

    gba_cpu_arm_init_table();
}


void gba_cpu_write_byte(gba_cpu_t* cpu,uint8_t value,uint32_t address){

}

uint8_t gba_cpu_read_byte(gba_cpu_t* cpu,uint32_t address){
    return 0x00;
}

uint32_t gba_cpu_read_signed_byte(gba_cpu_t* cpu,uint32_t address){
    uint32_t value = gba_cpu_read_byte(cpu,address);
    if(value & 0x00000080){
        value |= 0xFFFFFF00;
    }
    return value;
}


void gba_cpu_write_halfword(gba_cpu_t* cpu,uint16_t value,uint32_t address){
    //address & ~1
}

uint16_t gba_cpu_read_halfword(gba_cpu_t* cpu,uint32_t address){
    //address & ~1
    return 0x0000;
}

uint32_t gba_cpu_read_halfword_rotate(gba_cpu_t* cpu,uint32_t address){
    uint32_t value = gba_cpu_read_halfword(cpu,address & ~1);
    if(address & 1){
        value = (value >> 8) | (value << 24);
    }
    return value;
}

uint32_t gba_cpu_read_signed_halfword(gba_cpu_t* cpu,uint32_t address){
    uint32_t value = 0;
    if(address & 1){
        value = gba_cpu_read_byte(cpu,address);
        if(value & 0x00000080) value |= 0xFFFFFF00;
    }
    else{
        value = gba_cpu_read_halfword(cpu,address);
        if(value & 0x00008000) value |= 0xFFFF0000;
    }
    return value;
}


void gba_cpu_write_word(gba_cpu_t* cpu,uint32_t value,uint32_t address){
    //address & ~3
}

uint32_t gba_cpu_read_word(gba_cpu_t* cpu,uint32_t address){
    //address & ~3
    return 0x00000000;
}

uint32_t gba_cpu_read_word_rotate(gba_cpu_t* cpu,uint32_t address){
    uint32_t value = gba_cpu_read_word(cpu,address);
    uint8_t shift = (address & 3) << 3;
    if(shift){
        value = (value >> shift) | (value << (32 - shift));
    }
    return value;
}


void gba_cpu_switch_mode_bank(gba_cpu_t* cpu,uint8_t new_mode){
    uint8_t old_mode = cpu->cpsr & gba_cpu_mode_flag;

    if(new_mode == old_mode) return;

    uint8_t old_mode_bank = gba_cpu_mode_bank[old_mode];
    uint8_t new_mode_bank = gba_cpu_mode_bank[new_mode];

    if(new_mode_bank == gba_cpu_invalid_bank) return;

    if(old_mode == gba_cpu_fiq_mode){
        for(int i = 0; i < 5; ++i){
            cpu->r8_r12[gba_cpu_fiq_bank][i] = cpu->r[8 + i];
            cpu->r[8 + 1] = cpu->r8_r12[gba_cpu_none_bank][i];
        }
    }
    else if(new_mode == gba_cpu_fiq_mode){
        for(int i = 0; i < 5; ++i){
            cpu->r8_r12[gba_cpu_none_bank][i] = cpu->r[8 + i];
            cpu->r[8 + 1] = cpu->r8_r12[gba_cpu_fiq_bank][i];
        }
    }

    cpu->r13[old_mode_bank] = cpu->r[13];
    cpu->r[13] = cpu->r13[new_mode_bank];

    cpu->r14[old_mode_bank] = cpu->r[14];
    cpu->r[14] = cpu->r14[new_mode_bank];
}

void gba_cpu_reload_pipeline(gba_cpu_t* cpu){
    if(cpu->cpsr & gba_cpu_thumb_flag){
        cpu->r[15] &= ~1;
        cpu->pipeline.decode = gba_cpu_read_halfword(cpu,cpu->r[15] + 0);
        cpu->pipeline.fetch = gba_cpu_read_halfword(cpu,cpu->r[15] + 2);
        cpu->r[15] += 4;
    }
    else{
        cpu->r[15] &= ~3;
        cpu->pipeline.decode = gba_cpu_read_word(cpu,cpu->r[15] + 0);
        cpu->pipeline.fetch = gba_cpu_read_word(cpu,cpu->r[15] + 4);
        cpu->r[15] += 8;
    }
}

uint32_t* gba_cpu_get_register(gba_cpu_t* cpu,uint8_t index,uint8_t mode){
    uint32_t* ptr = NULL;

    switch(index){
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:{
            ptr = &cpu->r[index];
            break;
        }
        case 8: case 9: case 10: case 11: case 12:{
            if(mode != gba_cpu_fiq_mode){
                ptr = &cpu->r8_r12[0][index - 8];
            }
            else{
                ptr = &cpu->r8_r12[1][index - 8];
            }
            break;
        }
        case 13:{
            ptr = &cpu->r13[gba_cpu_mode_bank[index]];
            break;
        }
        case 14:{
            ptr = &cpu->r14[gba_cpu_mode_bank[index]];
            break;
        }
        case 15:{
            ptr = &cpu->r[index];
            break;
        }
    }

    return ptr;
}

bool gba_cpu_valid_mode(uint8_t mode){
    if(
        mode == gba_cpu_user_mode ||
        mode == gba_cpu_fiq_mode ||
        mode == gba_cpu_irq_mode ||
        mode == gba_cpu_supervisor_mode ||
        mode == gba_cpu_abort_mode ||
        mode == gba_cpu_undefined_mode ||
        mode == gba_cpu_system_mode
    ){
        return true;
    }
    else{
        return false;
    }
}


#define gba_cpu_set_nz_flag(result)\
    gba_cpu_set_flag(gba_cpu_zero_flag,!result);\
    gba_cpu_set_flag(gba_cpu_negative_flag,result & (1 << 31));\

#define gba_cpu_set_logical_flag(result,carry)\
    gba_cpu_set_flag(gba_cpu_carry_flag,carry);\
    gba_cpu_set_nz_flag(result)\


uint32_t gba_cpu_alu_and(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    uint32_t result = op1 & op2;
    if(set_flags){
        gba_cpu_set_logical_flag(result,carry);
    }
    return result;
}

uint32_t gba_cpu_alu_xor(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    uint32_t result = op1 ^ op2;
    if(set_flags){
        gba_cpu_set_logical_flag(result,carry);
    }
    return result;
}

uint32_t gba_cpu_alu_orr(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    uint32_t result = op1 | op2;
    if(set_flags){
        gba_cpu_set_logical_flag(result,carry);
    }
    return result;
}

uint32_t gba_cpu_alu_mov(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    gba_unused(op1);
    if(set_flags){
        gba_cpu_set_logical_flag(op2,carry);
    }
    return op2;
}

uint32_t gba_cpu_alu_bic(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    uint32_t result = op1 & ~op2;
    if(set_flags){
        gba_cpu_set_logical_flag(result,carry);
    }
    return result;
}

uint32_t gba_cpu_alu_mvn(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    uint32_t result = ~op2;
    if(set_flags){
        gba_cpu_set_logical_flag(result,carry);
    }
    return result;
}

uint32_t gba_cpu_alu_arithmetic(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    uint64_t result64 = op1 + op2 + carry;
    uint32_t result32 = result64;

    if(set_flags){
        gba_cpu_set_nz_flag(result32);
        gba_cpu_set_flag(gba_cpu_carry_flag,result64 >> 32);
        gba_cpu_set_flag(gba_cpu_overflow_flag,(~(op1 ^ op2) & (op1 ^ result32)) >> 31);
    }

    return result32;
}


uint32_t gba_cpu_lsl(uint32_t value,uint8_t shift,bool* carry){
    if(shift){
        *carry = shift < 33 ? (value & (1 << (32 - shift))) : 0;
        value = shift < 32 ? (value << shift) : 0;
    }
    return value;
}

uint32_t gba_cpu_lsr(uint32_t value,uint8_t shift,bool* carry){
    if(shift){
        *carry = shift < 33 ? (value & (1 << (shift - 1))) : 0;
        value = shift < 32 ? (value >> shift) : 0;
    }
    return value;
}

uint32_t gba_cpu_asr(uint32_t value,uint8_t shift,bool* carry){
    if(shift){
        if(shift < 32){
            *carry = value & (1 << (shift - 1));
            value = (int32_t)value >> shift;
        }
        else{
            *carry = value & (1 << 31);
            value = (int32_t)value >> 31;
        }
    }
    return value;
}

uint32_t gba_cpu_ror(uint32_t value,uint8_t shift,bool* carry){
    if(shift){
        shift &= 31;
        if(shift){
            value = (value >> shift) | (value << (32 - shift));
        }
        *carry = value & (1 << 31);
    }
    return value;
}

uint32_t gba_cpu_rrx(uint32_t value,bool* carry){
    bool carry_in = *carry;
    *carry = value & 1;
    return (value >> 1) | (carry_in << 31);
}


void gba_cpu_execute(gba_cpu_t* cpu){
    gba_cpu_pipeline_t* pipeline = &cpu->pipeline;

    pipeline->execute = pipeline->decode;

    pipeline->decode = pipeline->fetch;

    if(cpu->cpsr & gba_cpu_thumb_flag){
        pipeline->fetch = gba_cpu_read_halfword(cpu,cpu->r[15]);
    }
    else{
        pipeline->fetch = gba_cpu_read_word(cpu,cpu->r[15]);

        if(gba_cpu_condition_table[(pipeline->execute >> 24) | (cpu->cpsr >> 28)]){

            uint32_t i = ((pipeline->execute & 0x0FF00000) >> 16) | ((pipeline->execute & 0x000000F0) >> 4);

            arm_table[i](cpu,pipeline->execute);
        }
        else{
            cpu->r[15] += 4;
        }
    }
}