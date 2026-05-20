#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRINTF_ERRNO(f) fprintf(stderr,"function: %s line: %d %s: %s\n",__func__,__LINE__,#f,strerror(errno))
#define PRINTF_ERROR(e) fprintf(stderr,"function: %s line: %d error: %s\n",__func__,__LINE__,e)

typedef struct _gb_t gb_t;

typedef struct _gb_memory_handler_t {
    void (*write)(void*,uint8_t,uint16_t);
    uint8_t (*read)(void*,uint16_t);
    void* data;
} gb_memory_handler_t;

#ifdef __cplusplus
}
#endif
