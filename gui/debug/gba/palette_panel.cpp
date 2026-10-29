#include <gui/debug/gba/palette_panel.hpp>

void gba_palette_panel_t::init(SDL_Renderer* renderer){
    texture = SDL_CreateTexture(renderer,texture_format,texture_access,texture_width,texture_height);
}

void gba_palette_panel_t::uninit(){
    if(texture != nullptr){
        SDL_DestroyTexture(texture);
    }
}

void gba_palette_panel_t::update_data(const uint8_t* src){
    memcpy(colors,src,sizeof(colors));
}

void gba_palette_panel_t::update_texture(){
    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,(void**)&pixels,&pitch);

    for(int row = 0; row < 16; ++row){
        for(int col = 0; col < 16; ++col){

            SDL_Color color = rgb555_to_rgb888(unpack_rgb555(colors[row * 16 + col]));

            for(int y = 0; y < 8; ++y){
                for(int x = 0; x < 8; ++x){
                    uint8_t* pixel = pixels + (row * 8 + y) * pitch + (col * 8 + x) * 3;
                    pixel[0] = color.r;
                    pixel[1] = color.g;
                    pixel[2] = color.b;
                }
            }
        }
    }

    SDL_UnlockTexture(texture);
}

void gba_palette_panel_t::clear(){
    clear_texture(texture,texture_height);
    memset(colors,0x00,sizeof(colors));
}