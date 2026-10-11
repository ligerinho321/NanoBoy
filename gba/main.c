#include "gba.h"

int main(int n_args,char** args){
    gba_t* gba = gba_new();
    gba_delete(gba);
    return 0;
}