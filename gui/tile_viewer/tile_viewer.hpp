#pragma once

#include <gui/utils/utils.hpp>

class tile_viewer_t {
private:
    enum{
        min_size = 2,
        max_size = 128,
        
        texture_min_size = min_size * gb_tile_size,
        texture_max_size = max_size * gb_tile_size,

        texture_access = SDL_TEXTUREACCESS_STREAMING,
        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_bytes_per_pixel = SDL_BYTESPERPIXEL(texture_format),

        tooltip_tile_scale = 8,
        palette_scale = 2,

        data_capacity = max_size * max_size * 16
    };

    enum{
        source_cpu,
        source_rom,
        source_vram,
        source_ram,
        source_wram,
        source_hram,
    };

    enum{
        layout_8x8,
        layout_8x16,
        layout_16x16
    };

    gb_t* gb = nullptr;

    SDL_Texture* texture = nullptr;

    bool cgb_mode;

    std::array<uint8_t,tile_viewer_t::data_capacity> data;
    uint32_t data_length;

    bool obj_palette_selected = false;
    uint8_t palette_index = 0;
    bg_palette_t bg_palette;
    obj_palette_t obj_palette;

    int current_source = 0;
    int current_layout = 0;

    uint32_t address_offset = 0;

    float input_scalar_width = 0.0f;
    int input_scalar_step = 1;
    int size_input_scalar_step = 2;

    const float min_scale = 1.0f;
    const float max_scale = 10.0f;
    float scale = min_scale;

    int columns = tile_viewer_t::min_size;
    int rows = tile_viewer_t::min_size;
    
    bool show_tile_grid = false;

    ImVec2 texture_size;
    ImVec2 texture_uv0;
    ImVec2 texture_uv1;

    uint32_t grid_color = 0;
    uint32_t border_hovered_color = 0;

    gb_ppu_handler_t callback_handler = {ppu_callback,this,gb_vblank_scanline,0,nullptr};

    bool _open = false;

    static void ppu_callback(void* userdata);

    void render_layout8x8(uint8_t* pixels,int pitch,const gb_rgb_t* colors) noexcept;
    void render_layout8x16(uint8_t* pixels,int pitch,const gb_rgb_t* colors) noexcept;
    void render_layout16x16(uint8_t* pixels,int pitch,const gb_rgb_t* colors) noexcept;
    void update_texture() noexcept;

    void update_texture_size() noexcept;
    void update_texture_uv() noexcept;
    void update_data_length() noexcept;
    void update_address_offset() noexcept;

    void event();

    void render_palette();
    void render_grid(ImVec2 start,ImVec2 end);
    void render_tile_tooltip(int row,int column);
    
public:
    tile_viewer_t(gb_t* gb,SDL_Renderer* renderer);

    ~tile_viewer_t();


    void render();


    void open(){
        if(_open) return;

        _open = true;

        gb_thread_safe_add_ppu_handler(gb,&callback_handler);
    }

    void close(){
        if(!_open) return;

        _open = false;

        gb_thread_safe_remove_ppu_handler(gb,&callback_handler);
    }

    bool get_open() const noexcept {
        return _open;
    }
};