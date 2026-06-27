#include "utils.hpp"

void palette_t::update_texture(gb_type_t type,bool cgb_mode){
    
    int rows = cgb_mode ? gb_cgb_palettes : (is_obj ? gb_dmg_obj_palettes : gb_dmg_bg_palettes);

    gb_rgb_t color = {0};

    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,(void**)&pixels,&pitch);

    for(int row = 0; row < rows; ++row){
        for(int col = 0; col < gb_palette_colors; ++col){

            if(type == gb_cgb){
                if(cgb_mode){
                    color = get_cgb_color(row,col);
                }
                else{
                    color = get_cgb_dmg_color(row,col);
                }
            }
            else{
                color = get_dmg_color(row,col);
            }

            for(int y = 0; y < gb_tile_size; ++y){
                for(int x = 0; x < gb_tile_size; ++x){
                    uint8_t* pixel = pixels + ((row * gb_tile_size) | y) * pitch + ((col * gb_tile_size) | x) * texture_bytes_per_pixel;
                    pixel[0] = color.r;
                    pixel[1] = color.g;
                    pixel[2] = color.b;
                }
            }

        }
    }

    SDL_UnlockTexture(texture);
}

void bg_palette_t::clear(){
    clear_texture(texture,texture_max_height);
    bgp = 0;
    memset(colors,0,sizeof(colors));
}

void obj_palette_t::clear(){
    clear_texture(texture,texture_max_height);
    obp[0] = 0;
    obp[1] = 0;
    memset(colors,0,sizeof(colors));
}


void clear_texture(SDL_Texture* texture,int height){
    void* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,&pixels,&pitch);
    memset(pixels,0,pitch * height);
    SDL_UnlockTexture(texture);
}
