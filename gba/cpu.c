#include "cpu.h"
#include "arm.h"
#include "thumb.h"
#include "gba.h"

const bool gba_cpu_condition_table[256] = {
    0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
    1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,
    0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,
    1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,
    0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
    0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
    0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,
    1,1,0,0,1,1,1,1,1,1,0,0,1,1,1,1,
    1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,
    0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,
    1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,0,
    0,1,0,1,1,1,1,1,1,0,1,0,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

const uint8_t gba_cpu_mode_bank[32] = {
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,1,4,2,6,6,6,3,6,6,6,5,6,6,6,0
};


void gba_cpu_init(gba_cpu_t* cpu,gba_t* gba){
    cpu->gba = gba;

    gba_cpu_arm_init_table();
    gba_cpu_thumb_init_table();
}


void gba_cpu_idle_cycle(gba_cpu_t* cpu){
    gba_clock(cpu->gba,1);
}


void gba_cpu_write_byte(gba_cpu_t* cpu,uint8_t value,uint32_t address,uint8_t access){
    gba_memory_write(&cpu->gba->memory,value,address,gba_memory_byte_access,access);
}

uint8_t gba_cpu_read_byte(gba_cpu_t* cpu,uint32_t address,uint8_t access){
    return gba_memory_read(&cpu->gba->memory,address,gba_memory_byte_access,access);
}

uint32_t gba_cpu_read_signed_byte(gba_cpu_t* cpu,uint32_t address,uint8_t access){
    uint32_t value = gba_cpu_read_byte(cpu,address,access);
    if(value & 0x00000080){
        value |= 0xFFFFFF00;
    }
    return value;
}


void gba_cpu_write_halfword(gba_cpu_t* cpu,uint16_t value,uint32_t address,uint8_t access){
    gba_memory_write(&cpu->gba->memory,value,address,gba_memory_halfword_access,access);
}

uint16_t gba_cpu_read_halfword(gba_cpu_t* cpu,uint32_t address,uint8_t access){
    return gba_memory_read(&cpu->gba->memory,address,gba_memory_halfword_access,access);
}

uint32_t gba_cpu_read_halfword_rotate(gba_cpu_t* cpu,uint32_t address,uint8_t access){
    uint32_t value = gba_cpu_read_halfword(cpu,address,access);
    if(address & 1){
        value = (value >> 8) | (value << 24);
    }
    return value;
}

uint32_t gba_cpu_read_signed_halfword(gba_cpu_t* cpu,uint32_t address,uint8_t access){
    uint32_t value = gba_cpu_read_halfword(cpu,address,access);
    if(value & 0x00008000){
        value |= 0xFFFF0000;
    }
    if(address & 1){
        value = (int32_t)value >> 8;
    }
    return value;
}


void gba_cpu_write_word(gba_cpu_t* cpu,uint32_t value,uint32_t address,uint8_t access){
    gba_memory_write(&cpu->gba->memory,value,address,gba_memory_word_access,access);
}

uint32_t gba_cpu_read_word(gba_cpu_t* cpu,uint32_t address,uint8_t access){
    return gba_memory_read(&cpu->gba->memory,address,gba_memory_word_access,access);
}

uint32_t gba_cpu_read_word_rotate(gba_cpu_t* cpu,uint32_t address,uint8_t access){
    uint32_t value = gba_cpu_read_word(cpu,address,access);
    uint8_t shift = (address & 3) << 3;
    if(shift){
        value = (value >> shift) | (value << (32 - shift));
    }
    return value;
}


void gba_cpu_set_cpsr(gba_cpu_t* cpu,uint32_t value){
    // TODO: What happens when writing an invalid mode to CPSR?
    
    gba_cpu_set_mode(cpu,value & gba_cpu_mode_flag);

    cpu->cpsr = (value & ~gba_cpu_mode_flag) | (cpu->cpsr & gba_cpu_mode_flag);
}

void gba_cpu_set_mode(gba_cpu_t* cpu,uint8_t new_mode){

    // The hardware keeps bit 4 set, according to the ROM psr.gba
    new_mode |= 0x10;

    uint8_t old_mode = cpu->cpsr & gba_cpu_mode_flag;

    if(new_mode == old_mode) return;

    cpu->cpsr &= ~gba_cpu_mode_flag;
    cpu->cpsr |= new_mode;

    uint8_t old_mode_bank = gba_cpu_mode_bank[old_mode];
    uint8_t new_mode_bank = gba_cpu_mode_bank[new_mode];

    if(old_mode == gba_cpu_fiq_mode){
        for(int i = 0; i < 5; ++i){
            cpu->r8_r12[gba_cpu_fiq_bank][i] = cpu->r[8 + i];
            cpu->r[8 + i] = cpu->r8_r12[gba_cpu_none_bank][i];
        }
    }
    else if(new_mode == gba_cpu_fiq_mode){
        for(int i = 0; i < 5; ++i){
            cpu->r8_r12[gba_cpu_none_bank][i] = cpu->r[8 + i];
            cpu->r[8 + i] = cpu->r8_r12[gba_cpu_fiq_bank][i];
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

        cpu->pipeline.decode = gba_cpu_read_halfword(cpu,cpu->r[15] + 0,gba_nonsequential_access);
        cpu->pipeline.fetch = gba_cpu_read_halfword(cpu,cpu->r[15] + 2,gba_sequential_access);

        cpu->pipeline.access = gba_sequential_access;
        cpu->r[15] += 4;
    }
    else{
        cpu->r[15] &= ~3;

        cpu->pipeline.decode = gba_cpu_read_word(cpu,cpu->r[15] + 0,gba_nonsequential_access);
        cpu->pipeline.fetch = gba_cpu_read_word(cpu,cpu->r[15] + 4,gba_sequential_access);

        cpu->pipeline.access = gba_sequential_access;
        cpu->r[15] += 8;
    }
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


uint32_t gba_cpu_alu_arithmetic(gba_cpu_t* cpu,uint32_t op1,uint32_t op2,bool carry,bool set_flags){
    uint64_t result64 = (uint64_t)op1 + op2 + carry;
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


void gba_cpu_multiply_clock(gba_cpu_t* cpu,uint32_t value,bool _signed){

    uint32_t mask = 0xFFFFFF00;
    
    while(true){
        gba_cpu_idle_cycle(cpu);
        
        value &= mask;

        if(value == 0 || (_signed && value == mask)) break;
        
        mask <<= 8;
    }
}


void gba_cpu_execute(gba_cpu_t* cpu){
    gba_cpu_pipeline_t* pipeline = &cpu->pipeline;

    pipeline->execute = pipeline->decode;

    pipeline->decode = pipeline->fetch;

    if(cpu->cpsr & gba_cpu_thumb_flag){
        pipeline->fetch = gba_cpu_read_halfword(cpu,cpu->r[15],cpu->pipeline.access);

        uint32_t i = (pipeline->execute & 0x0000FF80) >> 7;

        thumb_table[i](cpu,pipeline->execute);
    }
    else{
        pipeline->fetch = gba_cpu_read_word(cpu,cpu->r[15],cpu->pipeline.access);

        if(gba_cpu_condition_table[((pipeline->execute >> 24) & 0xF0) | (cpu->cpsr >> 28)]){

            uint32_t i = ((pipeline->execute & 0x0FF00000) >> 16) | ((pipeline->execute & 0x000000F0) >> 4);

            arm_table[i](cpu,pipeline->execute);
        }
        else{
            cpu->pipeline.access = gba_sequential_access;
            cpu->r[15] += 4;
        }
    }
}


void gba_cpu_reset(gba_cpu_t* cpu){
    memset(cpu->r8_r12,0x00,sizeof(cpu->r8_r12));
    memset(cpu->r13,0x00,sizeof(cpu->r13));
    memset(cpu->r14,0x00,sizeof(cpu->r14));
    memset(cpu->spsr,0x00,sizeof(cpu->spsr));
    memset(cpu->r,0x00,sizeof(cpu->r));
    memset(&cpu->pipeline,0x00,sizeof(cpu->pipeline));

    cpu->cpsr = gba_cpu_irq_flag | gba_cpu_fiq_flag | gba_cpu_supervisor_mode;
}