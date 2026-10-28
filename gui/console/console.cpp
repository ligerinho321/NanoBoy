#include <gui/console/console.hpp>
#include <gui/nanoboy/nanoboy.hpp>

void console_t::create_screen_texture(){

    if(screen_texture != nullptr){
        SDL_DestroyTexture(screen_texture);
    }

    screen_texture = SDL_CreateTexture(
        nanoboy->renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        screen_width(),
        screen_height()
    );

    clear_texture(screen_texture,screen_height());
}