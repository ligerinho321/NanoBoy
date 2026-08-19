#include <gui/utils/utils.hpp>

void palette_t::update_texture(bool is_cgb,bool cgb_mode){
    
    int rows = cgb_mode ? gb_cgb_palettes : (is_obj ? gb_dmg_obj_palettes : gb_dmg_bg_palettes);

    gb_rgb_t color{};

    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,(void**)&pixels,&pitch);

    for(int row = 0; row < rows; ++row){
        for(int col = 0; col < gb_palette_colors; ++col){

            if(is_cgb){
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


time_t get_file_last_write_time(std::filesystem::path file){

    std::chrono::time_point sys_time_point = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        std::filesystem::last_write_time(file) - 
        std::filesystem::file_time_type::clock::now() + 
        std::chrono::system_clock::now()
    );

    return std::chrono::system_clock::to_time_t(sys_time_point);
}

const char* get_time_formated(time_t time){
    static char buffer[32] = {0};

    tm* local_time = localtime(&time);
    
    std::strftime(buffer,sizeof(buffer),"%d/%m/%Y %H:%M",local_time);

    return buffer;
}


void clear_texture(SDL_Texture* texture,int height){
    void* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,&pixels,&pitch);
    memset(pixels,0,pitch * height);
    SDL_UnlockTexture(texture);
}
