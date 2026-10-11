#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define gba_printf_errno(f) fprintf(stderr,"function: %s line: %d %s: %s\n",__func__,__LINE__,#f,strerror(errno))
#define gba_printf_error(e) fprintf(stderr,"function: %s line: %d error: %s\n",__func__,__LINE__,e)

#define gba_unused(x) ((void)x)

typedef struct _gba_t gba_t;