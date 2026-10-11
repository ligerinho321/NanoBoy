#pragma once

#include "../utils.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t gb_disassembler_opcode_length(uint8_t opcode);

uint8_t gb_disassembler_disassemble(gb_t* gb,uint16_t pc,char *dst,size_t capacity);

uint8_t gb_disassembler_disassemble_current_pc(gb_t* gb,char* dst,size_t capacity);

#ifdef __cplusplus
}
#endif
