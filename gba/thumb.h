#pragma once

#include "utils.h"

typedef struct _gba_cpu_t gba_cpu_t;

typedef void (*gba_cpu_thumb_instruction_callback_t)(gba_cpu_t*,uint16_t);

extern gba_cpu_thumb_instruction_callback_t thumb_table[512];

#ifdef _cplusplus
#endif