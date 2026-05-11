#include <gb.h>

// 4.194.304 master clocks per seconds in single speed
// 8.388.608 master clocks per seconds in double speed

int main(int n_args,char** args){

    FILE* file = fopen("/home/artur/Documentos/NanoBoy/roms/gb-test-roms-master/cpu_instrs/cpu_instrs.gb","rb");
    
    if(!file){
        printf("failed open file");
        return 1;
    }

    fseek(file,0,SEEK_END);
    size_t length = ftell(file);
    fseek(file,0,SEEK_SET);

    uint8_t* data = malloc(length);
    
    fread(data,sizeof(uint8_t),length,file);
    
    fclose(file);

    printf("cartridge type: %02X\n",data[0x147]);

    free(data);

    return 0;
}