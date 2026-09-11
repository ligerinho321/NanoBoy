#include "cpu.h"
#include "gb.h"

const char* gb_cpu_mode_names[3] = {
    "Running",
    "Halted",
    "Stopped"
};

#define gb_cpu_set_flag(flag,condition) (condition) ? (cpu->state.f |= (flag)) : (cpu->state.f &= ~(flag))

#define gb_cpu_irq_pending(cpu) ((cpu)->gb->interrupt.state.enable & (cpu)->gb->interrupt.state.flag)

#define gb_cpu_cycle(cpu) gb_machine_cycle((cpu)->gb)


void gb_cpu_init(gb_cpu_t* cpu,gb_t* gb){
    cpu->gb = gb;
}


void gb_cpu_write_byte(gb_cpu_t* cpu,uint8_t value,uint16_t address){
    gb_cpu_cycle(cpu);
    gb_memory_cpu_write(&cpu->gb->memory,value,address);
}

uint8_t gb_cpu_read_byte(gb_cpu_t* cpu,uint16_t address){
    gb_cpu_cycle(cpu);
    return gb_memory_cpu_read(&cpu->gb->memory,address);
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


static inline void gb_cpu_ld_r16_imm16(gb_cpu_t* cpu,uint16_t* word){
    *word = gb_cpu_read_word(cpu,cpu->state.pc);
    cpu->state.pc += 2;
}

static inline void gb_cpu_ld_pimm16_byte(gb_cpu_t* cpu,uint8_t byte){
    uint16_t address = gb_cpu_read_word(cpu,cpu->state.pc);
    cpu->state.pc += 2;
    gb_cpu_write_byte(cpu,byte,address);
}

static inline void gb_cpu_ld_pimm16_word(gb_cpu_t* cpu,uint16_t word){
    uint16_t address = gb_cpu_read_word(cpu,cpu->state.pc);
    cpu->state.pc += 2;
    gb_cpu_write_word(cpu,word,address);
}

static inline void gb_cpu_ld_a_pimm16(gb_cpu_t* cpu){
    uint16_t address = gb_cpu_read_word(cpu,cpu->state.pc);
    cpu->state.pc += 2;
    cpu->state.a = gb_cpu_read_byte(cpu,address);
}

static inline void gb_cpu_ld_sp_hl(gb_cpu_t* cpu){
    cpu->state.sp = cpu->state.hl;
    gb_cpu_cycle(cpu);
}


static inline void gb_cpu_inc_byte(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(*byte & 0x0F) == 0x0F);
    *byte += 1;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
}

static inline void gb_cpu_inc_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_inc_byte(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_dec_byte(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(*byte & 0x0F) == 0x00);
    *byte -= 1;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
}

static inline void gb_cpu_dec_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_dec_byte(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_inc_word(gb_cpu_t* cpu,uint16_t* word){
    *word += 1;
    gb_cpu_cycle(cpu);
}

static inline void gb_cpu_dec_word(gb_cpu_t* cpu,uint16_t* word){
    *word -= 1;
    gb_cpu_cycle(cpu);
}


static inline void gb_cpu_rlca(gb_cpu_t* cpu){
    uint8_t b7 = cpu->state.a & 0x80;
    cpu->state.a = (cpu->state.a << 0x01) | (b7 ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_znh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

static inline void gb_cpu_rrca(gb_cpu_t* cpu){
    uint8_t b0 = cpu->state.a & 0x01;
    cpu->state.a = (cpu->state.a >> 0x01) | (b0 ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_znh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

static inline void gb_cpu_rla(gb_cpu_t* cpu){
    uint8_t b7 = cpu->state.a & 0x80;
    cpu->state.a = (cpu->state.a << 0x01) | ((cpu->state.f & gb_cpu_carry_flag) ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_znh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

static inline void gb_cpu_rra(gb_cpu_t* cpu){
    uint8_t b0 = cpu->state.a & 0x01;
    cpu->state.a = (cpu->state.a >> 0x01) | ((cpu->state.f & gb_cpu_carry_flag) ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_znh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

static inline void gb_cpu_daa(gb_cpu_t* cpu){
    if(cpu->state.f & gb_cpu_subtraction_flag){
        if(cpu->state.f & gb_cpu_carry_flag){
            cpu->state.a -= 0x60;
        }
        if(cpu->state.f & gb_cpu_half_carry_flag){
            cpu->state.a -= 0x06;
        }
    }
    else{
        if((cpu->state.f & gb_cpu_carry_flag) || cpu->state.a > 0x99){
            cpu->state.a += 0x60;
            gb_cpu_set_flag(gb_cpu_carry_flag,true);
        }        
        if((cpu->state.f & gb_cpu_half_carry_flag) || (cpu->state.a & 0x0F) > 0x09){
            cpu->state.a += 0x06;
        }
    }

    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,false);
}

static inline void gb_cpu_cpl(gb_cpu_t* cpu){
    cpu->state.a = ~cpu->state.a;
    gb_cpu_set_flag(gb_cpu_nh_flag,true);
}

static inline void gb_cpu_scf(gb_cpu_t* cpu){
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,true);
}

static inline void gb_cpu_ccf(gb_cpu_t* cpu){
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,(cpu->state.f & gb_cpu_carry_flag) ? 0x00 : 0x01);
}


static inline void gb_cpu_rlc(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b7 = *byte & 0x80;
    *byte = (*byte << 0x01) |(b7 ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

static inline void gb_cpu_rlc_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_rlc(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_rl(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b7 = *byte & 0x80;
    *byte = (*byte << 0x01) | ((cpu->state.f & gb_cpu_carry_flag) ? 0x01 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b7);
}

static inline void gb_cpu_rl_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_rl(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_rr(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b0 = *byte & 0x01;
    *byte = (*byte >> 0x01) | ((cpu->state.f & gb_cpu_carry_flag) ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

static inline void gb_cpu_rr_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_rr(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_rrc(gb_cpu_t* cpu,uint8_t* byte){
    uint8_t b0 = *byte & 0x01;
    *byte = (*byte >> 0x01) | (b0 ? 0x80 : 0x00);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
    gb_cpu_set_flag(gb_cpu_carry_flag,b0);
}

static inline void gb_cpu_rrc_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_rrc(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_sla(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_carry_flag,*byte & 0x80);
    *byte <<= 0x01;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
}

static inline void gb_cpu_sla_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_sla(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_sra(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_carry_flag,*byte & 0x01);
    *byte = (*byte & 0x80) | (*byte >> 0x01);
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
}

static inline void gb_cpu_sra_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_sra(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_swap(gb_cpu_t* cpu,uint8_t* byte){
    *byte = (*byte << 0x04) | (*byte >> 0x04); 
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nhc_flag,false);
}

static inline void gb_cpu_swap_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_swap(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_srl(gb_cpu_t* cpu,uint8_t* byte){
    gb_cpu_set_flag(gb_cpu_carry_flag,*byte & 0x01);
    *byte >>= 0x01;
    gb_cpu_set_flag(gb_cpu_zero_flag,*byte == 0x00);
    gb_cpu_set_flag(gb_cpu_nh_flag,false);
}

static inline void gb_cpu_srl_phl(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    gb_cpu_srl(cpu,&byte);
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_bit(gb_cpu_t* cpu,bool bit){
    gb_cpu_set_flag(gb_cpu_zero_flag,!bit);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,true);
}

static inline void gb_cpu_res_phl(gb_cpu_t* cpu,uint8_t bit){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    byte &= ~bit;
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}

static inline void gb_cpu_set_phl(gb_cpu_t* cpu,uint8_t bit){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.hl);
    byte |= bit;
    gb_cpu_write_byte(cpu,byte,cpu->state.hl);
}



static inline void gb_cpu_jr_imm8(gb_cpu_t* cpu,bool condition){
    int8_t offset = gb_cpu_read_byte(cpu,cpu->state.pc++);
    if(condition){
        cpu->state.pc += offset;
        gb_cpu_cycle(cpu);
    }
}

static inline void gb_cpu_jp_imm16(gb_cpu_t* cpu,bool condition){
    uint16_t address = gb_cpu_read_word(cpu,cpu->state.pc);
    cpu->state.pc += 2;
    if(condition){
        cpu->state.pc = address;
        gb_cpu_cycle(cpu);
    }
}

static inline void gb_cpu_call_imm16(gb_cpu_t* cpu,bool condition){
    uint16_t address = gb_cpu_read_word(cpu,cpu->state.pc);
    cpu->state.pc += 2;
    if(condition){
        gb_cpu_cycle(cpu);
        gb_cpu_write_byte(cpu,cpu->state.pc >> 0x08,--cpu->state.sp);
        gb_cpu_write_byte(cpu,cpu->state.pc & 0xFF,--cpu->state.sp);
        cpu->state.pc = address;
    }
}


static inline void gb_cpu_add_byte(gb_cpu_t* cpu,uint8_t byte){
    int result = cpu->state.a + byte;
    
    gb_cpu_set_flag(gb_cpu_carry_flag,result > 0xFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->state.a & 0x0F) + (byte & 0x0F) > 0x0F);

    cpu->state.a = result;

    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
}

static inline void gb_cpu_adc_byte(gb_cpu_t* cpu,uint8_t byte){
    uint8_t carry = ((cpu->state.f & gb_cpu_carry_flag) ? 0x01 : 0x00);

    int result = cpu->state.a + byte + carry;

    gb_cpu_set_flag(gb_cpu_carry_flag,result > 0xFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->state.a & 0x0F) + (byte & 0x0F) + carry > 0x0F);

    cpu->state.a = result;

    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
}

static inline void gb_cpu_sub_byte(gb_cpu_t* cpu,uint8_t byte){
    int result = cpu->state.a - byte;

    gb_cpu_set_flag(gb_cpu_carry_flag,result < 0x00);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->state.a & 0x0F) < (byte & 0x0F));

    cpu->state.a = result;

    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
}

static inline void gb_cpu_sbc_byte(gb_cpu_t* cpu,uint8_t byte){
    uint8_t carry = ((cpu->state.f & gb_cpu_carry_flag) ? 0x01 : 0x00);

    int result = cpu->state.a - byte - carry;

    gb_cpu_set_flag(gb_cpu_carry_flag,result < 0x00);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->state.a & 0x0F) < (byte & 0x0F) + carry);

    cpu->state.a = result;

    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
}

static inline void gb_cpu_and_byte(gb_cpu_t* cpu,uint8_t byte){
    cpu->state.a &= byte;
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_carry_flag,false);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,true);
}

static inline void gb_cpu_xor_byte(gb_cpu_t* cpu,uint8_t byte){
    cpu->state.a ^= byte;
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
    gb_cpu_set_flag(gb_cpu_nhc_flag,false);
}

static inline void gb_cpu_or_byte(gb_cpu_t* cpu,uint8_t byte){
    cpu->state.a |= byte;
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == 0x00);
    gb_cpu_set_flag(gb_cpu_nhc_flag,false);
}

static inline void gb_cpu_cp_byte(gb_cpu_t* cpu,uint8_t byte){
    gb_cpu_set_flag(gb_cpu_zero_flag,cpu->state.a == byte);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,true);
    gb_cpu_set_flag(gb_cpu_carry_flag,cpu->state.a < byte);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->state.a & 0x0F) < (byte & 0x0F));
}


static inline uint16_t gb_cpu_sp_plus_imm8(gb_cpu_t* cpu){
    uint8_t byte = gb_cpu_read_byte(cpu,cpu->state.pc++);
    gb_cpu_set_flag(gb_cpu_carry_flag,(cpu->state.sp & 0xFF) + byte > 0xFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->state.sp & 0x0F) + (byte & 0x0F) > 0x0F);
    gb_cpu_set_flag(gb_cpu_subtraction_flag | gb_cpu_zero_flag,false);
    return cpu->state.sp + (int8_t)byte;
}

static inline void gb_cpu_add_sp_imm8(gb_cpu_t* cpu){
    cpu->state.sp = gb_cpu_sp_plus_imm8(cpu);
    gb_cpu_cycle(cpu);
    gb_cpu_cycle(cpu);
}

static inline void gb_cpu_ld_hl_sp_plus_imm8(gb_cpu_t* cpu){
    cpu->state.hl = gb_cpu_sp_plus_imm8(cpu);
    gb_cpu_cycle(cpu);
}


static inline void gb_cpu_add_hl_word(gb_cpu_t* cpu,uint16_t word){
    int result = cpu->state.hl + word;
    gb_cpu_set_flag(gb_cpu_carry_flag,result > 0xFFFF);
    gb_cpu_set_flag(gb_cpu_half_carry_flag,(cpu->state.hl & 0x0FFF) + (word & 0x0FFF) > 0x0FFF);
    gb_cpu_set_flag(gb_cpu_subtraction_flag,false);
    cpu->state.hl = result;
}


static inline void gb_cpu_push_word(gb_cpu_t* cpu,uint16_t word){
    gb_cpu_cycle(cpu);
    gb_cpu_write_byte(cpu,word >> 0x08,--cpu->state.sp);
    gb_cpu_write_byte(cpu,word & 0xFF,--cpu->state.sp);
}

static inline void gb_cpu_pop_word(gb_cpu_t* cpu,uint16_t* word){
    *word = gb_cpu_read_byte(cpu,cpu->state.sp++);
    *word |= gb_cpu_read_byte(cpu,cpu->state.sp++) << 0x08;
}

static inline void gb_cpu_push_af(gb_cpu_t* cpu){
    gb_cpu_cycle(cpu);
    gb_cpu_write_byte(cpu,cpu->state.a,--cpu->state.sp);
    gb_cpu_write_byte(cpu,cpu->state.f & 0xF0,--cpu->state.sp);
}

static inline void gb_cpu_pop_af(gb_cpu_t* cpu){
    cpu->state.f = gb_cpu_read_byte(cpu,cpu->state.sp++) & 0xF0;
    cpu->state.a = gb_cpu_read_byte(cpu,cpu->state.sp++);
}


static inline void gb_cpu_ret(gb_cpu_t* cpu){
    cpu->state.pc = gb_cpu_read_byte(cpu,cpu->state.sp++);
    cpu->state.pc |= gb_cpu_read_byte(cpu,cpu->state.sp++) << 0x08;
    gb_cpu_cycle(cpu);
}

static inline void gb_cpu_ret_cc(gb_cpu_t* cpu,bool condition){
    gb_cpu_cycle(cpu);
    if(condition){
        cpu->state.pc = gb_cpu_read_byte(cpu,cpu->state.sp++);
        cpu->state.pc |= gb_cpu_read_byte(cpu,cpu->state.sp++) << 0x08;
        gb_cpu_cycle(cpu);
    }
}

static inline void gb_cpu_reti(gb_cpu_t* cpu){
    cpu->state.pc = gb_cpu_read_byte(cpu,cpu->state.sp++);
    cpu->state.pc |= gb_cpu_read_byte(cpu,cpu->state.sp++) << 0x08;
    cpu->state.ime = true;
    gb_cpu_cycle(cpu);
}

static inline void gb_cpu_rst(gb_cpu_t* cpu,uint8_t target){
    gb_cpu_cycle(cpu);
    gb_cpu_write_byte(cpu,cpu->state.pc >> 0x08,--cpu->state.sp);
    gb_cpu_write_byte(cpu,cpu->state.pc & 0xFF,--cpu->state.sp);
    cpu->state.pc = target;
}


static inline void gb_cpu_halt(gb_cpu_t* cpu){
    if(cpu->state.ime || !gb_cpu_irq_pending(cpu)){
        gb_cpu_set_mode(cpu,gb_cpu_halted_mode);
    }
    else{
        cpu->state.halt_bug = true;
    }
}

static inline void gb_cpu_stop(gb_cpu_t* cpu){
    gb_t* gb = cpu->gb;

    if(gb_joypad_is_any_button_pressed(&gb->joypad)){
        if(gb_cpu_irq_pending(cpu)){
            //STOP is a 1byte opcode, mode doesnt's change, DIV doesnt's reset 
        }
        else{
            //STOP is a 2byte opcode, HALT mode is entered, DIV is not reset
            gb_cpu_read_byte(cpu,cpu->state.pc++);

            gb_cpu_set_mode(cpu,gb_cpu_halted_mode);
        }
    }
    else{
        if(gb->state.speed_switch_needed){
            if(gb_cpu_irq_pending(cpu)){
                //STOP is a 1byte opcode, mode doesnt's change, DIV is reset, CPU speed changes
                gb_switch_speed(gb);
            }
            else{
                //STOP is a 2byte opcode, HALT mode is entered, DIV is reset, CPU speed changes
                gb_cpu_read_byte(cpu,cpu->state.pc++);
                
                gb_cpu_set_mode(cpu,gb_cpu_halted_mode);

                //Unless an interrupt ocurrs before the, HALT mode whill exit automatically after about 0x20000 T-cycles
                cpu->state.halt_cycles = 0x20000 >> 0x02;

                gb_switch_speed(gb);
            }
        }
        else{
            if(gb_cpu_irq_pending(cpu)){
                //STOP is a 1byte opcode, STOP mode is entered, DIV is reset
                gb_cpu_set_mode(cpu,gb_cpu_stopped_mode);

                gb_timer_set_div(&gb->timer,0);
            }
            else{
                //STOP is a 2byte opcode, STOP mode is entered, DIV is reset
                gb_cpu_read_byte(cpu,cpu->state.pc++);

                gb_cpu_set_mode(cpu,gb_cpu_stopped_mode);

                gb_timer_set_div(&gb->timer,0);
            }
        }
    }
}

static inline void gb_cpu_irq(gb_cpu_t* cpu){
    cpu->state.pc--;
    
    gb_cpu_cycle(cpu);
    gb_cpu_cycle(cpu);

    gb_cpu_write_byte(cpu,cpu->state.pc >> 0x08,--cpu->state.sp);
    
    uint8_t vector = gb_interrupt_get_vector(&cpu->gb->interrupt);
    
    gb_cpu_write_byte(cpu,cpu->state.pc & 0xFF,--cpu->state.sp);

    cpu->state.pc = vector;
    
    cpu->state.ime = false;

    if(cpu->gb->event_manager.enabled){
        gb_event_manager_irq(cpu->gb,vector);
    }
}


static inline void gb_cpu_prefix(gb_cpu_t* cpu,uint8_t prefix){

    switch(prefix){
        //RLC B
        case 0x00: gb_cpu_rlc(cpu,&cpu->state.b); break;
        //RLC C
        case 0x01: gb_cpu_rlc(cpu,&cpu->state.c); break;
        //RLC D
        case 0x02: gb_cpu_rlc(cpu,&cpu->state.d); break;
        //RLC E
        case 0x03: gb_cpu_rlc(cpu,&cpu->state.e); break;
        //RLC H
        case 0x04: gb_cpu_rlc(cpu,&cpu->state.h); break;
        //RLC L
        case 0x05: gb_cpu_rlc(cpu,&cpu->state.l); break;
        //RLC [HL]
        case 0x06: gb_cpu_rlc_phl(cpu); break;
        //RLC A
        case 0x07: gb_cpu_rlc(cpu,&cpu->state.a); break;
        //RRC B
        case 0x08: gb_cpu_rrc(cpu,&cpu->state.b); break;
        //RRC C
        case 0x09: gb_cpu_rrc(cpu,&cpu->state.c); break;
        //RRC D
        case 0x0A: gb_cpu_rrc(cpu,&cpu->state.d); break;
        //RRC E
        case 0x0B: gb_cpu_rrc(cpu,&cpu->state.e); break;
        //RRC H
        case 0x0C: gb_cpu_rrc(cpu,&cpu->state.h); break;
        //RRC L
        case 0x0D: gb_cpu_rrc(cpu,&cpu->state.l); break;
        //RRC [HL]
        case 0x0E: gb_cpu_rrc_phl(cpu); break;
        //RRC A
        case 0x0F: gb_cpu_rrc(cpu,&cpu->state.a); break;

        //RL B
        case 0x10: gb_cpu_rl(cpu,&cpu->state.b); break;
        //RL C
        case 0x11: gb_cpu_rl(cpu,&cpu->state.c); break;
        //RL D
        case 0x12: gb_cpu_rl(cpu,&cpu->state.d); break;
        //RL E
        case 0x13: gb_cpu_rl(cpu,&cpu->state.e); break;
        //RL H
        case 0x14: gb_cpu_rl(cpu,&cpu->state.h); break;
        //RL L
        case 0x15: gb_cpu_rl(cpu,&cpu->state.l); break;
        //RL [HL]
        case 0x16: gb_cpu_rl_phl(cpu); break;
        //RL A
        case 0x17: gb_cpu_rl(cpu,&cpu->state.a); break;
        //RR B
        case 0x18: gb_cpu_rr(cpu,&cpu->state.b); break;
        //RR C
        case 0x19: gb_cpu_rr(cpu,&cpu->state.c); break;
        //RR D
        case 0x1A: gb_cpu_rr(cpu,&cpu->state.d); break;
        //RR E
        case 0x1B: gb_cpu_rr(cpu,&cpu->state.e); break;
        //RR H
        case 0x1C: gb_cpu_rr(cpu,&cpu->state.h); break;
        //RR L
        case 0x1D: gb_cpu_rr(cpu,&cpu->state.l); break;
        //RR [HL]
        case 0x1E: gb_cpu_rr_phl(cpu); break;
        //RR A
        case 0x1F: gb_cpu_rr(cpu,&cpu->state.a); break;

        //SLA B
        case 0x20: gb_cpu_sla(cpu,&cpu->state.b); break;
        //SLA C
        case 0x21: gb_cpu_sla(cpu,&cpu->state.c); break;
        //SLA D
        case 0x22: gb_cpu_sla(cpu,&cpu->state.d); break;
        //SLA E
        case 0x23: gb_cpu_sla(cpu,&cpu->state.e); break;
        //SLA H
        case 0x24: gb_cpu_sla(cpu,&cpu->state.h); break;
        //SLA L
        case 0x25: gb_cpu_sla(cpu,&cpu->state.l); break;
        //SLA [HL]
        case 0x26: gb_cpu_sla_phl(cpu); break;
        //SLA A
        case 0x27: gb_cpu_sla(cpu,&cpu->state.a); break;
        //SRA B
        case 0x28: gb_cpu_sra(cpu,&cpu->state.b); break;
        //SRA C
        case 0x29: gb_cpu_sra(cpu,&cpu->state.c); break;
        //SRA D
        case 0x2A: gb_cpu_sra(cpu,&cpu->state.d); break;
        //SRA E
        case 0x2B: gb_cpu_sra(cpu,&cpu->state.e); break;
        //SRA H
        case 0x2C: gb_cpu_sra(cpu,&cpu->state.h); break;
        //SRA L
        case 0x2D: gb_cpu_sra(cpu,&cpu->state.l); break;
        //SRA [HL]
        case 0x2E: gb_cpu_sra_phl(cpu); break;
        //SRA A
        case 0x2F: gb_cpu_sra(cpu,&cpu->state.a); break;

        //SWAP B
        case 0x30: gb_cpu_swap(cpu,&cpu->state.b); break;
        //SWAP C
        case 0x31: gb_cpu_swap(cpu,&cpu->state.c); break;
        //SWAP D
        case 0x32: gb_cpu_swap(cpu,&cpu->state.d); break;
        //SWAP E
        case 0x33: gb_cpu_swap(cpu,&cpu->state.e); break;
        //SWAP H
        case 0x34: gb_cpu_swap(cpu,&cpu->state.h); break;
        //SWAP L
        case 0x35: gb_cpu_swap(cpu,&cpu->state.l); break;
        //SWAP [HL]
        case 0x36: gb_cpu_swap_phl(cpu); break;
        //SWAP A
        case 0x37: gb_cpu_swap(cpu,&cpu->state.a); break;
        //SRL B
        case 0x38: gb_cpu_srl(cpu,&cpu->state.b); break;
        //SRL C
        case 0x39: gb_cpu_srl(cpu,&cpu->state.c); break;
        //SRL D
        case 0x3A: gb_cpu_srl(cpu,&cpu->state.d); break;
        //SRL E
        case 0x3B: gb_cpu_srl(cpu,&cpu->state.e); break;
        //SRL H
        case 0x3C: gb_cpu_srl(cpu,&cpu->state.h); break;
        //SRL L
        case 0x3D: gb_cpu_srl(cpu,&cpu->state.l); break;
        //SRL [HL]
        case 0x3E: gb_cpu_srl_phl(cpu); break;
        //SRL A
        case 0x3F: gb_cpu_srl(cpu,&cpu->state.a); break;

        //BIT 0,B
        case 0x40: gb_cpu_bit(cpu,cpu->state.b & 0x01); break;
        //BIT 0,C
        case 0x41: gb_cpu_bit(cpu,cpu->state.c & 0x01); break;
        //BIT 0,D
        case 0x42: gb_cpu_bit(cpu,cpu->state.d & 0x01); break;
        //BIT 0,E
        case 0x43: gb_cpu_bit(cpu,cpu->state.e & 0x01); break;
        //BIT 0,H
        case 0x44: gb_cpu_bit(cpu,cpu->state.h & 0x01); break;
        //BIT 0,L
        case 0x45: gb_cpu_bit(cpu,cpu->state.l & 0x01); break;
        //BIT 0,[HL]
        case 0x46: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x01); break;
        //BIT 0,A
        case 0x47: gb_cpu_bit(cpu,cpu->state.a & 0x01); break;
        //BIT 1,B
        case 0x48: gb_cpu_bit(cpu,cpu->state.b & 0x02); break;
        //BIT 1,C
        case 0x49: gb_cpu_bit(cpu,cpu->state.c & 0x02); break;
        //BIT 1,D
        case 0x4A: gb_cpu_bit(cpu,cpu->state.d & 0x02); break;
        //BIT 1,E
        case 0x4B: gb_cpu_bit(cpu,cpu->state.e & 0x02); break;
        //BIT 1,H
        case 0x4C: gb_cpu_bit(cpu,cpu->state.h & 0x02); break;
        //BIT 1,L
        case 0x4D: gb_cpu_bit(cpu,cpu->state.l & 0x02); break;
        //BIT 1,[HL]
        case 0x4E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x02); break;
        //BIT 1,A
        case 0x4F: gb_cpu_bit(cpu,cpu->state.a & 0x02); break;

        //BIT 2,B
        case 0x50: gb_cpu_bit(cpu,cpu->state.b & 0x04); break;
        //BIT 2,C
        case 0x51: gb_cpu_bit(cpu,cpu->state.c & 0x04); break;
        //BIT 2,D
        case 0x52: gb_cpu_bit(cpu,cpu->state.d & 0x04); break;
        //BIT 2,E
        case 0x53: gb_cpu_bit(cpu,cpu->state.e & 0x04); break;
        //BIT 2,H
        case 0x54: gb_cpu_bit(cpu,cpu->state.h & 0x04); break;
        //BIT 2,L
        case 0x55: gb_cpu_bit(cpu,cpu->state.l & 0x04); break;
        //BIT 2,[HL]
        case 0x56: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x04); break;
        //BIT 2,A
        case 0x57: gb_cpu_bit(cpu,cpu->state.a & 0x04); break;
        //BIT 3,B
        case 0x58: gb_cpu_bit(cpu,cpu->state.b & 0x08); break;
        //BIT 3,C
        case 0x59: gb_cpu_bit(cpu,cpu->state.c & 0x08); break;
        //BIT 3,D
        case 0x5A: gb_cpu_bit(cpu,cpu->state.d & 0x08); break;
        //BIT 3,E
        case 0x5B: gb_cpu_bit(cpu,cpu->state.e & 0x08); break;
        //BIT 3,H
        case 0x5C: gb_cpu_bit(cpu,cpu->state.h & 0x08); break;
        //BIT 3,L
        case 0x5D: gb_cpu_bit(cpu,cpu->state.l & 0x08); break;
        //BIT 3,[HL]
        case 0x5E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x08); break;
        //BIT 3,A
        case 0x5F: gb_cpu_bit(cpu,cpu->state.a & 0x08); break;

        //BIT 4,B
        case 0x60: gb_cpu_bit(cpu,cpu->state.b & 0x10); break;
        //BIT 4,C
        case 0x61: gb_cpu_bit(cpu,cpu->state.c & 0x10); break;
        //BIT 4,D
        case 0x62: gb_cpu_bit(cpu,cpu->state.d & 0x10); break;
        //BIT 4,E
        case 0x63: gb_cpu_bit(cpu,cpu->state.e & 0x10); break;
        //BIT 4,H
        case 0x64: gb_cpu_bit(cpu,cpu->state.h & 0x10); break;
        //BIT 4,L
        case 0x65: gb_cpu_bit(cpu,cpu->state.l & 0x10); break;
        //BIT 4,[HL]
        case 0x66: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x10); break;
        //BIT 4,A
        case 0x67: gb_cpu_bit(cpu,cpu->state.a & 0x10); break;
        //BIT 5,B
        case 0x68: gb_cpu_bit(cpu,cpu->state.b & 0x20); break;
        //BIT 5,C
        case 0x69: gb_cpu_bit(cpu,cpu->state.c & 0x20); break;
        //BIT 5,D
        case 0x6A: gb_cpu_bit(cpu,cpu->state.d & 0x20); break;
        //BIT 5,E
        case 0x6B: gb_cpu_bit(cpu,cpu->state.e & 0x20); break;
        //BIT 5,H
        case 0x6C: gb_cpu_bit(cpu,cpu->state.h & 0x20); break;
        //BIT 5,L
        case 0x6D: gb_cpu_bit(cpu,cpu->state.l & 0x20); break;
        //BIT 5,[HL]
        case 0x6E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x20); break;
        //BIT 5,A
        case 0x6F: gb_cpu_bit(cpu,cpu->state.a & 0x20); break;

        //BIT 6,B
        case 0x70: gb_cpu_bit(cpu,cpu->state.b & 0x40); break;
        //BIT 6,C
        case 0x71: gb_cpu_bit(cpu,cpu->state.c & 0x40); break;
        //BIT 6,D
        case 0x72: gb_cpu_bit(cpu,cpu->state.d & 0x40); break;
        //BIT 6,E
        case 0x73: gb_cpu_bit(cpu,cpu->state.e & 0x40); break;
        //BIT 6,H
        case 0x74: gb_cpu_bit(cpu,cpu->state.h & 0x40); break;
        //BIT 6,L
        case 0x75: gb_cpu_bit(cpu,cpu->state.l & 0x40); break;
        //BIT 6,[HL]
        case 0x76: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x40); break;
        //BIT 6,A
        case 0x77: gb_cpu_bit(cpu,cpu->state.a & 0x40); break;
        //BIT 7,B
        case 0x78: gb_cpu_bit(cpu,cpu->state.b & 0x80); break;
        //BIT 7,C
        case 0x79: gb_cpu_bit(cpu,cpu->state.c & 0x80); break;
        //BIT 7,D
        case 0x7A: gb_cpu_bit(cpu,cpu->state.d & 0x80); break;
        //BIT 7,E
        case 0x7B: gb_cpu_bit(cpu,cpu->state.e & 0x80); break;
        //BIT 7,H
        case 0x7C: gb_cpu_bit(cpu,cpu->state.h & 0x80); break;
        //BIT 7,L
        case 0x7D: gb_cpu_bit(cpu,cpu->state.l & 0x80); break;
        //BIT 7,[HL]
        case 0x7E: gb_cpu_bit(cpu,gb_cpu_read_byte(cpu,cpu->state.hl) & 0x80); break;
        //BIT 7,A
        case 0x7F: gb_cpu_bit(cpu,cpu->state.a & 0x80); break;

        //RES 0,B
        case 0x80: cpu->state.b &= ~0x01; break;
        //RES 0,C
        case 0x81: cpu->state.c &= ~0x01; break;
        //RES 0,D
        case 0x82: cpu->state.d &= ~0x01; break;
        //RES 0,E
        case 0x83: cpu->state.e &= ~0x01; break;
        //RES 0,H
        case 0x84: cpu->state.h &= ~0x01; break;
        //RES 0,L
        case 0x85: cpu->state.l &= ~0x01; break;
        //RES 0,[HL]
        case 0x86: gb_cpu_res_phl(cpu,0x01); break;
        //RES 0,A
        case 0x87: cpu->state.a &= ~0x01; break;
        //RES 1,B
        case 0x88: cpu->state.b &= ~0x02; break;
        //RES 1,C
        case 0x89: cpu->state.c &= ~0x02; break;
        //RES 1,D
        case 0x8A: cpu->state.d &= ~0x02; break;
        //RES 1,E
        case 0x8B: cpu->state.e &= ~0x02; break;
        //RES 1,H
        case 0x8C: cpu->state.h &= ~0x02; break;
        //RES 1,L
        case 0x8D: cpu->state.l &= ~0x02; break;
        //RES 1,[HL]
        case 0x8E: gb_cpu_res_phl(cpu,0x02); break;
        //RES 1,A
        case 0x8F: cpu->state.a &= ~0x02; break;

        //RES 2,B
        case 0x90: cpu->state.b &= ~0x04; break;
        //RES 2,C
        case 0x91: cpu->state.c &= ~0x04; break;
        //RES 2,D
        case 0x92: cpu->state.d &= ~0x04; break;
        //RES 2,E
        case 0x93: cpu->state.e &= ~0x04; break;
        //RES 2,H
        case 0x94: cpu->state.h &= ~0x04; break;
        //RES 2,L
        case 0x95: cpu->state.l &= ~0x04; break;
        //RES 2,[HL]
        case 0x96: gb_cpu_res_phl(cpu,0x04); break;
        //RES 2,A
        case 0x97: cpu->state.a &= ~0x04; break;
        //RES 3,B
        case 0x98: cpu->state.b &= ~0x08; break;
        //RES 3,C
        case 0x99: cpu->state.c &= ~0x08; break;
        //RES 3,D
        case 0x9A: cpu->state.d &= ~0x08; break;
        //RES 3,E
        case 0x9B: cpu->state.e &= ~0x08; break;
        //RES 3,H
        case 0x9C: cpu->state.h &= ~0x08; break;
        //RES 3,L
        case 0x9D: cpu->state.l &= ~0x08; break;
        //RES 3,[HL]
        case 0x9E: gb_cpu_res_phl(cpu,0x08); break;
        //RES 3,A
        case 0x9F: cpu->state.a &= ~0x08; break;

        //RES 4,B
        case 0xA0: cpu->state.b &= ~0x10; break;
        //RES 4,C
        case 0xA1: cpu->state.c &= ~0x10; break;
        //RES 4,D
        case 0xA2: cpu->state.d &= ~0x10; break;
        //RES 4,E
        case 0xA3: cpu->state.e &= ~0x10; break;
        //RES 4,H
        case 0xA4: cpu->state.h &= ~0x10; break;
        //RES 4,L
        case 0xA5: cpu->state.l &= ~0x10; break;
        //RES 4,[HL]
        case 0xA6: gb_cpu_res_phl(cpu,0x10); break;
        //RES 4,A
        case 0xA7: cpu->state.a &= ~0x10; break;
        //RES 5,B
        case 0xA8: cpu->state.b &= ~0x20; break;
        //RES 5,C
        case 0xA9: cpu->state.c &= ~0x20; break;
        //RES 5,D
        case 0xAA: cpu->state.d &= ~0x20; break;
        //RES 5,E
        case 0xAB: cpu->state.e &= ~0x20; break;
        //RES 5,H
        case 0xAC: cpu->state.h &= ~0x20; break;
        //RES 5,L
        case 0xAD: cpu->state.l &= ~0x20; break;
        //RES 5,[HL]
        case 0xAE: gb_cpu_res_phl(cpu,0x20); break;
        //RES 5,A
        case 0xAF: cpu->state.a &= ~0x20; break;

        //RES 6,B
        case 0xB0: cpu->state.b &= ~0x40; break;
        //RES 6,C
        case 0xB1: cpu->state.c &= ~0x40; break;
        //RES 6,D
        case 0xB2: cpu->state.d &= ~0x40; break;
        //RES 6,E
        case 0xB3: cpu->state.e &= ~0x40; break;
        //RES 6,H
        case 0xB4: cpu->state.h &= ~0x40; break;
        //RES 6,L
        case 0xB5: cpu->state.l &= ~0x40; break;
        //RES 6,[HL]
        case 0xB6: gb_cpu_res_phl(cpu,0x40); break;
        //RES 6,A
        case 0xB7: cpu->state.a &= ~0x40; break;
        //RES 7,B
        case 0xB8: cpu->state.b &= ~0x80; break;
        //RES 7,C
        case 0xB9: cpu->state.c &= ~0x80; break;
        //RES 7,D
        case 0xBA: cpu->state.d &= ~0x80; break;
        //RES 7,E
        case 0xBB: cpu->state.e &= ~0x80; break;
        //RES 7,H
        case 0xBC: cpu->state.h &= ~0x80; break;
        //RES 7,L
        case 0xBD: cpu->state.l &= ~0x80; break;
        //RES 7,[HL]
        case 0xBE: gb_cpu_res_phl(cpu,0x80); break;
        //RES 7,A
        case 0xBF: cpu->state.a &= ~0x80; break;
        
        //SET 0,B
        case 0xC0: cpu->state.b |= 0x01; break;
        //SET 0,C
        case 0xC1: cpu->state.c |= 0x01; break;
        //SET 0,D  
        case 0xC2: cpu->state.d |= 0x01; break;
        //SET 0,E
        case 0xC3: cpu->state.e |= 0x01; break;
        //SET 0,H
        case 0xC4: cpu->state.h |= 0x01; break;
        //SET 0,L
        case 0xC5: cpu->state.l |= 0x01; break;
        //SET 0,[HL]  
        case 0xC6: gb_cpu_set_phl(cpu,0x01); break;
        //SET 0,A
        case 0xC7: cpu->state.a |= 0x01; break;
        //SET 1,B
        case 0xC8: cpu->state.b |= 0x02; break;
        //SET 1,C
        case 0xC9: cpu->state.c |= 0x02; break;
        //SET 1,D
        case 0xCA: cpu->state.d |= 0x02; break;
        //SET 1,E
        case 0xCB: cpu->state.e |= 0x02; break;
        //SET 1,H
        case 0xCC: cpu->state.h |= 0x02; break;
        //SET 1,L
        case 0xCD: cpu->state.l |= 0x02; break;
        //SET 1,[HL]
        case 0xCE: gb_cpu_set_phl(cpu,0x02); break;
        //SET 1,A
        case 0xCF: cpu->state.a |= 0x02; break;

        //SET 2,B
        case 0xD0: cpu->state.b |= 0x04; break;
        //SET 2,C
        case 0xD1: cpu->state.c |= 0x04; break;
        //SET 2,D  
        case 0xD2: cpu->state.d |= 0x04; break;
        //SET 2,E
        case 0xD3: cpu->state.e |= 0x04; break;
        //SET 2,H
        case 0xD4: cpu->state.h |= 0x04; break;
        //SET 2,L
        case 0xD5: cpu->state.l |= 0x04; break;
        //SET 2,[HL]  
        case 0xD6: gb_cpu_set_phl(cpu,0x04); break;
        //SET 2,A
        case 0xD7: cpu->state.a |= 0x04; break;
        //SET 3,B
        case 0xD8: cpu->state.b |= 0x08; break;
        //SET 3,C
        case 0xD9: cpu->state.c |= 0x08; break;
        //SET 3,D
        case 0xDA: cpu->state.d |= 0x08; break;
        //SET 3,E
        case 0xDB: cpu->state.e |= 0x08; break;
        //SET 3,H
        case 0xDC: cpu->state.h |= 0x08; break;
        //SET 3,L
        case 0xDD: cpu->state.l |= 0x08; break;
        //SET 3,[HL]
        case 0xDE: gb_cpu_set_phl(cpu,0x08); break;
        //SET 3,A
        case 0xDF: cpu->state.a |= 0x08; break;

        //SET 4,B
        case 0xE0: cpu->state.b |= 0x10; break;
        //SET 4,C
        case 0xE1: cpu->state.c |= 0x10; break;
        //SET 4,D  
        case 0xE2: cpu->state.d |= 0x10; break;
        //SET 4,E
        case 0xE3: cpu->state.e |= 0x10; break;
        //SET 4,H
        case 0xE4: cpu->state.h |= 0x10; break;
        //SET 4,L
        case 0xE5: cpu->state.l |= 0x10; break;
        //SET 4,[HL]  
        case 0xE6: gb_cpu_set_phl(cpu,0x10); break;
        //SET 4,A
        case 0xE7: cpu->state.a |= 0x10; break;
        //SET 5,B
        case 0xE8: cpu->state.b |= 0x20; break;
        //SET 5,C
        case 0xE9: cpu->state.c |= 0x20; break;
        //SET 5,D
        case 0xEA: cpu->state.d |= 0x20; break;
        //SET 5,E
        case 0xEB: cpu->state.e |= 0x20; break;
        //SET 5,H
        case 0xEC: cpu->state.h |= 0x20; break;
        //SET 5,L
        case 0xED: cpu->state.l |= 0x20; break;
        //SET 5,[HL]
        case 0xEE: gb_cpu_set_phl(cpu,0x20); break;
        //SET 5,A
        case 0xEF: cpu->state.a |= 0x20; break;

        //SET 6,B
        case 0xF0: cpu->state.b |= 0x40; break;
        //SET 6,C
        case 0xF1: cpu->state.c |= 0x40; break;
        //SET 6,D  
        case 0xF2: cpu->state.d |= 0x40; break;
        //SET 6,E
        case 0xF3: cpu->state.e |= 0x40; break;
        //SET 6,H
        case 0xF4: cpu->state.h |= 0x40; break;
        //SET 6,L
        case 0xF5: cpu->state.l |= 0x40; break;
        //SET 6,[HL]  
        case 0xF6: gb_cpu_set_phl(cpu,0x40); break;
        //SET 6,A
        case 0xF7: cpu->state.a |= 0x40; break;
        //SET 7,B
        case 0xF8: cpu->state.b |= 0x80; break;
        //SET 7,C
        case 0xF9: cpu->state.c |= 0x80; break;
        //SET 7,D
        case 0xFA: cpu->state.d |= 0x80; break;
        //SET 7,E
        case 0xFB: cpu->state.e |= 0x80; break;
        //SET 7,H
        case 0xFC: cpu->state.h |= 0x80; break;
        //SET 7,L
        case 0xFD: cpu->state.l |= 0x80; break;
        //SET 7,[HL]
        case 0xFE: gb_cpu_set_phl(cpu,0x80); break;
        //SET 7,A
        case 0xFF: cpu->state.a |= 0x80; break;
    }
}

static inline void gb_cpu_opcode(gb_cpu_t* cpu,uint8_t opcode){

    switch(opcode){
        //NOP
        case 0x00: break;
        //LD BC,IMM16
        case 0x01: gb_cpu_ld_r16_imm16(cpu,&cpu->state.bc); break;
        //LD [BC],A
        case 0x02: gb_cpu_write_byte(cpu,cpu->state.a,cpu->state.bc); break;
        //INC BC
        case 0x03: gb_cpu_inc_word(cpu,&cpu->state.bc); break;
        //INC B
        case 0x04: gb_cpu_inc_byte(cpu,&cpu->state.b); break;
        //DEC B
        case 0x05: gb_cpu_dec_byte(cpu,&cpu->state.b); break;
        //LD B,IMM8
        case 0x06: cpu->state.b = gb_cpu_read_byte(cpu,cpu->state.pc++); break;
        //RLCA
        case 0x07: gb_cpu_rlca(cpu); break;
        //LD [IMM16],SP
        case 0x08: gb_cpu_ld_pimm16_word(cpu,cpu->state.sp); break;
        //ADD HL,BC
        case 0x09: gb_cpu_add_hl_word(cpu,cpu->state.bc); break;
        //LD A,[BC]
        case 0x0A: cpu->state.a = gb_cpu_read_byte(cpu,cpu->state.bc); break;
        //DEC BC
        case 0x0B: gb_cpu_dec_word(cpu,&cpu->state.bc); break;
        //INC C
        case 0x0C: gb_cpu_inc_byte(cpu,&cpu->state.c); break;
        //DEC C
        case 0x0D: gb_cpu_dec_byte(cpu,&cpu->state.c); break;
        //LD C,IMM8
        case 0x0E: cpu->state.c = gb_cpu_read_byte(cpu,cpu->state.pc++); break;
        //RRCA
        case 0x0F: gb_cpu_rrca(cpu); break;

        //STOP
        case 0x10: gb_cpu_stop(cpu); break;
        //LD DE,IMM16
        case 0x11: gb_cpu_ld_r16_imm16(cpu,&cpu->state.de); break;
        //LD [DE],A
        case 0x12: gb_cpu_write_byte(cpu,cpu->state.a,cpu->state.de); break;
        //INC DE
        case 0x13: gb_cpu_inc_word(cpu,&cpu->state.de); break;
        //INC D
        case 0x14: gb_cpu_inc_byte(cpu,&cpu->state.d); break;
        //DEC D
        case 0x15: gb_cpu_dec_byte(cpu,&cpu->state.d); break;
        //LD D,IMM8
        case 0x16: cpu->state.d = gb_cpu_read_byte(cpu,cpu->state.pc++); break;
        //RLA
        case 0x17: gb_cpu_rla(cpu); break;
        //JR IMM8
        case 0x18: gb_cpu_jr_imm8(cpu,true); break;
        //ADD HL,DE
        case 0x19: gb_cpu_add_hl_word(cpu,cpu->state.de); break;
        //LD A,[DE]
        case 0x1A: cpu->state.a = gb_cpu_read_byte(cpu,cpu->state.de); break;
        //DEC DE
        case 0x1B: gb_cpu_dec_word(cpu,&cpu->state.de); break;
        //INC E
        case 0x1C: gb_cpu_inc_byte(cpu,&cpu->state.e); break;
        //DEC E
        case 0x1D: gb_cpu_dec_byte(cpu,&cpu->state.e); break;
        //LD E,IMM8
        case 0x1E: cpu->state.e = gb_cpu_read_byte(cpu,cpu->state.pc++); break;
        //RRA
        case 0x1F: gb_cpu_rra(cpu); break;

        //JR NZ,IMM8
        case 0x20: gb_cpu_jr_imm8(cpu,!(cpu->state.f & gb_cpu_zero_flag)); break;
        //LD HL,IMM16
        case 0x21: gb_cpu_ld_r16_imm16(cpu,&cpu->state.hl); break;
        //LD [HL+],A
        case 0x22: gb_cpu_write_byte(cpu,cpu->state.a,cpu->state.hl++); break;
        //INC HL
        case 0x23: gb_cpu_inc_word(cpu,&cpu->state.hl); break;
        //INC H
        case 0x24: gb_cpu_inc_byte(cpu,&cpu->state.h); break;
        //DEC H
        case 0x25: gb_cpu_dec_byte(cpu,&cpu->state.h); break;
        //LD H,IMM8
        case 0x26: cpu->state.h = gb_cpu_read_byte(cpu,cpu->state.pc++); break;
        //DAA
        case 0x27: gb_cpu_daa(cpu); break;
        //JR Z,IMM8
        case 0x28: gb_cpu_jr_imm8(cpu,cpu->state.f & gb_cpu_zero_flag); break;
        //ADD HL,HL
        case 0x29: gb_cpu_add_hl_word(cpu,cpu->state.hl); break;
        //LD A,[HL+]
        case 0x2A: cpu->state.a = gb_cpu_read_byte(cpu,cpu->state.hl++); break;
        //DEC HL
        case 0x2B: gb_cpu_dec_word(cpu,&cpu->state.hl); break;
        //INC L
        case 0x2C: gb_cpu_inc_byte(cpu,&cpu->state.l); break;
        //DEC L
        case 0x2D: gb_cpu_dec_byte(cpu,&cpu->state.l); break;
        //LD L,IMM8
        case 0x2E: cpu->state.l = gb_cpu_read_byte(cpu,cpu->state.pc++); break;
        //CPL
        case 0x2F: gb_cpu_cpl(cpu); break;

        //JR NC,IMM8
        case 0x30: gb_cpu_jr_imm8(cpu,!(cpu->state.f & gb_cpu_carry_flag)); break;
        //LD SP,IMM16
        case 0x31: gb_cpu_ld_r16_imm16(cpu,&cpu->state.sp); break;
        //LD [HL-],A
        case 0x32: gb_cpu_write_byte(cpu,cpu->state.a,cpu->state.hl--); break;
        //INC SP
        case 0x33: gb_cpu_inc_word(cpu,&cpu->state.sp); break;
        //INC [HL]
        case 0x34: gb_cpu_inc_phl(cpu); break;
        //DEC [HL]
        case 0x35: gb_cpu_dec_phl(cpu); break;
        //LD [HL],IMM8
        case 0x36: gb_cpu_write_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++),cpu->state.hl); break;
        //SCF
        case 0x37: gb_cpu_scf(cpu); break;
        //JR C,IMM8
        case 0x38: gb_cpu_jr_imm8(cpu,cpu->state.f & gb_cpu_carry_flag); break;
        //ADD HL,SP
        case 0x39: gb_cpu_add_hl_word(cpu,cpu->state.sp); break;
        //LD A,[HL-]
        case 0x3A: cpu->state.a = gb_cpu_read_byte(cpu,cpu->state.hl--); break;
        //DEC SP
        case 0x3B: gb_cpu_dec_word(cpu,&cpu->state.sp); break;
        //INC A
        case 0x3C: gb_cpu_inc_byte(cpu,&cpu->state.a); break;
        //DEC A
        case 0x3D: gb_cpu_dec_byte(cpu,&cpu->state.a); break;
        //LD A,IMM8
        case 0x3E: cpu->state.a = gb_cpu_read_byte(cpu,cpu->state.pc++); break;
        //CCF
        case 0x3F: gb_cpu_ccf(cpu); break;
        
        //LD B,B
        case 0x40: break;
        //LD B,C
        case 0x41: cpu->state.b = cpu->state.c; break;
        //LD B,D
        case 0x42: cpu->state.b = cpu->state.d; break;
        //LD B,E
        case 0x43: cpu->state.b = cpu->state.e; break;
        //LD B,H
        case 0x44: cpu->state.b = cpu->state.h; break;
        //LD B,L
        case 0x45: cpu->state.b = cpu->state.l; break;
        //LD B,[HL]
        case 0x46: cpu->state.b = gb_cpu_read_byte(cpu,cpu->state.hl); break;
        //LD B,A
        case 0x47: cpu->state.b = cpu->state.a; break;
        //LD C,B
        case 0x48: cpu->state.c = cpu->state.b; break;
        //LD C,C
        case 0x49: break;
        //LD C,D
        case 0x4A: cpu->state.c = cpu->state.d; break;
        //LD C,E
        case 0x4B: cpu->state.c = cpu->state.e; break;
        //LD C,H
        case 0x4C: cpu->state.c = cpu->state.h; break;
        //LD C,L
        case 0x4D: cpu->state.c = cpu->state.l; break;
        //LD C,[HL]
        case 0x4E: cpu->state.c = gb_cpu_read_byte(cpu,cpu->state.hl); break;
        //LD C,A
        case 0x4F: cpu->state.c = cpu->state.a; break;

        //LD D,B
        case 0x50: cpu->state.d = cpu->state.b; break;
        //LD D,C
        case 0x51: cpu->state.d = cpu->state.c; break;
        //LD D,D
        case 0x52: break;
        //LD D,E
        case 0x53: cpu->state.d = cpu->state.e; break;
        //LD D,H
        case 0x54: cpu->state.d = cpu->state.h; break;
        //LD D,L
        case 0x55: cpu->state.d = cpu->state.l; break;
        //LD D,[HL]
        case 0x56: cpu->state.d = gb_cpu_read_byte(cpu,cpu->state.hl); break;
        //LD D,A
        case 0x57: cpu->state.d = cpu->state.a; break;
        //LD E,B
        case 0x58: cpu->state.e = cpu->state.b; break;
        //LD E,C
        case 0x59: cpu->state.e = cpu->state.c; break;
        //LD E,D
        case 0x5A: cpu->state.e = cpu->state.d; break;
        //LD E,E
        case 0x5B: break;
        //LD E,H
        case 0x5C: cpu->state.e = cpu->state.h; break;
        //LD E,L
        case 0x5D: cpu->state.e = cpu->state.l; break;
        //LD E,[HL]
        case 0x5E: cpu->state.e = gb_cpu_read_byte(cpu,cpu->state.hl); break;
        //LD E,A
        case 0x5F: cpu->state.e = cpu->state.a; break;

        //LD H,B
        case 0x60: cpu->state.h = cpu->state.b; break;
        //LD H,C
        case 0x61: cpu->state.h = cpu->state.c; break;
        //LD H,D
        case 0x62: cpu->state.h = cpu->state.d; break;
        //LD H,E
        case 0x63: cpu->state.h = cpu->state.e; break;
        //LD H,H
        case 0x64: break;
        //LD H,L
        case 0x65: cpu->state.h = cpu->state.l; break;
        //LD H,[HL]
        case 0x66: cpu->state.h = gb_cpu_read_byte(cpu,cpu->state.hl); break;
        //LD H,A
        case 0x67: cpu->state.h = cpu->state.a; break;
        //LD L,B
        case 0x68: cpu->state.l = cpu->state.b; break;
        //LD L,C
        case 0x69: cpu->state.l = cpu->state.c; break;
        //LD L,D
        case 0x6A: cpu->state.l = cpu->state.d; break;
        //LD L,E
        case 0x6B: cpu->state.l = cpu->state.e; break;
        //LD L,H
        case 0x6C: cpu->state.l = cpu->state.h; break;
        //LD L,L
        case 0x6D: break;
        //LD L,[HL]
        case 0x6E: cpu->state.l = gb_cpu_read_byte(cpu,cpu->state.hl); break;
        //LD L,A
        case 0x6F: cpu->state.l = cpu->state.a; break;

        //LD [HL],B
        case 0x70: gb_cpu_write_byte(cpu,cpu->state.b,cpu->state.hl); break;
        //LD [HL],C
        case 0x71: gb_cpu_write_byte(cpu,cpu->state.c,cpu->state.hl); break;
        //LD [HL],D
        case 0x72: gb_cpu_write_byte(cpu,cpu->state.d,cpu->state.hl); break;
        //LD [HL],E
        case 0x73: gb_cpu_write_byte(cpu,cpu->state.e,cpu->state.hl); break;
        //LD [HL],H
        case 0x74: gb_cpu_write_byte(cpu,cpu->state.h,cpu->state.hl); break;
        //LD [HL],L
        case 0x75: gb_cpu_write_byte(cpu,cpu->state.l,cpu->state.hl); break;
        //HALT
        case 0x76: gb_cpu_halt(cpu); break;
        //LD [HL],A
        case 0x77: gb_cpu_write_byte(cpu,cpu->state.a,cpu->state.hl); break;
        //LD A,B
        case 0x78: cpu->state.a = cpu->state.b; break;
        //LD A,C
        case 0x79: cpu->state.a = cpu->state.c; break;
        //LD A,D
        case 0x7A: cpu->state.a = cpu->state.d; break;
        //LD A,E
        case 0x7B: cpu->state.a = cpu->state.e; break;
        //LD A,H
        case 0x7C: cpu->state.a = cpu->state.h; break;
        //LD A,L
        case 0x7D: cpu->state.a = cpu->state.l; break;
        //LD A,[HL]
        case 0x7E: cpu->state.a = gb_cpu_read_byte(cpu,cpu->state.hl); break;
        //LD A,A
        case 0x7F: break;

        //ADD A,B
        case 0x80: gb_cpu_add_byte(cpu,cpu->state.b); break;
        //ADD A,C
        case 0x81: gb_cpu_add_byte(cpu,cpu->state.c); break;
        //ADD A,D
        case 0x82: gb_cpu_add_byte(cpu,cpu->state.d); break;
        //ADD A,E
        case 0x83: gb_cpu_add_byte(cpu,cpu->state.e); break;
        //ADD A,H
        case 0x84: gb_cpu_add_byte(cpu,cpu->state.h); break;
        //ADD A,L
        case 0x85: gb_cpu_add_byte(cpu,cpu->state.l); break;
        //ADD A,[HL]
        case 0x86: gb_cpu_add_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //ADD A,A
        case 0x87: gb_cpu_add_byte(cpu,cpu->state.a); break;
        //ADC A,B
        case 0x88: gb_cpu_adc_byte(cpu,cpu->state.b); break;
        //ADC A,C
        case 0x89: gb_cpu_adc_byte(cpu,cpu->state.c); break;
        //ADC A,D
        case 0x8A: gb_cpu_adc_byte(cpu,cpu->state.d); break;
        //ADC A,E
        case 0x8B: gb_cpu_adc_byte(cpu,cpu->state.e); break;
        //ADC A,H
        case 0x8C: gb_cpu_adc_byte(cpu,cpu->state.h); break;
        //ADC A,L
        case 0x8D: gb_cpu_adc_byte(cpu,cpu->state.l); break;
        //ADC A,[HL]
        case 0x8E: gb_cpu_adc_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //ADC A,A
        case 0x8F: gb_cpu_adc_byte(cpu,cpu->state.a); break;

        //SUB A,B
        case 0x90: gb_cpu_sub_byte(cpu,cpu->state.b); break;
        //SUB A,C
        case 0x91: gb_cpu_sub_byte(cpu,cpu->state.c); break;
        //SUB A,D
        case 0x92: gb_cpu_sub_byte(cpu,cpu->state.d); break;
        //SUB A,E
        case 0x93: gb_cpu_sub_byte(cpu,cpu->state.e); break;
        //SUB A,H
        case 0x94: gb_cpu_sub_byte(cpu,cpu->state.h); break;
        //SUB A,L
        case 0x95: gb_cpu_sub_byte(cpu,cpu->state.l); break;
        //SUB A,[HL]
        case 0x96: gb_cpu_sub_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //SUB A,A
        case 0x97: gb_cpu_sub_byte(cpu,cpu->state.a); break;
        //SBC A,B
        case 0x98: gb_cpu_sbc_byte(cpu,cpu->state.b); break;
        //SBC A,C
        case 0x99: gb_cpu_sbc_byte(cpu,cpu->state.c); break;
        //SBC A,D
        case 0x9A: gb_cpu_sbc_byte(cpu,cpu->state.d); break;
        //SBC A,E
        case 0x9B: gb_cpu_sbc_byte(cpu,cpu->state.e); break;
        //SBC A,H
        case 0x9C: gb_cpu_sbc_byte(cpu,cpu->state.h); break;
        //SBC A,L
        case 0x9D: gb_cpu_sbc_byte(cpu,cpu->state.l); break;
        //SBC A,[HL]
        case 0x9E: gb_cpu_sbc_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //SBC A,A
        case 0x9F: gb_cpu_sbc_byte(cpu,cpu->state.a); break;

        //AND A,B
        case 0xA0: gb_cpu_and_byte(cpu,cpu->state.b); break;
        //AND A,C
        case 0xA1: gb_cpu_and_byte(cpu,cpu->state.c); break;
        //AND A,D
        case 0xA2: gb_cpu_and_byte(cpu,cpu->state.d); break;
        //AND A,E
        case 0xA3: gb_cpu_and_byte(cpu,cpu->state.e); break;
        //AND A,H
        case 0xA4: gb_cpu_and_byte(cpu,cpu->state.h); break;
        //AND A,L
        case 0xA5: gb_cpu_and_byte(cpu,cpu->state.l); break;
        //AND A,[HL]
        case 0xA6: gb_cpu_and_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //AND A,A
        case 0xA7: gb_cpu_and_byte(cpu,cpu->state.a); break;
        //XOR A,B
        case 0xA8: gb_cpu_xor_byte(cpu,cpu->state.b); break;
        //XOR,A,C
        case 0xA9: gb_cpu_xor_byte(cpu,cpu->state.c); break;
        //XOR A,D
        case 0xAA: gb_cpu_xor_byte(cpu,cpu->state.d); break;
        //XOR A,E
        case 0xAB: gb_cpu_xor_byte(cpu,cpu->state.e); break;
        //XOR A,H
        case 0xAC: gb_cpu_xor_byte(cpu,cpu->state.h); break;
        //XOR A,L
        case 0xAD: gb_cpu_xor_byte(cpu,cpu->state.l); break;
        //XOR A,[HL]
        case 0xAE: gb_cpu_xor_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //XOR A,A
        case 0xAF: gb_cpu_xor_byte(cpu,cpu->state.a); break;

        //OR A,B
        case 0xB0: gb_cpu_or_byte(cpu,cpu->state.b); break;
        //OR A,C
        case 0xB1: gb_cpu_or_byte(cpu,cpu->state.c); break;
        //OR A,D
        case 0xB2: gb_cpu_or_byte(cpu,cpu->state.d); break;
        //OR A,E
        case 0xB3: gb_cpu_or_byte(cpu,cpu->state.e); break;
        //OR A,H
        case 0xB4: gb_cpu_or_byte(cpu,cpu->state.h); break;
        //OR A,L
        case 0xB5: gb_cpu_or_byte(cpu,cpu->state.l); break;
        //OR,A,[HL]
        case 0xB6: gb_cpu_or_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //OR A,A
        case 0xB7: gb_cpu_or_byte(cpu,cpu->state.a); break;
        //CP A,B
        case 0xB8: gb_cpu_cp_byte(cpu,cpu->state.b); break;
        //CP A,C
        case 0xB9: gb_cpu_cp_byte(cpu,cpu->state.c); break;
        //CP A,D
        case 0xBA: gb_cpu_cp_byte(cpu,cpu->state.d); break;
        //CP A,E
        case 0xBB: gb_cpu_cp_byte(cpu,cpu->state.e); break;
        //CP A,H
        case 0xBC: gb_cpu_cp_byte(cpu,cpu->state.h); break;
        //CP A,L
        case 0xBD: gb_cpu_cp_byte(cpu,cpu->state.l); break;
        //CP A,[HL]
        case 0xBE: gb_cpu_cp_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.hl)); break;
        //CP A,A
        case 0xBF: gb_cpu_cp_byte(cpu,cpu->state.a); break;

        //RET NZ
        case 0xC0: gb_cpu_ret_cc(cpu,!(cpu->state.f & gb_cpu_zero_flag)); break;
        //POP BC
        case 0xC1: gb_cpu_pop_word(cpu,&cpu->state.bc); break;
        //JP NZ,IMM16
        case 0xC2: gb_cpu_jp_imm16(cpu,!(cpu->state.f & gb_cpu_zero_flag)); break;
        //JP IMM16
        case 0xC3: gb_cpu_jp_imm16(cpu,true); break;
        //CALL NZ,IMM16
        case 0xC4: gb_cpu_call_imm16(cpu,!(cpu->state.f & gb_cpu_zero_flag)); break;
        //PUSH BC
        case 0xC5: gb_cpu_push_word(cpu,cpu->state.bc); break;
        //ADD A,IMM8
        case 0xC6: gb_cpu_add_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $00
        case 0xC7: gb_cpu_rst(cpu,0x00); break;
        //RET Z
        case 0xC8: gb_cpu_ret_cc(cpu,cpu->state.f & gb_cpu_zero_flag); break;
        //RET
        case 0xC9: gb_cpu_ret(cpu); break;
        //JP Z,IMM16
        case 0xCA: gb_cpu_jp_imm16(cpu,cpu->state.f & gb_cpu_zero_flag); break;
        //PREFIX
        case 0xCB: gb_cpu_prefix(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //CALL Z,IMM16
        case 0xCC: gb_cpu_call_imm16(cpu,cpu->state.f & gb_cpu_zero_flag); break;
        //CALL IMM16
        case 0xCD: gb_cpu_call_imm16(cpu,true); break;
        //ADC A,IMM8
        case 0xCE: gb_cpu_adc_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $08
        case 0xCF: gb_cpu_rst(cpu,0x08); break;

        //RET NC
        case 0xD0: gb_cpu_ret_cc(cpu,!(cpu->state.f & gb_cpu_carry_flag)); break;
        //POP DE
        case 0xD1: gb_cpu_pop_word(cpu,&cpu->state.de); break;
        //JP NC,IMM16
        case 0xD2: gb_cpu_jp_imm16(cpu,!(cpu->state.f & gb_cpu_carry_flag)); break;
        //Undefined
        case 0xD3: break;
        //CALL NC,IMM16
        case 0xD4: gb_cpu_call_imm16(cpu,!(cpu->state.f & gb_cpu_carry_flag)); break;
        //PUSH DE
        case 0xD5: gb_cpu_push_word(cpu,cpu->state.de); break;
        //SUB A,IMM8
        case 0xD6: gb_cpu_sub_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $10
        case 0xD7: gb_cpu_rst(cpu,0x10); break;
        //RET C
        case 0xD8: gb_cpu_ret_cc(cpu,cpu->state.f & gb_cpu_carry_flag); break;
        //RETI
        case 0xD9: gb_cpu_reti(cpu); break;
        //JP C,IMM16
        case 0xDA: gb_cpu_jp_imm16(cpu,cpu->state.f & gb_cpu_carry_flag); break;
        //Undefined
        case 0xDB: break;
        //CALL C,IMM16
        case 0xDC: gb_cpu_call_imm16(cpu,cpu->state.f & gb_cpu_carry_flag); break;
        //Undefined
        case 0xDD: break;
        //SBC A,IMM8
        case 0xDE: gb_cpu_sbc_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $18
        case 0xDF: gb_cpu_rst(cpu,0x18); break;

        //LDH [IMM8],A
        case 0xE0: gb_cpu_write_byte(cpu,cpu->state.a,0xFF00 | gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //POP HL
        case 0xE1: gb_cpu_pop_word(cpu,&cpu->state.hl); break;
        //LDH [C],A
        case 0xE2: gb_cpu_write_byte(cpu,cpu->state.a,0xFF00 | cpu->state.c); break;
        //Undefined
        case 0xE3: break;
        //Undefined
        case 0xE4: break;
        //PUSH HL
        case 0xE5: gb_cpu_push_word(cpu,cpu->state.hl); break;
        //AND A,IMM8
        case 0xE6: gb_cpu_and_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $20
        case 0xE7: gb_cpu_rst(cpu,0x20); break;
        //ADD SP,IMM8
        case 0xE8: gb_cpu_add_sp_imm8(cpu); break;
        //JP HL
        case 0xE9: cpu->state.pc = cpu->state.hl; break;
        //LD [IMM16],A
        case 0xEA: gb_cpu_ld_pimm16_byte(cpu,cpu->state.a); break;
        //Undefined
        case 0xEB: break;
        //Undefined
        case 0xEC: break;
        //Unedefined
        case 0xED: break;
        //XOR A,IMM8
        case 0xEE: gb_cpu_xor_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $28
        case 0xEF: gb_cpu_rst(cpu,0x28); break;

        //LDH A,[IMM8]
        case 0xF0: cpu->state.a = gb_cpu_read_byte(cpu,0xFF00 | gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //POP AF
        case 0xF1: gb_cpu_pop_af(cpu); break;
        //LDH A,[C]
        case 0xF2: cpu->state.a = gb_cpu_read_byte(cpu,0xFF00 | cpu->state.c); break;
        //DI
        case 0xF3: cpu->state.ime = false; break;
        //Undefined
        case 0xF4: break;
        //PUSH AF
        case 0xF5: gb_cpu_push_af(cpu); break;
        //OR A,IMM8
        case 0xF6: gb_cpu_or_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $30
        case 0xF7: gb_cpu_rst(cpu,0x30); break;
        //LD HL,SP+IMM8
        case 0xF8: gb_cpu_ld_hl_sp_plus_imm8(cpu); break;
        //LD SP,HL
        case 0xF9: gb_cpu_ld_sp_hl(cpu); break;
        //LD A,[IMM16]
        case 0xFA: gb_cpu_ld_a_pimm16(cpu); break;
        //EI
        case 0xFB: cpu->state.ime_pending = true; break;
        //Undefined
        case 0xFC: break;
        //Undefined
        case 0xFD: break;
        //CP A,IMM8
        case 0xFE: gb_cpu_cp_byte(cpu,gb_cpu_read_byte(cpu,cpu->state.pc++)); break;
        //RST $38
        case 0xFF: gb_cpu_rst(cpu,0x38); break;
    }
}


static void gb_cpu_running(gb_cpu_t* cpu){

    cpu->instruction_pc = cpu->state.pc;

    uint8_t opcode = gb_cpu_read_byte(cpu,cpu->state.pc);

    if(!cpu->state.halt_bug){
        cpu->state.pc++;
    }
    else{
        cpu->state.halt_bug = false;
    }

    if(cpu->state.ime && gb_cpu_irq_pending(cpu)){
        gb_cpu_irq(cpu);
    }
    else{
        if(cpu->state.ime_pending){
            cpu->state.ime_pending = false;
            cpu->state.ime = true;
        }
        gb_cpu_opcode(cpu,opcode);
    }
}

static void gb_cpu_halted(gb_cpu_t* cpu){

    gb_cpu_cycle(cpu);

    if(gb_cpu_irq_pending(cpu) || (cpu->state.halt_cycles && --cpu->state.halt_cycles == 0x00)){
        
        gb_cpu_set_mode(cpu,gb_cpu_running_mode);

        cpu->instruction_pc = cpu->state.pc;

        uint8_t opcode = gb_memory_cpu_read(&cpu->gb->memory,cpu->state.pc++);

        if(cpu->state.ime && gb_cpu_irq_pending(cpu)){
            gb_cpu_irq(cpu);
        }
        else{
            if(cpu->state.ime_pending){
                cpu->state.ime_pending = false;
                cpu->state.ime = true;
            }
            gb_cpu_opcode(cpu,opcode);
        }
    }
}

static void gb_cpu_stopped(gb_cpu_t* cpu){
    
    gb_cpu_cycle(cpu);

    if(gb_joypad_is_any_button_pressed(&cpu->gb->joypad)){
        
        gb_cpu_set_mode(cpu,gb_cpu_running_mode);

        cpu->instruction_pc = cpu->state.pc;

        uint8_t opcode = gb_memory_cpu_read(&cpu->gb->memory,cpu->state.pc++);

        if(cpu->state.ime && gb_cpu_irq_pending(cpu)){
            gb_cpu_irq(cpu);
        }
        else{
            if(cpu->state.ime_pending){
                cpu->state.ime_pending = false;
                cpu->state.ime = true;
            }
            gb_cpu_opcode(cpu,opcode);
        }
    }
}


void gb_cpu_set_mode(gb_cpu_t* cpu,uint8_t mode){
    switch(mode){
        case gb_cpu_running_mode:{
            
            cpu->state.mode = gb_cpu_running_mode;

            cpu->execute = gb_cpu_running;

            break;
        }
        case gb_cpu_halted_mode:{

            cpu->state.mode = gb_cpu_halted_mode;
            
            cpu->state.halt_cycles = 0;
            
            cpu->execute = gb_cpu_halted;
            
            if(cpu->gb->event_manager.enabled){
                gb_event_manager_halt(cpu->gb);
            }

            break;
        }
        case gb_cpu_stopped_mode:{

            cpu->state.mode = gb_cpu_stopped_mode;
            
            cpu->execute = gb_cpu_stopped;

            if(cpu->gb->event_manager.enabled){
                gb_event_manager_stop(cpu->gb);
            }

            break;
        }
    }
}


void gb_cpu_reset(gb_cpu_t* cpu){

    memset(&cpu->state,0x00,sizeof(cpu->state));

    cpu->instruction_pc = 0x00;

    gb_cpu_set_mode(cpu,gb_cpu_running_mode);
}

void gb_cpu_skip_boot(gb_cpu_t* cpu){
    if(cpu->gb->state.is_cgb){

        if(cpu->gb->state.cgb_mode){
            cpu->state.af = 0x1180;
            cpu->state.bc = 0x0000;
            cpu->state.de = 0xFF56;
            cpu->state.hl = 0x000D;
        }
        else{
            cpu->state.af = 0x1180;
            cpu->state.bc = 0x1400;
            cpu->state.de = 0x0008;
            cpu->state.hl = 0x007C;
        }
    }
    else{
        cpu->state.af = 0x0190;
        cpu->state.bc = 0x0013;
        cpu->state.de = 0x00D8;
        cpu->state.hl = 0x014D;
    }

    cpu->state.sp = 0xFFFE;
    cpu->state.pc = 0x0100;

    cpu->instruction_pc = cpu->state.pc;
}


void gb_cpu_save_state(gb_cpu_t* cpu,gb_snapshot_t* snapshot){
    snapshot->cpu = cpu->state;
}

void gb_cpu_load_state(gb_cpu_t* cpu,gb_snapshot_t* snapshot){
    cpu->state = snapshot->cpu;

    cpu->instruction_pc = cpu->state.pc;

    switch(cpu->state.mode){
        case gb_cpu_running_mode:{
            cpu->execute = gb_cpu_running;
            break;
        }
        case gb_cpu_halted_mode:{
            cpu->execute = gb_cpu_halted;
            break;
        }
        case gb_cpu_stopped_mode:{
            cpu->execute = gb_cpu_stopped;
            break;
        }
    }
}
