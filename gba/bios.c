#include "bios.h"
#include "gba.h"

void gba_bios_init(gba_bios_t* bios,gba_t* gba){
    bios->gba = gba;
}


void gba_bios_set_path_reference(gba_t* gba,const char* path){
    gba->bios.path = path;
}


void gba_bios_update(gba_bios_t* bios){
    bios->inserted = false;
    bios->crc32 = 0x00;

    if(!bios->path) return;

    FILE* file = fopen(bios->path,"rb");

    if(!file){
        gba_printf_errno(fopen);
        goto end;
    }

    fseek(file,0,SEEK_END);
    size_t size = ftell(file);
    fseek(file,0,SEEK_SET);

    if(size != gba_bios_length) goto end;

    fread(bios->data,1,gba_bios_length,file);

    bios->inserted = true;
    bios->crc32 = gba_crc32(bios->data,gba_bios_length);

    end:
    fclose(file);
}


gba_word_t gba_bios_read(gba_bios_t* bios,uint32_t address,uint8_t size){

    address &= gba_bios_address_mask;

    gba_word_t value = {0};

    if(bios->gba->cpu.r[15] < gba_bios_length){

        switch(size){
            case gba_memory_byte_access:{
                gba_clock(bios->gba,1);

                value.b[0] = bios->data[address];
                value.b[1] = value.b[0];
                value.b[2] = value.b[0];
                value.b[3] = value.b[0];
                break;
            }
            case gba_memory_halfword_access:{
                address &= ~0x01;

                gba_clock(bios->gba,1);

                value.b[0] = bios->data[address | 0x00];
                value.b[1] = bios->data[address | 0x01];
                value.b[2] = value.b[0];
                value.b[3] = value.b[1];
                break;
            }
            case gba_memory_word_access:{
                address &= ~0x03;

                gba_clock(bios->gba,1);

                value.b[0] = bios->data[address | 0x00];
                value.b[1] = bios->data[address | 0x01];
                value.b[2] = bios->data[address | 0x02];
                value.b[3] = bios->data[address | 0x03];
                break;
            }
        }

        bios->bus.v = value.v;
    }
    else{
        switch(size){
            case gba_memory_byte_access:{
                gba_clock(bios->gba,1);

                value.b[0] = bios->bus.b[address & 0x03];
                value.b[1] = value.b[0];
                value.b[2] = value.b[0];
                value.b[3] = value.b[0];
                break;
            }
            case gba_memory_halfword_access:{
                address &= ~0x01;

                gba_clock(bios->gba,1);

                value.b[0] = bios->bus.b[address | 0x00];
                value.b[1] = bios->bus.b[address | 0x01];
                value.b[2] = value.b[0];
                value.b[3] = value.b[1];
                break;
            }
            case gba_memory_word_access:{
                gba_clock(bios->gba,1);

                value.v = bios->bus.v;
                break;
            }
        }
    }

    return value;
}


void gba_bios_reset(gba_bios_t* bios){
    bios->bus.v = 0x00;
}