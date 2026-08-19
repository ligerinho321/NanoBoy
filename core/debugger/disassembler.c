#include "disassembler.h"
#include "../gb.h"

#define gb_disassembler_print(src)\
    len += snprintf(dst + len,capacity - len,src);

#define gb_disassembler_printf(fmt,...)\
    len += snprintf(dst + len,capacity - len,fmt,__VA_ARGS__)


static const char* r8[8] = {"B", "C", "D", "E", "H", "L", "[HL]", "A"};

static const char* r16[4] = {"BC", "DE", "HL", "SP"};

static const char* r16stk[4] = {"BC", "DE", "HL", "AF"};

static const char* r16mem[4] = {"BC", "DE", "HL+", "HL-"};

static const char* cond[4] = {"NZ", "Z", "NC", "C"};


static inline uint8_t read_byte(gb_t* gb,uint16_t address){
    return gb_memory_cpu_read(&gb->memory,address);
}

static inline uint16_t read_word(gb_t* gb,uint16_t address){
    uint8_t low = gb_memory_cpu_read(&gb->memory,address);
    uint8_t high = gb_memory_cpu_read(&gb->memory,address + 0x01);
    return (high << 0x08) | low;
}


uint8_t gb_disassembler_opcode_length(uint8_t opcode){
    switch(opcode){
        case 0x00: return 1;
        case 0x07: return 1;
        case 0x08: return 3;
        case 0x0F: return 1;
        case 0x10: return 2;
        case 0x17: return 1;
        case 0x18: return 2;
        case 0x1F: return 1;
        case 0x27: return 1;
        case 0x2F: return 1;
        case 0x37: return 1;
        case 0x3F: return 1;
        case 0x76: return 1;
        case 0xC3: return 3;
        case 0xC6: return 2;
        case 0xC9: return 1;
        case 0xCB: return 2;
        case 0xCD: return 3;
        case 0xCE: return 2;
        case 0xD6: return 2;
        case 0xD9: return 1;
        case 0xDE: return 2;
        case 0xE0: return 2;
        case 0xE2: return 1;
        case 0xE6: return 2;
        case 0xE8: return 2;
        case 0xE9: return 1;
        case 0xEA: return 3;
        case 0xEE: return 2;
        case 0xF0: return 2;
        case 0xF2: return 1;
        case 0xF3: return 1;
        case 0xF6: return 2;
        case 0xF8: return 2;
        case 0xF9: return 1;
        case 0xFA: return 3;
        case 0xFB: return 1;
        case 0xFE: return 2;
    }

    if((opcode & 0xC0) == 0x40){
        return 1;
    }

    switch(opcode & ~0x30){
        case 0x01: return 3;
        case 0x02: return 1;
        case 0x03: return 1;
        case 0x09: return 1;
        case 0x0A: return 1;
        case 0x0B: return 1;
        case 0xC1: return 1;
        case 0xC5: return 1;
    }

    switch(opcode & ~0x18){
        case 0x20: return 2;
        case 0xC0: return 1;
        case 0xC2: return 3;
        case 0xC4: return 3;
    }

    switch(opcode & ~0x07){
        case 0x80: return 1;
        case 0x88: return 1;
        case 0x90: return 1;
        case 0x98: return 1;
        case 0xA0: return 1;
        case 0xA8: return 1;
        case 0xB0: return 1;
        case 0xB8: return 1;
    }

    switch(opcode & ~0x38){
        case 0x04: return 1;
        case 0x05: return 1;
        case 0x06: return 2;
        case 0xC7: return 1;
    }

    return 1;
}

uint8_t gb_disassembler_disassemble(gb_t* gb,uint16_t pc,char *dst,size_t capacity){
    
    int len = 0;

    uint8_t opcode = read_byte(gb,pc);
    uint8_t opcode_len = gb_disassembler_opcode_length(opcode);

    gb_disassembler_printf("%04X: ",pc);

    for(uint8_t i = 0; i < opcode_len; ++i){
        gb_disassembler_printf("%02X ",read_byte(gb,pc + i));
    }

    uint8_t padding = 12 - (opcode_len * 3);

    while(padding--) gb_disassembler_print(" ");

    ++pc;

    switch(opcode){
        case 0x00: gb_disassembler_print("NOP"); goto end;

        case 0x07: gb_disassembler_print("RLCA"); goto end;
        
        case 0x08: gb_disassembler_printf("LD [$%04X], SP",read_word(gb,pc)); goto end;
        
        case 0x0F: gb_disassembler_print("RRCA"); goto end;
        
        case 0x10: gb_disassembler_print("STOP"); goto end;
        
        case 0x17: gb_disassembler_print("RLA"); goto end;
        
        case 0x18: gb_disassembler_printf("JR $%04X",(pc + 0x01) + (int8_t)read_byte(gb,pc)); goto end;
        
        case 0x1F: gb_disassembler_print("RRA"); goto end;
        
        case 0x27: gb_disassembler_print("DAA"); goto end;
        
        case 0x2F: gb_disassembler_print("CPL"); goto end;
        
        case 0x37: gb_disassembler_print("SCF"); goto end;
        
        case 0x3F: gb_disassembler_print("CCF"); goto end;
        
        case 0x76: gb_disassembler_print("HALT"); goto end;
        
        case 0xC3: gb_disassembler_printf("JP $%04X",read_word(gb,pc)); goto end;
        
        case 0xC6: gb_disassembler_printf("ADD A, $%02X",read_byte(gb,pc)); goto end;
        
        case 0xC9: gb_disassembler_print("RET"); goto end;
        
        case 0xCB:{
            
            uint8_t prefix = read_byte(gb,pc++);

            switch((prefix & 0xC0) >> 0x06){
                case 0x00:{
                    switch((prefix & 0x38) >> 0x03){
                        case 0x00: gb_disassembler_printf("RLC %s",r8[prefix & 0x07]); goto end;
                        
                        case 0x01: gb_disassembler_printf("RRC %s",r8[prefix & 0x07]); goto end;
                        
                        case 0x02: gb_disassembler_printf("RL %s",r8[prefix & 0x07]); goto end;
                        
                        case 0x03: gb_disassembler_printf("RR %s",r8[prefix & 0x07]); goto end;
                        
                        case 0x04: gb_disassembler_printf("SLA %s",r8[prefix & 0x07]); goto end;
                        
                        case 0x05: gb_disassembler_printf("SRA %s",r8[prefix & 0x07]); goto end;
                        
                        case 0x06: gb_disassembler_printf("SWAP %s",r8[prefix & 0x07]); goto end;

                        case 0x07: gb_disassembler_printf("SRL %s",r8[prefix & 0x07]); goto end;
                    }
                    break;
                }

                case 0x01: gb_disassembler_printf("BIT %d, %s",(prefix & 0x38) >> 0x03,r8[prefix & 0x07]); goto end;

                case 0x02: gb_disassembler_printf("RES %d, %s",(prefix & 0x38) >> 0x03,r8[prefix & 0x07]); goto end;
                
                case 0x03: gb_disassembler_printf("SET %d, %s",(prefix & 0x38) >> 0x03,r8[prefix & 0x07]); goto end;
            }
            break;
        }
        
        case 0xCD: gb_disassembler_printf("CALL $%04X",read_word(gb,pc)); goto end;
        
        case 0xCE: gb_disassembler_printf("ADC A, $%02X",read_byte(gb,pc)); goto end;
        
        case 0xD6: gb_disassembler_printf("SUB A, $%02X",read_byte(gb,pc)); goto end;
        
        case 0xD9: gb_disassembler_print("RETI"); goto end;
        
        case 0xDE: gb_disassembler_printf("SBC A, $%02X",read_byte(gb,pc)); goto end;
        
        case 0xE0: gb_disassembler_printf("LDH [$%04X], A",0xFF00 | read_byte(gb,pc)); goto end;
        
        case 0xE2: gb_disassembler_print("LDH [$FF00+C], A"); goto end;
        
        case 0xE6: gb_disassembler_printf("AND A, $%02X",read_byte(gb,pc)); goto end;
        
        case 0xE8: gb_disassembler_printf("ADD SP, $%02X",read_byte(gb,pc)); goto end;
        
        case 0xE9: gb_disassembler_print("JP HL"); goto end;
        
        case 0xEA: gb_disassembler_printf("LD [$%04X], A",read_word(gb,pc)); goto end;
        
        case 0xEE: gb_disassembler_printf("XOR A, $%02X",read_byte(gb,pc)); goto end;
       
        case 0xF0: gb_disassembler_printf("LDH A, [$%04X]",0xFF00 | read_byte(gb,pc)); goto end;
        
        case 0xF2: gb_disassembler_print("LDH A, [$FF00+C]"); goto end;
        
        case 0xF3: gb_disassembler_print("DI"); goto end;
        
        case 0xF6: gb_disassembler_printf("OR A, $%02X",read_byte(gb,pc)); goto end;
        
        case 0xF8: gb_disassembler_printf("LD HL, SP + $%02X",read_byte(gb,pc)); goto end;
        
        case 0xF9: gb_disassembler_print("LD SP, HL"); goto end;
        
        case 0xFA: gb_disassembler_printf("LD A, [$%04X]",read_word(gb,pc)); goto end;
        
        case 0xFB: gb_disassembler_print("EI"); goto end;
        
        case 0xFE: gb_disassembler_printf("CP A, $%02X",read_byte(gb,pc)); goto end;
    }

    switch(opcode & ~0x07){
        case 0x80: gb_disassembler_printf("ADD A, %s",r8[opcode & 0x07]); goto end;
        
        case 0x88: gb_disassembler_printf("ADC A, %s",r8[opcode & 0x07]); goto end;
        
        case 0x90: gb_disassembler_printf("SUB A, %s",r8[opcode & 0x07]); goto end;
        
        case 0x98: gb_disassembler_printf("SBC A, %s",r8[opcode & 0x07]); goto end;
        
        case 0xA0: gb_disassembler_printf("AND A, %s",r8[opcode & 0x07]); goto end;
        
        case 0xA8: gb_disassembler_printf("XOR A, %s",r8[opcode & 0x07]); goto end;
        
        case 0xB0: gb_disassembler_printf("OR A, %s",r8[opcode & 0x07]); goto end;

        case 0xB8: gb_disassembler_printf("CP A, %s",r8[opcode & 0x07]); goto end;
    }

    switch(opcode & ~0x38){
        case 0x04: gb_disassembler_printf("INC %s",r8[(opcode & 0x38) >> 0x03]); goto end;

        case 0x05: gb_disassembler_printf("DEC %s",r8[(opcode & 0x38) >> 0x03]); goto end;

        case 0x06: gb_disassembler_printf("LD %s, $%02X",r8[(opcode & 0x38) >> 0x03],read_byte(gb,pc)); goto end;

        case 0xC7: gb_disassembler_printf("RST $%02X",opcode & 0x38); goto end;
    }

    switch(opcode & ~0x30){
        case 0x01: gb_disassembler_printf("LD %s, $%04X",r16[(opcode & 0x30) >> 0x04],read_word(gb,pc)); goto end;
        
        case 0x02: gb_disassembler_printf("LD [%s], A",r16mem[(opcode & 0x30) >> 0x04]); goto end;
        
        case 0x03: gb_disassembler_printf("INC %s",r16[(opcode & 0x30) >> 0x04]); goto end;
        
        case 0x09: gb_disassembler_printf("ADD HL, %s",r16[(opcode & 0x30) >> 0x04]); goto end;
 
        case 0x0A: gb_disassembler_printf("LD A, [%s]",r16mem[(opcode & 0x30) >> 0x04]); goto end;

        case 0x0B: gb_disassembler_printf("DEC %s",r16[(opcode & 0x30) >> 0x04]); goto end;

        case 0xC1: gb_disassembler_printf("POP %s",r16stk[(opcode & 0x30) >> 0x04]); goto end;

        case 0xC5: gb_disassembler_printf("PUSH %s",r16stk[(opcode & 0x30) >> 0x04]); goto end;
    }

    switch(opcode & ~0x18){
        case 0x20: gb_disassembler_printf("JR %s, $%04X",cond[(opcode & 0x18) >> 0x03],(pc + 0x01) + (int8_t)read_byte(gb,pc)); goto end;
        
        case 0xC0: gb_disassembler_printf("RET %s",cond[(opcode & 0x18) >> 0x03]); goto end;

        case 0xC2: gb_disassembler_printf("JP %s, $%04X",cond[(opcode & 0x18) >> 0x03],read_word(gb,pc)); goto end;

        case 0xC4: gb_disassembler_printf("CALL %s, $%04X",cond[(opcode & 0x18) >> 0x03],read_word(gb,pc)); goto end;
    }

    if((opcode & 0xC0) == 0x40){
        gb_disassembler_printf("LD %s, %s",r8[(opcode & 0x38) >> 0x03],r8[opcode & 0x07]);
        goto end;
    }

    gb_disassembler_print("INVALID");

    end:
    return opcode_len;
}

uint8_t gb_disassembler_disassemble_current_pc(gb_t* gb,char* dst,size_t capacity){
    return gb_disassembler_disassemble(gb,gb->cpu.pc,dst,capacity);
}