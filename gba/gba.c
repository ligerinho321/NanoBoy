#include "gba.h"

gba_t* gba_new(){
    gba_t* gba = malloc(sizeof(gba_t));

    if(!gba){
        gba_printf_errno(malloc);
        return NULL;
    }

    memset(gba,0x00,sizeof(gba_t));

    gba_cpu_init(&gba->cpu,gba);
    
    return gba;
}

void gba_delete(gba_t* gba){
    free(gba);
}