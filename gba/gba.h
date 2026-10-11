#pragma once

#include "utils.h"
#include "cpu.h"

typedef struct _gba_t {
    gba_cpu_t cpu;
} gba_t;

gba_t* gba_new();

void gba_delete(gba_t* gba);