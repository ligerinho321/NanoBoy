#include "boot.h"
#include "gba.h"

void gba_boot_init(gba_boot_t* boot,gba_t* gba){
    boot->gba = gba;
}


void gba_boot_set_path_reference(gba_t* gba,const char* path){
    gba->boot.path = path;
}

void gba_boot_remove_path_reference(gba_t* gba){
    gba->boot.path = NULL;
}


void gba_boot_enable_skip(gba_t* gba,bool enabled){
    gba->boot.skip_enabled = enabled;
}


void gba_boot_update(gba_boot_t* boot){
    boot->inserted = false;
    boot->crc32 = 0x00;

    if(!boot->path) return;

    FILE* file = fopen(boot->path,"rb");

    if(!file){
        gba_printf_errno(fopen);
        goto end;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    if(size != gba_boot_length) goto end;

    fread(boot->data,1,gba_boot_length,file);

    boot->inserted = true;
    boot->crc32 = gba_crc32(boot->data,gba_boot_length);

    end:
    fclose(file);
}


gba_word_t gba_boot_read(gba_boot_t* boot,uint32_t address,uint8_t size){

    address &= gba_boot_address_mask;

    gba_word_t value = {0};

    if(boot->gba->cpu.r[15] < gba_boot_length){

        switch(size){
            case gba_memory_byte_access:{
                gba_clock(boot->gba,1);

                value.b[0] = boot->data[address];
                value.b[1] = value.b[0];
                value.b[2] = value.b[0];
                value.b[3] = value.b[0];
                break;
            }
            case gba_memory_halfword_access:{
                address &= ~0x01;

                gba_clock(boot->gba,1);

                value.b[0] = boot->data[address | 0x00];
                value.b[1] = boot->data[address | 0x01];
                value.b[2] = value.b[0];
                value.b[3] = value.b[1];
                break;
            }
            case gba_memory_word_access:{
                address &= ~0x03;

                gba_clock(boot->gba,1);

                value.b[0] = boot->data[address | 0x00];
                value.b[1] = boot->data[address | 0x01];
                value.b[2] = boot->data[address | 0x02];
                value.b[3] = boot->data[address | 0x03];
                break;
            }
        }

        boot->bus.v = value.v;
    }
    else{
        switch(size){
            case gba_memory_byte_access:{
                gba_clock(boot->gba,1);

                value.b[0] = boot->bus.b[address & 0x03];
                value.b[1] = value.b[0];
                value.b[2] = value.b[0];
                value.b[3] = value.b[0];
                break;
            }
            case gba_memory_halfword_access:{
                address &= ~0x01;

                gba_clock(boot->gba,1);

                value.b[0] = boot->bus.b[address | 0x00];
                value.b[1] = boot->bus.b[address | 0x01];
                value.b[2] = value.b[0];
                value.b[3] = value.b[1];
                break;
            }
            case gba_memory_word_access:{
                gba_clock(boot->gba,1);

                value.v = boot->bus.v;
                break;
            }
        }
    }

    return value;
}


void gba_boot_reset(gba_boot_t* boot){
    boot->bus.v = 0x00;
}