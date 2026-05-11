#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct _gb_memory_handler_t {
    void (*write)(void*,uint8_t,uint16_t);
    uint8_t (*read)(void*,uint16_t);
    void* data;
} gb_memory_handler_t;
