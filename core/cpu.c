#include "./cpu.h"
#include "./gb.h"

#define gb_cpu_set_flag(flag,state) (state) ? (cpu->f |= (flag)) : (cpu->f &= ~(flag))

void gb_cpu_init(gb_cpu_t* cpu,gb_t* gb){
    cpu->gb = gb;
}


void gb_cpu_cycle(gb_cpu_t* cpu){
    gb_master_clock(cpu->gb);
    gb_master_clock(cpu->gb);
    gb_master_clock(cpu->gb);
    gb_master_clock(cpu->gb);
}


void gb_cpu_write_byte(gb_cpu_t* cpu,uint8_t value,uint16_t address){
    gb_cpu_cycle(cpu);
    gb_memory_write(&cpu->gb->memory,value,address);
}

uint8_t gb_cpu_read_byte(gb_cpu_t* cpu,uint16_t address){
    gb_cpu_cycle(cpu);
    return gb_memory_read(&cpu->gb->memory,address);
}


void gb_cpu_write_word(gb_cpu_t* cpu,uint16_t value,uint16_t address){
    gb_cpu_write_byte(cpu,value & 0xFF,address);
    gb_cpu_write_byte(cpu,value >> 0x08,address + 0x01);
}

uint16_t gb_cpu_read_word(gb_cpu_t* cpu,uint16_t address){
    uint8_t lo = gb_cpu_read_byte(cpu,address);
    uint8_t hi = gb_cpu_read_byte(cpu,address + 0x01);
    return (hi << 0x08) | lo;
}


void gb_cpu_ld_r16_imm16(gb_cpu_t* cpu,uint16_t* word){
    *word = gb_cpu_read_word(cpu,cpu->pc);
    cpu->pc += 2;
}

void gb_cpu_ld_pimm16_byte(gb_cpu_t* cpu,uint8_t byte){
    uint16_t address = gb_cpu_read_word(cpu,cpu->pc);
    cpu->pc += 2;
    gb_cpu_write_byte(cpu,byte,address);
}

void gb_cpu_ld_pimm16_word(gb_cpu_t* cpu,uint16_t word){
    uint16_t address = gb_cpu_read_word(cpu,cpu->pc);
    cpu->pc += 2;
    gb_cpu_write_word(cpu,word,address);
}

void gb_cpu_ld_a_pimm16(gb_cpu_t* cpu){
    uint16_t address = gb_cpu_read_word(cpu,cpu->pc);
    cpu->pc += 2;
    cpu->a = gb_cpu_read_byte(cpu,address);
}

void gb_cpu_ld_sp_hl(gb_cpu_t* cpu){
    cpu->sp = cpu->hl;
    gb_cpu_cycle(cpu);
}


void gb_cpu_inc_byte(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(*byte & 0x0F) == 0x0F);
    *byte += 1;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
}

void gb_cpu_inc_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_inc_byte(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_dec_byte(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(*byte & 0x0F) == 0x00);
    *byte -= 1;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
}

void gb_cpu_dec_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_dec_byte(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_inc_word(gb_cpu_t* cpu,uint16_t* word){
    *word += 1;
    gb_cpu_cycle(cpu);
}

void gb_cpu_dec_word(gb_cpu_t* cpu,uint16_t* word){
    *word -= 1;
    gb_cpu_cycle(cpu);
}


void gb_cpu_rlca(gb_cpu_t* cpu){
    uint8_t b7 = cpu->a & 0x80;
    cpu->a = (cpu->a << 0x01) | (b7 ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag | gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

void gb_cpu_rrca(gb_cpu_t* cpu){
    uint8_t b0 = cpu->a & 0x01;
    cpu->a = (cpu->a >> 0x01) | (b0 ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag | gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

void gb_cpu_rla(gb_cpu_t* cpu){
    uint8_t b7 = cpu->a & 0x80;
    cpu->a = (cpu->a << 0x01) | ((cpu->f & gb_cpu_carry_flag) ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag | gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

void gb_cpu_rra(gb_cpu_t* cpu){
    uint8_t b0 = cpu->a & 0x01;
    cpu->a = (cpu->a >> 0x01) | ((cpu->f & gb_cpu_carry_flag) ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag | gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

void gb_cpu_daa(gb_cpu_t* cpu){
    if(cpu->f & gb_cpu_subtraction_flag){
        if(cpu->f & gb_cpu_carry_flag){
            cpu->a -= 0x60;
        }
        if(cpu->f & gb_cpu_half_carry_flag){
            cpu->a -= 0x06;
        }
    }
    else{
        if((cpu->f & gb_cpu_carry_flag) || cpu->a > 0x99){
            cpu->a += 0x60;
            gb_cpu_set_flag(gb_cpu_carry_flag,true);
        }        
        if((cpu->f & gb_cpu_half_carry_flag) || (cpu->a & 0x0F) > 0x09){
            cpu->a += 0x06;
        }
    }

    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,false);
}

void gb_cpu_cpl(gb_cpu_t* cpu){
    cpu->a = ~cpu->a;
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,true);
}

void gb_cpu_scf(gb_cpu_t* cpu){
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,true);
}

void gb_cpu_ccf(gb_cpu_t* cpu){
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,(cpu->f & gb_cpu_carry_flag) ? 0x00 : 0x01);
}


void gb_cpu_rlc(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b7 = *byte & 0x80;
    *byte = (*byte << 0x01) |(b7 ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

void gb_cpu_rlc_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_rlc(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_rl(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b7 = *byte & 0x80;
    *byte = (*byte << 0x01) | ((cpu->f & gb_cpu_carry_flag) ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

void gb_cpu_rl_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_rl(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_rr(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b0 = *byte & 0x01;
    *byte = (*byte >> 0x01) | ((cpu->f & gb_cpu_carry_flag) ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

void gb_cpu_rr_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_rr(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_rrc(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b0 = *byte & 0x01;
    *byte = (*byte >> 0x01) | (b0 ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

void gb_cpu_rrc_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_rrc(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_sla(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_carry_flag,*byte & 0x80);
    *byte <<= 0x01;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
}

void gb_cpu_sla_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_sla(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_sra(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_carry_flag,*byte & 0x01);
    *byte = (*byte & 0x80) | (*byte >> 0x01);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
}

void gb_cpu_sra_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_sra(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_swap(gb_cpu_t* cpu,uint8_t* byte){
    *byte = (*byte << 0x04) | (*byte >> 0x04); 
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag | gb_cpu_carry_flag,false);
}

void gb_cpu_swap_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_swap(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_srl(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_carry_flag,*byte & 0x01);
    *byte >>= 0x01;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag,false);
}

void gb_cpu_srl_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    gb_cpu_srl(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_bit(gb_cpu_t* cpu,bool bit){
    gb_cpu_set_flag(gb_cpu_zero_flag,!bit);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,true);
}

void gb_cpu_res_phl(gb_cpu_t* cpu,uint8_t bit){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    byte &= ~bit;
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}

void gb_cpu_set_phl(gb_cpu_t* cpu,uint8_t bit){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->hl);
    byte |= bit;
    gb_cpu_write_byte(cpu,byte,cpu->hl);
}



void gb_cpu_jr_imm8(gb_cpu_t* cpu,bool condition){
    int8_t offset = gb_cpu_read_byte(cpu,cpu->pc++);
    if(condition){
        cpu->pc += offset;
        gb_cpu_cycle(cpu);
    }
}

void gb_cpu_jp_imm16(gb_cpu_t* cpu,bool condition){
    uint16_t address = gb_cpu_read_word(cpu,cpu->pc);
    cpu->pc += 2;
    if(condition){
        cpu->pc = address;
        gb_cpu_cycle(cpu);
    }
}

void gb_cpu_call_imm16(gb_cpu_t* cpu,bool condition){
    uint16_t address = gb_cpu_read_word(cpu,cpu->pc);
    cpu->pc += 2;
    if(condition){
        gb_cpu_cycle(cpu);
        gb_cpu_write_byte(cpu,cpu->pc >> 0x08,--cpu->sp);
        gb_cpu_write_byte(cpu,cpu->pc & 0xFF,--cpu->sp);
        cpu->pc = address;
    }
}


void gb_cpu_add_byte(gb_cpu_t* cpu,uint8_t byte){
    int result = cpu->a + byte;
    
    gb_cpu_set_flag(gb_cpu_carry_flag,result > 0xFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->a & 0x0F) + (byte & 0x0F) > 0x0F);

    cpu->a = result;

    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
}

void gb_cpu_adc_byte(gb_cpu_t* cpu,uint8_t byte){
    uint8_t carry = ((cpu->f & gb_cpu_carry_flag) ? 0x01 : 0x00);

    int result = cpu->a + byte + carry;

    gb_cpu_set_flag(gb_cpu_carry_flag,result > 0xFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->a & 0x0F) + (byte & 0x0F) + carry > 0x0F);

    cpu->a = result;

    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
}

void gb_cpu_sub_byte(gb_cpu_t* cpu,uint8_t byte){
    int result = cpu->a - byte;

    gb_cpu_set_flag(gb_cpu_carry_flag,result < 0x00);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->a & 0x0F) < (byte & 0x0F));

    cpu->a = result;

    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
}

void gb_cpu_sbc_byte(gb_cpu_t* cpu,uint8_t byte){
    uint8_t carry = ((cpu->f & gb_cpu_carry_flag) ? 0x01 : 0x00);

    int result = cpu->a - byte - carry;

    gb_cpu_set_flag(gb_cpu_carry_flag,result < 0x00);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->a & 0x0F) < (byte & 0x0F) + carry);

    cpu->a = result;

    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
}

void gb_cpu_and_byte(gb_cpu_t* cpu,uint8_t byte){
    cpu->a &= byte;
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,true);
}

void gb_cpu_xor_byte(gb_cpu_t* cpu,uint8_t byte){
    cpu->a ^= byte;
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag | gb_cpu_carry_flag,false);
}

void gb_cpu_or_byte(gb_cpu_t* cpu,uint8_t byte){
    cpu->a |= byte;
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_half_carry_flag | gb_cpu_carry_flag,false);
}

void gb_cpu_cp_byte(gb_cpu_t* cpu,uint8_t byte){
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->a == byte);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
    gb_cpu_set_flag(gb_cpu_carry_flag,cpu->a < byte);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->a & 0x0F) < (byte & 0x0F));
}


uint16_t gb_cpu_sp_plus_imm8(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->pc++);
    gb_cpu_set_flag(gb_cpu_carry_flag,(cpu->sp & 0xFF) + byte > 0xFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->sp & 0x0F) + (byte & 0x0F) > 0x0F);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_zero_flag,false);
    return cpu->sp + (int8_t)byte;
}

void gb_cpu_add_sp_imm8(gb_cpu_t* cpu){
    cpu->sp = gb_cpu_sp_plus_imm8(cpu);
    gb_cpu_cycle(cpu);
    gb_cpu_cycle(cpu);
}

void gb_cpu_ld_hl_sp_plus_imm8(gb_cpu_t* cpu){
    cpu->hl = gb_cpu_sp_plus_imm8(cpu);
    gb_cpu_cycle(cpu);
}


void gb_cpu_add_hl_word(gb_cpu_t* cpu,uint16_t word){
    int result = cpu->hl + word;
    gb_cpu_set_flag(gb_cpu_carry_flag,result > 0xFFFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->hl & 0x0FFF) + (word & 0x0FFF) > 0x0FFF);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    cpu->hl = result;
}


void gb_cpu_push_word(gb_cpu_t* cpu,uint16_t word){
    gb_cpu_cycle(cpu);
    gb_cpu_write_byte(cpu,word >> 0x08,--cpu->sp);
    gb_cpu_write_byte(cpu,word & 0xFF,--cpu->sp);
}

void gb_cpu_pop_word(gb_cpu_t* cpu,uint16_t* word){
    *word = gb_cpu_read_byte(cpu,cpu->sp++);
    *word |= gb_cpu_read_byte(cpu,cpu->sp++) << 0x08;
}

void gb_cpu_push_af(gb_cpu_t* cpu){
    gb_cpu_cycle(cpu);
    gb_cpu_write_byte(cpu,cpu->a,--cpu->sp);
    gb_cpu_write_byte(cpu,cpu->f & 0xF0,--cpu->sp);
}

void gb_cpu_pop_af(gb_cpu_t* cpu){
    cpu->f = gb_cpu_read_byte(cpu,cpu->sp++) & 0xF0;
    cpu->a = gb_cpu_read_byte(cpu,cpu->sp++);
}


void gb_cpu_ret(gb_cpu_t* cpu){
    cpu->pc = gb_cpu_read_byte(cpu,cpu->sp++);
    cpu->pc |= gb_cpu_read_byte(cpu,cpu->sp++) << 0x08;
    gb_cpu_cycle(cpu);
}

void gb_cpu_ret_cc(gb_cpu_t* cpu,bool condition){
    gb_cpu_cycle(cpu);
    if(condition){
        cpu->pc = gb_cpu_read_byte(cpu,cpu->sp++);
        cpu->pc |= gb_cpu_read_byte(cpu,cpu->sp++) << 0x08;
        gb_cpu_cycle(cpu);
    }
}

void gb_cpu_reti(gb_cpu_t* cpu){
    cpu->pc = gb_cpu_read_byte(cpu,cpu->sp++);
    cpu->pc |= gb_cpu_read_byte(cpu,cpu->sp++) << 0x08;
    cpu->ime = true;
    gb_cpu_cycle(cpu);
}

void gb_cpu_rst(gb_cpu_t* cpu,uint8_t target){
    gb_cpu_cycle(cpu);
    gb_cpu_write_byte(cpu,cpu->pc >> 0x08,--cpu->sp);
    gb_cpu_write_byte(cpu,cpu->pc & 0xFF,--cpu->sp);
    cpu->pc = target;
}


void gb_cpu_halt(gb_cpu_t* cpu){
    if(cpu->ime || !(cpu->gb->interrupt.enable & cpu->gb->interrupt.flag)){
        cpu->halted = true;
    }
    cpu->halt_fetch = true;
}


void gb_cpu_prefix(gb_cpu_t* cpu,uint8_t prefix){
    switch(prefix){
        //RLC B
        case 0x00: gb_cpu_rlc(cpu,&cpu->b); break;
        //RLC C
        case 0x01: gb_cpu_rlc(cpu,&cpu->c); break;
        //RLC D
        case 0x02: gb_cpu_rlc(cpu,&cpu->d); break;
        //RLC E
        case 0x03: gb_cpu_rlc(cpu,&cpu->e); break;
        //RLC H
        case 0x04: gb_cpu_rlc(cpu,&cpu->h); break;
        //RLC L
        case 0x05: gb_cpu_rlc(cpu,&cpu->l); break;
        //RLC [HL]
        case 0x06: gb_cpu_rlc_phl(cpu); break;
        //RLC A
        case 0x07: gb_cpu_rlc(cpu,&cpu->a); break;
        //RRC B
        case 0x08: gb_cpu_rrc(cpu,&cpu->b); break;
        //RRC C
        case 0x09: gb_cpu_rrc(cpu,&cpu->c); break;
        //RRC D
        case 0x0A: gb_cpu_rrc(cpu,&cpu->d); break;
        //RRC E
        case 0x0B: gb_cpu_rrc(cpu,&cpu->e); break;
        //RRC H
        case 0x0C: gb_cpu_rrc(cpu,&cpu->h); break;
        //RRC L
        case 0x0D: gb_cpu_rrc(cpu,&cpu->l); break;
        //RRC [HL]
        case 0x0E: gb_cpu_rrc_phl(cpu); break;
        //RRC A
        case 0x0F: gb_cpu_rrc(cpu,&cpu->a); break;

        //RL B
        case 0x10: gb_cpu_rl(cpu,&cpu->b); break;
        //RL C
        case 0x11: gb_cpu_rl(cpu,&cpu->c); break;
        //RL D
        case 0x12: gb_cpu_rl(cpu,&cpu->d); break;
        //RL E
        case 0x13: gb_cpu_rl(cpu,&cpu->e); break;
        //RL H
        case 0x14: gb_cpu_rl(cpu,&cpu->h); break;
        //RL L
        case 0x15: gb_cpu_rl(cpu,&cpu->l); break;
        //RL [HL]
        case 0x16: gb_cpu_rl_phl(cpu); break;
        //RL A
        case 0x17: gb_cpu_rl(cpu,&cpu->a); break;
        //RR B
        case 0x18: gb_cpu_rr(cpu,&cpu->b); break;
        //RR C
        case 0x19: gb_cpu_rr(cpu,&cpu->c); break;
        //RR D
        case 0x1A: gb_cpu_rr(cpu,&cpu->d); break;
        //RR E
        case 0x1B: gb_cpu_rr(cpu,&cpu->e); break;
        //RR H
        case 0x1C: gb_cpu_rr(cpu,&cpu->h); break;
        //RR L
        case 0x1D: gb_cpu_rr(cpu,&cpu->l); break;
        //RR [HL]
        case 0x1E: gb_cpu_rr_phl(cpu); break;
        //RR A
        case 0x1F: gb_cpu_rr(cpu,&cpu->a); break;

        //SLA B
        case 0x20: gb_cpu_sla(cpu,&cpu->b); break;
        //SLA C
        case 0x21: gb_cpu_sla(cpu,&cpu->c); break;
        //SLA D
        case 0x22: gb_cpu_sla(cpu,&cpu->d); break;
        //SLA E
        case 0x23: gb_cpu_sla(cpu,&cpu->e); break;
        //SLA H
        case 0x24: gb_cpu_sla(cpu,&cpu->h); break;
        //SLA L
        case 0x25: gb_cpu_sla(cpu,&cpu->l); break;
        //SLA [HL]
        case 0x26: gb_cpu_sla_phl(cpu); break;
        //SLA A
        case 0x27: gb_cpu_sla(cpu,&cpu->a); break;
        //SRA B
        case 0x28: gb_cpu_sra(cpu,&cpu->b); break;
        //SRA C
        case 0x29: gb_cpu_sra(cpu,&cpu->c); break;
        //SRA D
        case 0x2A: gb_cpu_sra(cpu,&cpu->d); break;
        //SRA E
        case 0x2B: gb_cpu_sra(cpu,&cpu->e); break;
        //SRA H
        case 0x2C: gb_cpu_sra(cpu,&cpu->h); break;
        //SRA L
        case 0x2D: gb_cpu_sra(cpu,&cpu->l); break;
        //SRA [HL]
        case 0x2E: gb_cpu_sra_phl(cpu); break;
        //SRA A
        case 0x2F: gb_cpu_sra(cpu,&cpu->a); break;

        //SWAP B
        case 0x30: gb_cpu_swap(cpu,&cpu->b); break;
        //SWAP C
        case 0x31: gb_cpu_swap(cpu,&cpu->c); break;
        //SWAP D
        case 0x32: gb_cpu_swap(cpu,&cpu->d); break;
        //SWAP E
        case 0x33: gb_cpu_swap(cpu,&cpu->e); break;
        //SWAP H
        case 0x34: gb_cpu_swap(cpu,&cpu->h); break;
        //SWAP L
        case 0x35: gb_cpu_swap(cpu,&cpu->l); break;
        //SWAP [HL]
        case 0x36: gb_cpu_swap_phl(cpu); break;
        //SWAP A
        case 0x37: gb_cpu_swap(cpu,&cpu->a); break;
        //SRL B
        case 0x38: gb_cpu_srl(cpu,&cpu->b); break;
        //SRL C
        case 0x39: gb_cpu_srl(cpu,&cpu->c); break;
        //SRL D
        case 0x3A: gb_cpu_srl(cpu,&cpu->d); break;
        //SRL E
        case 0x3B: gb_cpu_srl(cpu,&cpu->e); break;
        //SRL H
        case 0x3C: gb_cpu_srl(cpu,&cpu->h); break;
        //SRL L
        case 0x3D: gb_cpu_srl(cpu,&cpu->l); break;
        //SRL [HL]
        case 0x3E: gb_cpu_srl_phl(cpu); break;
        //SRL A
        case 0x3F: gb_cpu_srl(cpu,&cpu->a); break;

        //BIT 0,B
        case 0x40: gb_cpu_bit(cpu,cpu->b & 0x01); break;
        //BIT 0,C
        case 0x41: gb_cpu_bit(cpu,cpu->c & 0x01); break;
        //BIT 0,D
        case 0x42: gb_cpu_bit(cpu,cpu->d & 0x01); break;
        //BIT 0,E
        case 0x43: gb_cpu_bit(cpu,cpu->e & 0x01); break;
        //BIT 0,H
        case 0x44: gb_cpu_bit(cpu,cpu->h & 0x01); break;
        //BIT 0,L
        case 0x45: gb_cpu_bit(cpu,cpu->l & 0x01); break;
        //BIT 0,[HL]
        case 0x46: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x01); break;
        //BIT 0,A
        case 0x47: gb_cpu_bit(cpu,cpu->a & 0x01); break;
        //BIT 1,B
        case 0x48: gb_cpu_bit(cpu,cpu->b & 0x02); break;
        //BIT 1,C
        case 0x49: gb_cpu_bit(cpu,cpu->c & 0x02); break;
        //BIT 1,D
        case 0x4A: gb_cpu_bit(cpu,cpu->d & 0x02); break;
        //BIT 1,E
        case 0x4B: gb_cpu_bit(cpu,cpu->e & 0x02); break;
        //BIT 1,H
        case 0x4C: gb_cpu_bit(cpu,cpu->h & 0x02); break;
        //BIT 1,L
        case 0x4D: gb_cpu_bit(cpu,cpu->l & 0x02); break;
        //BIT 1,[HL]
        case 0x4E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x02); break;
        //BIT 1,A
        case 0x4F: gb_cpu_bit(cpu,cpu->a & 0x02); break;

        //BIT 2,B
        case 0x50: gb_cpu_bit(cpu,cpu->b & 0x04); break;
        //BIT 2,C
        case 0x51: gb_cpu_bit(cpu,cpu->c & 0x04); break;
        //BIT 2,D
        case 0x52: gb_cpu_bit(cpu,cpu->d & 0x04); break;
        //BIT 2,E
        case 0x53: gb_cpu_bit(cpu,cpu->e & 0x04); break;
        //BIT 2,H
        case 0x54: gb_cpu_bit(cpu,cpu->h & 0x04); break;
        //BIT 2,L
        case 0x55: gb_cpu_bit(cpu,cpu->l & 0x04); break;
        //BIT 2,[HL]
        case 0x56: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x04); break;
        //BIT 2,A
        case 0x57: gb_cpu_bit(cpu,cpu->a & 0x04); break;
        //BIT 3,B
        case 0x58: gb_cpu_bit(cpu,cpu->b & 0x08); break;
        //BIT 3,C
        case 0x59: gb_cpu_bit(cpu,cpu->c & 0x08); break;
        //BIT 3,D
        case 0x5A: gb_cpu_bit(cpu,cpu->d & 0x08); break;
        //BIT 3,E
        case 0x5B: gb_cpu_bit(cpu,cpu->e & 0x08); break;
        //BIT 3,H
        case 0x5C: gb_cpu_bit(cpu,cpu->h & 0x08); break;
        //BIT 3,L
        case 0x5D: gb_cpu_bit(cpu,cpu->l & 0x08); break;
        //BIT 3,[HL]
        case 0x5E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x08); break;
        //BIT 3,A
        case 0x5F: gb_cpu_bit(cpu,cpu->a & 0x08); break;

        //BIT 4,B
        case 0x60: gb_cpu_bit(cpu,cpu->b & 0x10); break;
        //BIT 4,C
        case 0x61: gb_cpu_bit(cpu,cpu->c & 0x10); break;
        //BIT 4,D
        case 0x62: gb_cpu_bit(cpu,cpu->d & 0x10); break;
        //BIT 4,E
        case 0x63: gb_cpu_bit(cpu,cpu->e & 0x10); break;
        //BIT 4,H
        case 0x64: gb_cpu_bit(cpu,cpu->h & 0x10); break;
        //BIT 4,L
        case 0x65: gb_cpu_bit(cpu,cpu->l & 0x10); break;
        //BIT 4,[HL]
        case 0x66: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x10); break;
        //BIT 4,A
        case 0x67: gb_cpu_bit(cpu,cpu->a & 0x10); break;
        //BIT 5,B
        case 0x68: gb_cpu_bit(cpu,cpu->b & 0x20); break;
        //BIT 5,C
        case 0x69: gb_cpu_bit(cpu,cpu->c & 0x20); break;
        //BIT 5,D
        case 0x6A: gb_cpu_bit(cpu,cpu->d & 0x20); break;
        //BIT 5,E
        case 0x6B: gb_cpu_bit(cpu,cpu->e & 0x20); break;
        //BIT 5,H
        case 0x6C: gb_cpu_bit(cpu,cpu->h & 0x20); break;
        //BIT 5,L
        case 0x6D: gb_cpu_bit(cpu,cpu->l & 0x20); break;
        //BIT 5,[HL]
        case 0x6E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x20); break;
        //BIT 5,A
        case 0x6F: gb_cpu_bit(cpu,cpu->a & 0x20); break;

        //BIT 6,B
        case 0x70: gb_cpu_bit(cpu,cpu->b & 0x40); break;
        //BIT 6,C
        case 0x71: gb_cpu_bit(cpu,cpu->c & 0x40); break;
        //BIT 6,D
        case 0x72: gb_cpu_bit(cpu,cpu->d & 0x40); break;
        //BIT 6,E
        case 0x73: gb_cpu_bit(cpu,cpu->e & 0x40); break;
        //BIT 6,H
        case 0x74: gb_cpu_bit(cpu,cpu->h & 0x40); break;
        //BIT 6,L
        case 0x75: gb_cpu_bit(cpu,cpu->l & 0x40); break;
        //BIT 6,[HL]
        case 0x76: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x40); break;
        //BIT 6,A
        case 0x77: gb_cpu_bit(cpu,cpu->a & 0x40); break;
        //BIT 7,B
        case 0x78: gb_cpu_bit(cpu,cpu->b & 0x80); break;
        //BIT 7,C
        case 0x79: gb_cpu_bit(cpu,cpu->c & 0x80); break;
        //BIT 7,D
        case 0x7A: gb_cpu_bit(cpu,cpu->d & 0x80); break;
        //BIT 7,E
        case 0x7B: gb_cpu_bit(cpu,cpu->e & 0x80); break;
        //BIT 7,H
        case 0x7C: gb_cpu_bit(cpu,cpu->h & 0x80); break;
        //BIT 7,L
        case 0x7D: gb_cpu_bit(cpu,cpu->l & 0x80); break;
        //BIT 7,[HL]
        case 0x7E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->hl) & 0x80); break;
        //BIT 7,A
        case 0x7F: gb_cpu_bit(cpu,cpu->a & 0x80); break;

        //RES 0,B
        case 0x80: cpu->b &= ~0x01; break;
        //RES 0,C
        case 0x81: cpu->c &= ~0x01; break;
        //RES 0,D
        case 0x82: cpu->d &= ~0x01; break;
        //RES 0,E
        case 0x83: cpu->e &= ~0x01; break;
        //RES 0,H
        case 0x84: cpu->h &= ~0x01; break;
        //RES 0,L
        case 0x85: cpu->l &= ~0x01; break;
        //RES 0,[HL]
        case 0x86: gb_cpu_res_phl(cpu,0x01); break;
        //RES 0,A
        case 0x87: cpu->a &= ~0x01; break;
        //RES 1,B
        case 0x88: cpu->b &= ~0x02; break;
        //RES 1,C
        case 0x89: cpu->c &= ~0x02; break;
        //RES 1,D
        case 0x8A: cpu->d &= ~0x02; break;
        //RES 1,E
        case 0x8B: cpu->e &= ~0x02; break;
        //RES 1,H
        case 0x8C: cpu->h &= ~0x02; break;
        //RES 1,L
        case 0x8D: cpu->l &= ~0x02; break;
        //RES 1,[HL]
        case 0x8E: gb_cpu_res_phl(cpu,0x02); break;
        //RES 1,A
        case 0x8F: cpu->a &= ~0x02; break;

        //RES 2,B
        case 0x90: cpu->b &= ~0x04; break;
        //RES 2,C
        case 0x91: cpu->c &= ~0x04; break;
        //RES 2,D
        case 0x92: cpu->d &= ~0x04; break;
        //RES 2,E
        case 0x93: cpu->e &= ~0x04; break;
        //RES 2,H
        case 0x94: cpu->h &= ~0x04; break;
        //RES 2,L
        case 0x95: cpu->l &= ~0x04; break;
        //RES 2,[HL]
        case 0x96: gb_cpu_res_phl(cpu,0x04); break;
        //RES 2,A
        case 0x97: cpu->a &= ~0x04; break;
        //RES 3,B
        case 0x98: cpu->b &= ~0x08; break;
        //RES 3,C
        case 0x99: cpu->c &= ~0x08; break;
        //RES 3,D
        case 0x9A: cpu->d &= ~0x08; break;
        //RES 3,E
        case 0x9B: cpu->e &= ~0x08; break;
        //RES 3,H
        case 0x9C: cpu->h &= ~0x08; break;
        //RES 3,L
        case 0x9D: cpu->l &= ~0x08; break;
        //RES 3,[HL]
        case 0x9E: gb_cpu_res_phl(cpu,0x08); break;
        //RES 3,A
        case 0x9F: cpu->a &= ~0x08; break;

        //RES 4,B
        case 0xA0: cpu->b &= ~0x10; break;
        //RES 4,C
        case 0xA1: cpu->c &= ~0x10; break;
        //RES 4,D
        case 0xA2: cpu->d &= ~0x10; break;
        //RES 4,E
        case 0xA3: cpu->e &= ~0x10; break;
        //RES 4,H
        case 0xA4: cpu->h &= ~0x10; break;
        //RES 4,L
        case 0xA5: cpu->l &= ~0x10; break;
        //RES 4,[HL]
        case 0xA6: gb_cpu_res_phl(cpu,0x10); break;
        //RES 4,A
        case 0xA7: cpu->a &= ~0x10; break;
        //RES 5,B
        case 0xA8: cpu->b &= ~0x20; break;
        //RES 5,C
        case 0xA9: cpu->c &= ~0x20; break;
        //RES 5,D
        case 0xAA: cpu->d &= ~0x20; break;
        //RES 5,E
        case 0xAB: cpu->e &= ~0x20; break;
        //RES 5,H
        case 0xAC: cpu->h &= ~0x20; break;
        //RES 5,L
        case 0xAD: cpu->l &= ~0x20; break;
        //RES 5,[HL]
        case 0xAE: gb_cpu_res_phl(cpu,0x20); break;
        //RES 5,A
        case 0xAF: cpu->a &= ~0x20; break;

        //RES 6,B
        case 0xB0: cpu->b &= ~0x40; break;
        //RES 6,C
        case 0xB1: cpu->c &= ~0x40; break;
        //RES 6,D
        case 0xB2: cpu->d &= ~0x40; break;
        //RES 6,E
        case 0xB3: cpu->e &= ~0x40; break;
        //RES 6,H
        case 0xB4: cpu->h &= ~0x40; break;
        //RES 6,L
        case 0xB5: cpu->l &= ~0x40; break;
        //RES 6,[HL]
        case 0xB6: gb_cpu_res_phl(cpu,0x40); break;
        //RES 6,A
        case 0xB7: cpu->a &= ~0x40; break;
        //RES 7,B
        case 0xB8: cpu->b &= ~0x80; break;
        //RES 7,C
        case 0xB9: cpu->c &= ~0x80; break;
        //RES 7,D
        case 0xBA: cpu->d &= ~0x80; break;
        //RES 7,E
        case 0xBB: cpu->e &= ~0x80; break;
        //RES 7,H
        case 0xBC: cpu->h &= ~0x80; break;
        //RES 7,L
        case 0xBD: cpu->l &= ~0x80; break;
        //RES 7,[HL]
        case 0xBE: gb_cpu_res_phl(cpu,0x80); break;
        //RES 7,A
        case 0xBF: cpu->a &= ~0x80; break;
        
        //SET 0,B
        case 0xC0: cpu->b |= 0x01; break;
        //SET 0,C
        case 0xC1: cpu->c |= 0x01; break;
        //SET 0,D  
        case 0xC2: cpu->d |= 0x01; break;
        //SET 0,E
        case 0xC3: cpu->e |= 0x01; break;
        //SET 0,H
        case 0xC4: cpu->h |= 0x01; break;
        //SET 0,L
        case 0xC5: cpu->l |= 0x01; break;
        //SET 0,[HL]  
        case 0xC6: gb_cpu_set_phl(cpu,0x01); break;
        //SET 0,A
        case 0xC7: cpu->a |= 0x01; break;
        //SET 1,B
        case 0xC8: cpu->b |= 0x02; break;
        //SET 1,C
        case 0xC9: cpu->c |= 0x02; break;
        //SET 1,D
        case 0xCA: cpu->d |= 0x02; break;
        //SET 1,E
        case 0xCB: cpu->e |= 0x02; break;
        //SET 1,H
        case 0xCC: cpu->h |= 0x02; break;
        //SET 1,L
        case 0xCD: cpu->l |= 0x02; break;
        //SET 1,[HL]
        case 0xCE: gb_cpu_set_phl(cpu,0x02); break;
        //SET 1,A
        case 0xCF: cpu->a |= 0x02; break;

        //SET 2,B
        case 0xD0: cpu->b |= 0x04; break;
        //SET 2,C
        case 0xD1: cpu->c |= 0x04; break;
        //SET 2,D  
        case 0xD2: cpu->d |= 0x04; break;
        //SET 2,E
        case 0xD3: cpu->e |= 0x04; break;
        //SET 2,H
        case 0xD4: cpu->h |= 0x04; break;
        //SET 2,L
        case 0xD5: cpu->l |= 0x04; break;
        //SET 2,[HL]  
        case 0xD6: gb_cpu_set_phl(cpu,0x04); break;
        //SET 2,A
        case 0xD7: cpu->a |= 0x04; break;
        //SET 3,B
        case 0xD8: cpu->b |= 0x08; break;
        //SET 3,C
        case 0xD9: cpu->c |= 0x08; break;
        //SET 3,D
        case 0xDA: cpu->d |= 0x08; break;
        //SET 3,E
        case 0xDB: cpu->e |= 0x08; break;
        //SET 3,H
        case 0xDC: cpu->h |= 0x08; break;
        //SET 3,L
        case 0xDD: cpu->l |= 0x08; break;
        //SET 3,[HL]
        case 0xDE: gb_cpu_set_phl(cpu,0x08); break;
        //SET 3,A
        case 0xDF: cpu->a |= 0x08; break;

        //SET 4,B
        case 0xE0: cpu->b |= 0x10; break;
        //SET 4,C
        case 0xE1: cpu->c |= 0x10; break;
        //SET 4,D  
        case 0xE2: cpu->d |= 0x10; break;
        //SET 4,E
        case 0xE3: cpu->e |= 0x10; break;
        //SET 4,H
        case 0xE4: cpu->h |= 0x10; break;
        //SET 4,L
        case 0xE5: cpu->l |= 0x10; break;
        //SET 4,[HL]  
        case 0xE6: gb_cpu_set_phl(cpu,0x10); break;
        //SET 4,A
        case 0xE7: cpu->a |= 0x10; break;
        //SET 5,B
        case 0xE8: cpu->b |= 0x20; break;
        //SET 5,C
        case 0xE9: cpu->c |= 0x20; break;
        //SET 5,D
        case 0xEA: cpu->d |= 0x20; break;
        //SET 5,E
        case 0xEB: cpu->e |= 0x20; break;
        //SET 5,H
        case 0xEC: cpu->h |= 0x20; break;
        //SET 5,L
        case 0xED: cpu->l |= 0x20; break;
        //SET 5,[HL]
        case 0xEE: gb_cpu_set_phl(cpu,0x20); break;
        //SET 5,A
        case 0xEF: cpu->a |= 0x20; break;

        //SET 6,B
        case 0xF0: cpu->b |= 0x40; break;
        //SET 6,C
        case 0xF1: cpu->c |= 0x40; break;
        //SET 6,D  
        case 0xF2: cpu->d |= 0x40; break;
        //SET 6,E
        case 0xF3: cpu->e |= 0x40; break;
        //SET 6,H
        case 0xF4: cpu->h |= 0x40; break;
        //SET 6,L
        case 0xF5: cpu->l |= 0x40; break;
        //SET 6,[HL]  
        case 0xF6: gb_cpu_set_phl(cpu,0x40); break;
        //SET 6,A
        case 0xF7: cpu->a |= 0x40; break;
        //SET 7,B
        case 0xF8: cpu->b |= 0x80; break;
        //SET 7,C
        case 0xF9: cpu->c |= 0x80; break;
        //SET 7,D
        case 0xFA: cpu->d |= 0x80; break;
        //SET 7,E
        case 0xFB: cpu->e |= 0x80; break;
        //SET 7,H
        case 0xFC: cpu->h |= 0x80; break;
        //SET 7,L
        case 0xFD: cpu->l |= 0x80; break;
        //SET 7,[HL]
        case 0xFE: gb_cpu_set_phl(cpu,0x80); break;
        //SET 7,A
        case 0xFF: cpu->a |= 0x80; break;
    }
}


void gb_cpu_execute_opcode(gb_cpu_t* cpu){
    switch(cpu->opcode){
        //NOP
        case 0x00: break;
        //LD BC,IMM16
        case 0x01: gb_cpu_ld_r16_imm16(cpu,&cpu->bc); break;
        //LD [BC],A
        case 0x02: gb_cpu_write_byte(cpu,cpu->a,cpu->bc); break;
        //INC BC
        case 0x03: gb_cpu_inc_word(cpu,&cpu->bc); break;
        //INC B
        case 0x04: gb_cpu_inc_byte(cpu,&cpu->b); break;
        //DEC B
        case 0x05: gb_cpu_dec_byte(cpu,&cpu->b); break;
        //LD B,IMM8
        case 0x06: cpu->b = gb_cpu_read_byte(cpu,cpu->pc++);; break;
        //RLCA
        case 0x07: gb_cpu_rlca(cpu); break;
        //LD [IMM16],SP
        case 0x08: gb_cpu_ld_pimm16_word(cpu,cpu->sp); break;
        //ADD HL,BC
        case 0x09: gb_cpu_add_hl_word(cpu,cpu->bc); break;
        //LD A,[BC]
        case 0x0A: cpu->a = gb_cpu_read_byte(cpu,cpu->bc); break;
        //DEC BC
        case 0x0B: gb_cpu_dec_word(cpu,&cpu->bc); break;
        //INC C
        case 0x0C: gb_cpu_inc_byte(cpu,&cpu->c); break;
        //DEC C
        case 0x0D: gb_cpu_dec_byte(cpu,&cpu->c); break;
        //LD C,IMM8
        case 0x0E: cpu->c = gb_cpu_read_byte(cpu,cpu->pc++); break;
        //RRCA
        case 0x0F: gb_cpu_rrca(cpu); break;

        //STOP
        case 0x10: printf("STOP Instruction\n"); break;
        //LD DE,IMM16
        case 0x11: gb_cpu_ld_r16_imm16(cpu,&cpu->de); break;
        //LD [DE],A
        case 0x12: gb_cpu_write_byte(cpu,cpu->a,cpu->de); break;
        //INC DE
        case 0x13: gb_cpu_inc_word(cpu,&cpu->de); break;
        //INC D
        case 0x14: gb_cpu_inc_byte(cpu,&cpu->d); break;
        //DEC D
        case 0x15: gb_cpu_dec_byte(cpu,&cpu->d); break;
        //LD D,IMM8
        case 0x16: cpu->d = gb_cpu_read_byte(cpu,cpu->pc++); break;
        //RLA
        case 0x17: gb_cpu_rla(cpu); break;
        //JR IMM8
        case 0x18: gb_cpu_jr_imm8(cpu,true); break;
        //ADD HL,DE
        case 0x19: gb_cpu_add_hl_word(cpu,cpu->de); break;
        //LD A,[DE]
        case 0x1A: cpu->a = gb_cpu_read_byte(cpu,cpu->de); break;
        //DEC DE
        case 0x1B: gb_cpu_dec_word(cpu,&cpu->de); break;
        //INC E
        case 0x1C: gb_cpu_inc_byte(cpu,&cpu->e); break;
        //DEC E
        case 0x1D: gb_cpu_dec_byte(cpu,&cpu->e); break;
        //LD E,IMM8
        case 0x1E: cpu->e = gb_cpu_read_byte(cpu,cpu->pc++); break;
        //RRA
        case 0x1F: gb_cpu_rra(cpu); break;

        //JR NZ,IMM8
        case 0x20: gb_cpu_jr_imm8(cpu,!(cpu->f & gb_cpu_zero_flag)); break;
        //LD HL,IMM16
        case 0x21: gb_cpu_ld_r16_imm16(cpu,&cpu->hl); break;
        //LD [HL+],A
        case 0x22: gb_cpu_write_byte(cpu,cpu->a,cpu->hl++); break;
        //INC HL
        case 0x23: gb_cpu_inc_word(cpu,&cpu->hl); break;
        //INC H
        case 0x24: gb_cpu_inc_byte(cpu,&cpu->h); break;
        //DEC H
        case 0x25: gb_cpu_dec_byte(cpu,&cpu->h); break;
        //LD H,IMM8
        case 0x26: cpu->h = gb_cpu_read_byte(cpu,cpu->pc++); break;
        //DAA
        case 0x27: gb_cpu_daa(cpu); break;
        //JR Z,IMM8
        case 0x28: gb_cpu_jr_imm8(cpu,cpu->f & gb_cpu_zero_flag); break;
        //ADD HL,HL
        case 0x29: gb_cpu_add_hl_word(cpu,cpu->hl); break;
        //LD A,[HL+]
        case 0x2A: cpu->a = gb_cpu_read_byte(cpu,cpu->hl++); break;
        //DEC HL
        case 0x2B: gb_cpu_dec_word(cpu,&cpu->hl); break;
        //INC L
        case 0x2C: gb_cpu_inc_byte(cpu,&cpu->l); break;
        //DEC L
        case 0x2D: gb_cpu_dec_byte(cpu,&cpu->l); break;
        //LD L,IMM8
        case 0x2E: cpu->l = gb_cpu_read_byte(cpu,cpu->pc++); break;
        //CPL
        case 0x2F: gb_cpu_cpl(cpu); break;

        //JR NC,IMM8
        case 0x30: gb_cpu_jr_imm8(cpu,!(cpu->f & gb_cpu_carry_flag)); break;
        //LD SP,IMM16
        case 0x31: gb_cpu_ld_r16_imm16(cpu,&cpu->sp); break;
        //LD [HL-],A
        case 0x32: gb_cpu_write_byte(cpu,cpu->a,cpu->hl--); break;
        //INC SP
        case 0x33: gb_cpu_inc_word(cpu,&cpu->sp); break;
        //INC [HL]
        case 0x34: gb_cpu_inc_phl(cpu); break;
        //DEC [HL]
        case 0x35: gb_cpu_dec_phl(cpu); break;
        //LD [HL],IMM8
        case 0x36: gb_cpu_write_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++),cpu->hl); break;
        //SCF
        case 0x37: gb_cpu_scf(cpu); break;
        //JR C,IMM8
        case 0x38: gb_cpu_jr_imm8(cpu,cpu->f & gb_cpu_carry_flag); break;
        //ADD HL,SP
        case 0x39: gb_cpu_add_hl_word(cpu,cpu->sp); break;
        //LD A,[HL-]
        case 0x3A: cpu->a = gb_cpu_read_byte(cpu,cpu->hl--); break;
        //DEC SP
        case 0x3B: gb_cpu_dec_word(cpu,&cpu->sp); break;
        //INC A
        case 0x3C: gb_cpu_inc_byte(cpu,&cpu->a); break;
        //DEC A
        case 0x3D: gb_cpu_dec_byte(cpu,&cpu->a); break;
        //LD A,IMM8
        case 0x3E: cpu->a = gb_cpu_read_byte(cpu,cpu->pc++); break;
        //CCF
        case 0x3F: gb_cpu_ccf(cpu); break;
        
        //LD B,B
        case 0x40: break;
        //LD B,C
        case 0x41: cpu->b = cpu->c; break;
        //LD B,D
        case 0x42: cpu->b = cpu->d; break;
        //LD B,E
        case 0x43: cpu->b = cpu->e; break;
        //LD B,H
        case 0x44: cpu->b = cpu->h; break;
        //LD B,L
        case 0x45: cpu->b = cpu->l; break;
        //LD B,[HL]
        case 0x46: cpu->b = gb_cpu_read_byte(cpu,cpu->hl); break;
        //LD B,A
        case 0x47: cpu->b = cpu->a; break;
        //LD C,B
        case 0x48: cpu->c = cpu->b; break;
        //LD C,C
        case 0x49: break;
        //LD C,D
        case 0x4A: cpu->c = cpu->d; break;
        //LD C,E
        case 0x4B: cpu->c = cpu->e; break;
        //LD C,H
        case 0x4C: cpu->c = cpu->h; break;
        //LD C,L
        case 0x4D: cpu->c = cpu->l; break;
        //LD C,[HL]
        case 0x4E: cpu->c = gb_cpu_read_byte(cpu,cpu->hl); break;
        //LD C,A
        case 0x4F: cpu->c = cpu->a; break;

        //LD D,B
        case 0x50: cpu->d = cpu->b; break;
        //LD D,C
        case 0x51: cpu->d = cpu->c; break;
        //LD D,D
        case 0x52: break;
        //LD D,E
        case 0x53: cpu->d = cpu->e; break;
        //LD D,H
        case 0x54: cpu->d = cpu->h; break;
        //LD D,L
        case 0x55: cpu->d = cpu->l; break;
        //LD D,[HL]
        case 0x56: cpu->d = gb_cpu_read_byte(cpu,cpu->hl); break;
        //LD D,A
        case 0x57: cpu->d = cpu->a; break;
        //LD E,B
        case 0x58: cpu->e = cpu->b; break;
        //LD E,C
        case 0x59: cpu->e = cpu->c; break;
        //LD E,D
        case 0x5A: cpu->e = cpu->d; break;
        //LD E,E
        case 0x5B: break;
        //LD E,H
        case 0x5C: cpu->e = cpu->h; break;
        //LD E,L
        case 0x5D: cpu->e = cpu->l; break;
        //LD E,[HL]
        case 0x5E: cpu->e = gb_cpu_read_byte(cpu,cpu->hl); break;
        //LD E,A
        case 0x5F: cpu->e = cpu->a; break;

        //LD H,B
        case 0x60: cpu->h = cpu->b; break;
        //LD H,C
        case 0x61: cpu->h = cpu->c; break;
        //LD H,D
        case 0x62: cpu->h = cpu->d; break;
        //LD H,E
        case 0x63: cpu->h = cpu->e; break;
        //LD H,H
        case 0x64: break;
        //LD H,L
        case 0x65: cpu->h = cpu->l; break;
        //LD H,[HL]
        case 0x66: cpu->h = gb_cpu_read_byte(cpu,cpu->hl); break;
        //LD H,A
        case 0x67: cpu->h = cpu->a; break;
        //LD L,B
        case 0x68: cpu->l = cpu->b; break;
        //LD L,C
        case 0x69: cpu->l = cpu->c; break;
        //LD L,D
        case 0x6A: cpu->l = cpu->d; break;
        //LD L,E
        case 0x6B: cpu->l = cpu->e; break;
        //LD L,H
        case 0x6C: cpu->l = cpu->h; break;
        //LD L,L
        case 0x6D: break;
        //LD L,[HL]
        case 0x6E: cpu->l = gb_cpu_read_byte(cpu,cpu->hl); break;
        //LD L,A
        case 0x6F: cpu->l = cpu->a; break;

        //LD [HL],B
        case 0x70: gb_cpu_write_byte(cpu,cpu->b,cpu->hl); break;
        //LD [HL],C
        case 0x71: gb_cpu_write_byte(cpu,cpu->c,cpu->hl); break;
        //LD [HL],D
        case 0x72: gb_cpu_write_byte(cpu,cpu->d,cpu->hl); break;
        //LD [HL],E
        case 0x73: gb_cpu_write_byte(cpu,cpu->e,cpu->hl); break;
        //LD [HL],H
        case 0x74: gb_cpu_write_byte(cpu,cpu->h,cpu->hl); break;
        //LD [HL],L
        case 0x75: gb_cpu_write_byte(cpu,cpu->l,cpu->hl); break;
        //HALT
        case 0x76: gb_cpu_halt(cpu); break;
        //LD [HL],A
        case 0x77: gb_cpu_write_byte(cpu,cpu->a,cpu->hl); break;
        //LD A,B
        case 0x78: cpu->a = cpu->b; break;
        //LD A,C
        case 0x79: cpu->a = cpu->c; break;
        //LD A,D
        case 0x7A: cpu->a = cpu->d; break;
        //LD A,E
        case 0x7B: cpu->a = cpu->e; break;
        //LD A,H
        case 0x7C: cpu->a = cpu->h; break;
        //LD A,L
        case 0x7D: cpu->a = cpu->l; break;
        //LD A,[HL]
        case 0x7E: cpu->a = gb_cpu_read_byte(cpu,cpu->hl); break;
        //LD A,A
        case 0x7F: break;

        //ADD A,B
        case 0x80: gb_cpu_add_byte(cpu,cpu->b); break;
        //ADD A,C
        case 0x81: gb_cpu_add_byte(cpu,cpu->c); break;
        //ADD A,D
        case 0x82: gb_cpu_add_byte(cpu,cpu->d); break;
        //ADD A,E
        case 0x83: gb_cpu_add_byte(cpu,cpu->e); break;
        //ADD A,H
        case 0x84: gb_cpu_add_byte(cpu,cpu->h); break;
        //ADD A,L
        case 0x85: gb_cpu_add_byte(cpu,cpu->l); break;
        //ADD A,[HL]
        case 0x86: gb_cpu_add_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //ADD A,A
        case 0x87: gb_cpu_add_byte(cpu,cpu->a); break;
        //ADC A,B
        case 0x88: gb_cpu_adc_byte(cpu,cpu->b); break;
        //ADC A,C
        case 0x89: gb_cpu_adc_byte(cpu,cpu->c); break;
        //ADC A,D
        case 0x8A: gb_cpu_adc_byte(cpu,cpu->d); break;
        //ADC A,E
        case 0x8B: gb_cpu_adc_byte(cpu,cpu->e); break;
        //ADC A,H
        case 0x8C: gb_cpu_adc_byte(cpu,cpu->h); break;
        //ADC A,L
        case 0x8D: gb_cpu_adc_byte(cpu,cpu->l); break;
        //ADC A,[HL]
        case 0x8E: gb_cpu_adc_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //ADC A,A
        case 0x8F: gb_cpu_adc_byte(cpu,cpu->a); break;

        //SUB A,B
        case 0x90: gb_cpu_sub_byte(cpu,cpu->b); break;
        //SUB A,C
        case 0x91: gb_cpu_sub_byte(cpu,cpu->c); break;
        //SUB A,D
        case 0x92: gb_cpu_sub_byte(cpu,cpu->d); break;
        //SUB A,E
        case 0x93: gb_cpu_sub_byte(cpu,cpu->e); break;
        //SUB A,H
        case 0x94: gb_cpu_sub_byte(cpu,cpu->h); break;
        //SUB A,L
        case 0x95: gb_cpu_sub_byte(cpu,cpu->l); break;
        //SUB A,[HL]
        case 0x96: gb_cpu_sub_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //SUB A,A
        case 0x97: gb_cpu_sub_byte(cpu,cpu->a); break;
        //SBC A,B
        case 0x98: gb_cpu_sbc_byte(cpu,cpu->b); break;
        //SBC A,C
        case 0x99: gb_cpu_sbc_byte(cpu,cpu->c); break;
        //SBC A,D
        case 0x9A: gb_cpu_sbc_byte(cpu,cpu->d); break;
        //SBC A,E
        case 0x9B: gb_cpu_sbc_byte(cpu,cpu->e); break;
        //SBC A,H
        case 0x9C: gb_cpu_sbc_byte(cpu,cpu->h); break;
        //SBC A,L
        case 0x9D: gb_cpu_sbc_byte(cpu,cpu->l); break;
        //SBC A,[HL]
        case 0x9E: gb_cpu_sbc_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //SBC A,A
        case 0x9F: gb_cpu_sbc_byte(cpu,cpu->a); break;

        //AND A,B
        case 0xA0: gb_cpu_and_byte(cpu,cpu->b); break;
        //AND A,C
        case 0xA1: gb_cpu_and_byte(cpu,cpu->c); break;
        //AND A,D
        case 0xA2: gb_cpu_and_byte(cpu,cpu->d); break;
        //AND A,E
        case 0xA3: gb_cpu_and_byte(cpu,cpu->e); break;
        //AND A,H
        case 0xA4: gb_cpu_and_byte(cpu,cpu->h); break;
        //AND A,L
        case 0xA5: gb_cpu_and_byte(cpu,cpu->l); break;
        //AND A,[HL]
        case 0xA6: gb_cpu_and_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //AND A,A
        case 0xA7: gb_cpu_and_byte(cpu,cpu->a); break;
        //XOR A,B
        case 0xA8: gb_cpu_xor_byte(cpu,cpu->b); break;
        //XOR,A,C
        case 0xA9: gb_cpu_xor_byte(cpu,cpu->c); break;
        //XOR A,D
        case 0xAA: gb_cpu_xor_byte(cpu,cpu->d); break;
        //XOR A,E
        case 0xAB: gb_cpu_xor_byte(cpu,cpu->e); break;
        //XOR A,H
        case 0xAC: gb_cpu_xor_byte(cpu,cpu->h); break;
        //XOR A,L
        case 0xAD: gb_cpu_xor_byte(cpu,cpu->l); break;
        //XOR A,[HL]
        case 0xAE: gb_cpu_xor_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //XOR A,A
        case 0xAF: gb_cpu_xor_byte(cpu,cpu->a); break;

        //OR A,B
        case 0xB0: gb_cpu_or_byte(cpu,cpu->b); break;
        //OR A,C
        case 0xB1: gb_cpu_or_byte(cpu,cpu->c); break;
        //OR A,D
        case 0xB2: gb_cpu_or_byte(cpu,cpu->d); break;
        //OR A,E
        case 0xB3: gb_cpu_or_byte(cpu,cpu->e); break;
        //OR A,H
        case 0xB4: gb_cpu_or_byte(cpu,cpu->h); break;
        //OR A,L
        case 0xB5: gb_cpu_or_byte(cpu,cpu->l); break;
        //OR,A,[HL]
        case 0xB6: gb_cpu_or_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //OR A,A
        case 0xB7: gb_cpu_or_byte(cpu,cpu->a); break;
        //CP A,B
        case 0xB8: gb_cpu_cp_byte(cpu,cpu->b); break;
        //CP A,C
        case 0xB9: gb_cpu_cp_byte(cpu,cpu->c); break;
        //CP A,D
        case 0xBA: gb_cpu_cp_byte(cpu,cpu->d); break;
        //CP A,E
        case 0xBB: gb_cpu_cp_byte(cpu,cpu->e); break;
        //CP A,H
        case 0xBC: gb_cpu_cp_byte(cpu,cpu->h); break;
        //CP A,L
        case 0xBD: gb_cpu_cp_byte(cpu,cpu->l); break;
        //CP A,[HL]
        case 0xBE: gb_cpu_cp_byte(cpu,gb_cpu_read_byte(cpu,cpu->hl)); break;
        //CP A,A
        case 0xBF: gb_cpu_cp_byte(cpu,cpu->a); break;

        //RET NZ
        case 0xC0: gb_cpu_ret_cc(cpu,!(cpu->f & gb_cpu_zero_flag)); break;
        //POP BC
        case 0xC1: gb_cpu_pop_word(cpu,&cpu->bc); break;
        //JP NZ,IMM16
        case 0xC2: gb_cpu_jp_imm16(cpu,!(cpu->f & gb_cpu_zero_flag)); break;
        //JP IMM16
        case 0xC3: gb_cpu_jp_imm16(cpu,true); break;
        //CALL NZ,IMM16
        case 0xC4: gb_cpu_call_imm16(cpu,!(cpu->f & gb_cpu_zero_flag)); break;
        //PUSH BC
        case 0xC5: gb_cpu_push_word(cpu,cpu->bc); break;
        //ADD A,IMM8
        case 0xC6: gb_cpu_add_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $00
        case 0xC7: gb_cpu_rst(cpu,0x00); break;
        //RET Z
        case 0xC8: gb_cpu_ret_cc(cpu,cpu->f & gb_cpu_zero_flag); break;
        //RET
        case 0xC9: gb_cpu_ret(cpu); break;
        //JP Z,IMM16
        case 0xCA: gb_cpu_jp_imm16(cpu,cpu->f & gb_cpu_zero_flag); break;
        //PREFIX
        case 0xCB: gb_cpu_prefix(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //CALL Z,IMM16
        case 0xCC: gb_cpu_call_imm16(cpu,cpu->f & gb_cpu_zero_flag); break;
        //CALL IMM16
        case 0xCD: gb_cpu_call_imm16(cpu,true); break;
        //ADC A,IMM8
        case 0xCE: gb_cpu_adc_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $08
        case 0xCF: gb_cpu_rst(cpu,0x08); break;

        //RET NC
        case 0xD0: gb_cpu_ret_cc(cpu,!(cpu->f & gb_cpu_carry_flag)); break;
        //POP DE
        case 0xD1: gb_cpu_pop_word(cpu,&cpu->de); break;
        //JP NC,IMM16
        case 0xD2: gb_cpu_jp_imm16(cpu,!(cpu->f & gb_cpu_carry_flag)); break;
        //Undefined
        case 0xD3: break;
        //CALL NC,IMM16
        case 0xD4: gb_cpu_call_imm16(cpu,!(cpu->f & gb_cpu_carry_flag)); break;
        //PUSH DE
        case 0xD5: gb_cpu_push_word(cpu,cpu->de); break;
        //SUB A,IMM8
        case 0xD6: gb_cpu_sub_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $10
        case 0xD7: gb_cpu_rst(cpu,0x10); break;
        //RET C
        case 0xD8: gb_cpu_ret_cc(cpu,cpu->f & gb_cpu_carry_flag); break;
        //RETI
        case 0xD9: gb_cpu_reti(cpu); break;
        //JP C,IMM16
        case 0xDA: gb_cpu_jp_imm16(cpu,cpu->f & gb_cpu_carry_flag); break;
        //Undefined
        case 0xDB: break;
        //CALL C,IMM16
        case 0xDC: gb_cpu_call_imm16(cpu,cpu->f & gb_cpu_carry_flag); break;
        //Undefined
        case 0xDD: break;
        //SBC A,IMM8
        case 0xDE: gb_cpu_sbc_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $18
        case 0xDF: gb_cpu_rst(cpu,0x18); break;

        //LDH [IMM8],A
        case 0xE0: gb_cpu_write_byte(cpu,cpu->a,0xFF00 | gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //POP HL
        case 0xE1: gb_cpu_pop_word(cpu,&cpu->hl); break;
        //LDH [C],A
        case 0xE2: gb_cpu_write_byte(cpu,cpu->a,0xFF00 | cpu->c); break;
        //Undefined
        case 0xE3: break;
        //Undefined
        case 0xE4: break;
        //PUSH HL
        case 0xE5: gb_cpu_push_word(cpu,cpu->hl); break;
        //AND A,IMM8
        case 0xE6: gb_cpu_and_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $20
        case 0xE7: gb_cpu_rst(cpu,0x20); break;
        //ADD SP,IMM8
        case 0xE8: gb_cpu_add_sp_imm8(cpu); break;
        //JP HL
        case 0xE9: cpu->pc = cpu->hl; break;
        //LD [IMM16],A
        case 0xEA: gb_cpu_ld_pimm16_byte(cpu,cpu->a); break;
        //Undefined
        case 0xEB: break;
        //Undefined
        case 0xEC: break;
        //Unedefined
        case 0xED: break;
        //XOR A,IMM8
        case 0xEE: gb_cpu_xor_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $28
        case 0xEF: gb_cpu_rst(cpu,0x28); break;

        //LDH A,[IMM8]
        case 0xF0: cpu->a = gb_cpu_read_byte(cpu,0xFF00 | gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //POP AF
        case 0xF1: gb_cpu_pop_af(cpu); break;
        //LDH A,[C]
        case 0xF2: cpu->a = gb_cpu_read_byte(cpu,0xFF00 | cpu->c); break;
        //DI
        case 0xF3: cpu->ime = false; break;
        //Undefined
        case 0xF4: break;
        //PUSH AF
        case 0xF5: gb_cpu_push_af(cpu); break;
        //OR A,IMM8
        case 0xF6: gb_cpu_or_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $30
        case 0xF7: gb_cpu_rst(cpu,0x30); break;
        //LD HL,SP+IMM8
        case 0xF8: gb_cpu_ld_hl_sp_plus_imm8(cpu); break;
        //LD SP,HL
        case 0xF9: gb_cpu_ld_sp_hl(cpu); break;
        //LD A,[IMM16]
        case 0xFA: gb_cpu_ld_a_pimm16(cpu); break;
        //EI
        case 0xFB: cpu->ime_pending = true; break;
        //Undefined
        case 0xFC: break;
        //Undefined
        case 0xFD: break;
        //CP A,IMM8
        case 0xFE: gb_cpu_cp_byte(cpu,gb_cpu_read_byte(cpu,cpu->pc++)); break;
        //RST $38
        case 0xFF: gb_cpu_rst(cpu,0x38); break;
    }
}


void gb_cpu_execute(gb_cpu_t* cpu){
    if(!cpu->halted){
        if(cpu->ime && (cpu->gb->interrupt.enable & cpu->gb->interrupt.flag)){
            cpu->pc--;
            
            gb_cpu_cycle(cpu);
            gb_cpu_cycle(cpu);

            gb_cpu_write_byte(cpu,cpu->pc >> 0x08,--cpu->sp);
            
            uint8_t vector = gb_interrupt_get_vector(&cpu->gb->interrupt);
            
            gb_cpu_write_byte(cpu,cpu->pc & 0xFF,--cpu->sp);

            cpu->pc = vector;
            
            cpu->ime = false;
        }
        else{
            if(cpu->ime_pending){
                cpu->ime_pending = false;
                cpu->ime = true;
            }
            gb_cpu_execute_opcode(cpu);
        }

        cpu->opcode = gb_cpu_read_byte(cpu,cpu->pc);

        if(!cpu->halt_fetch){
            cpu->pc++;
        }
        else{
            cpu->halt_fetch = false;
        }
    }
    else{
        gb_master_clock(cpu->gb);
        gb_master_clock(cpu->gb);
        gb_master_clock(cpu->gb);
        gb_master_clock(cpu->gb);

        if(cpu->gb->interrupt.enable & cpu->gb->interrupt.flag){
            cpu->halted = false;
            cpu->opcode = gb_memory_read(&cpu->gb->memory,cpu->pc++);
        }
    }
}


void gb_cpu_reset(gb_cpu_t* cpu){
    cpu->opcode = 0x00;

    cpu->halted = false;
    cpu->halt_fetch = false;

    cpu->ime_pending = false;
    cpu->ime = false;

    cpu->af = 0x00;
    cpu->bc = 0x00;
    cpu->de = 0x00;
    cpu->hl = 0x00;
    cpu->sp = 0x00;
    cpu->pc = 0x00;
}