#pragma once

#include "utils.hpp"

class object_viewer_t {
private:
    enum{
        bg_texture_width = 256,
        bg_texture_height = 256,

        bg_texture_format = SDL_PIXELFORMAT_RGB24,
        bg_texture_bytes_per_pixel = SDL_BYTESPERPIXEL(bg_texture_format),

        obj_texture_format = SDL_PIXELFORMAT_RGBA32,
        obj_texture_bytes_per_pixel = SDL_BYTESPERPIXEL(obj_texture_format),

        texture_access = SDL_TEXTUREACCESS_STREAMING,

        on_screen_offset_x = 8,
        on_screen_offset_y = 16,

        oam_table_rows = 5,
        oam_table_columns = gb_oam_objects / oam_table_rows,
        oam_table_object_scale = 3,

        tooltip_object_scale = 8,
        tooltip_object_palette_scale = 2,
    };

    struct object_t {
        SDL_Texture* texture = nullptr;
        uint8_t index = 0;
        
        uint8_t y = 0;
        uint8_t x = 0;
        uint8_t tile_index = 0;
        uint16_t tile_address = 0;
        uint8_t palette_index = 0;
        bool horizontal_flip = false;
        bool vertical_flip = false;
        bool priority = false;

        object_t(uint8_t _index,SDL_Renderer* renderer){
            texture = SDL_CreateTexture(renderer,obj_texture_format,texture_access,gb_object_width,gb_object_max_height);
            SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
            index = _index;
        }

        object_t(object_t&& v) noexcept {
            memcpy(this,&v,sizeof(object_t));
            v.texture = nullptr;
        }

        object_t(const object_t& v) = delete;

        ~object_t(){
            SDL_DestroyTexture(texture);
        }

        void clear(){
            clear_texture(texture,gb_object_max_height);
            y = 0;
            x = 0;
            tile_index = 0;
            tile_address = 0;
            palette_index = 0;
            horizontal_flip = false;
            vertical_flip = false;
            priority = false;
        }
    };

    gb_t* gb = nullptr;

    SDL_Texture* bg_texture = nullptr;

    float min_scale = 1.0f;
    float max_scale = 10.0f;
    float scale = min_scale;

    bool show_offscreen = true;
    bool show_outline = true;

    float input_scalar_width = 0.0f;
    int input_scalar_step = 1;
    int input_scalar_step_fast = 100;

    ImVec2 oam_table_size{0.0f,0.0f};

    uint32_t outline_color = 0;
    uint32_t outline_hovered_color = 0;
    uint32_t obj_bg_color = 0;
    uint32_t border_color = 0;
    uint32_t border_hovered_color = 0;

    ImVec2 bg_size{0.0f,0.0f};
    ImVec2 bg_uv0{0.0f,0.0f};
    ImVec2 bg_uv1{0.0f,0.0f};
    ImVec2 bg_offset{0.0f,0.0f};

    bool cgb_mode = false;
    bool obj_priority_mode = false;
    bool object_size = false;
    ImVec2 object_texture_uv0{0.0f,0.0f};
    ImVec2 object_texture_uv1{1.0f,0.5f};
    obj_palette_t obj_palette;
    uint8_t oam[gb_oam_length] = {0};
    uint8_t vram[gb_vram_length] = {0};

    std::vector<object_t> objects;
    std::vector<object_t*> objects_sorted;
    object_t* oam_table_object_hovered = nullptr;

    gb_ppu_handler_t callback_handler = {callback,this,gb_vblank_scanline,0,nullptr};

    bool open = false;

    static void callback(void* data);

    
    void load_bg_texture();


    void update_bg_metrics();

    void update_object_texture(object_t& object);

    void update_objects();


    void render_object_tooltip(object_t* object);

    void render_oam_table();

    void render_oam_screen();

public:

    object_viewer_t(gb_t* gb,SDL_Renderer *renderer);

    ~object_viewer_t();

    void render();

    void clear();
    
    void set_open(bool _open){
        if(open == _open) return;
        open = _open;
        if(open){
            gb_add_ppu_handler(gb,&callback_handler);
        }
        else{
            gb_remove_ppu_handler(gb,&callback_handler);
        }
    }

    bool get_open() const {
        return open;
    }
};
