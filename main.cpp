#include <nanoboy.hpp>

int main(int n_args, char** args){
    gb_unused(n_args);
    gb_unused(args);

    nanoboy_t* nanoboy = new nanoboy_t();
    
    nanoboy->run();
    
    delete nanoboy;
    
    return 0;
}