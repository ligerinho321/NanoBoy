#pragma once

#include <gui/utils/utils.hpp>

class palette_viewer_t {
private:
    enum{
        palette_scale = 4,
        tooltip_color_scale = 8
    };

    gb_t* gb = nullptr;

    bool cgb_mode = false;
    bg_palette_t bg_palette;
    obj_palette_t obj_palette;
    uint8_t bg_cram[gb_cgb_cram_length] = {0};
    uint8_t obj_cram[gb_cgb_cram_length] = {0};

    float input_scalar_width = 0.0f;
    int input_scalar_step = 1;
    int input_scalar_step_fast = 100;

    uint32_t border_color = 0;
    uint32_t border_hovered_color = 0;

    gb_ppu_handler_t callback_handler = {callback,this,gb_vblank_scanline,0,nullptr};

    bool open = false;

    static void callback(void* data);

    void render_tooltip_color(palette_t& palette,uint8_t* cram,uint8_t col,uint8_t row);

    void render_palette(palette_t& palette,uint8_t* cram);

public:
    palette_viewer_t(gb_t* gb,SDL_Renderer* renderer);

    ~palette_viewer_t();

    void render();

    void clear();

    template<bool thread_safe>
    void set_open(bool _open){
        if(open == _open) return;

        open = _open;
        
        if(open){
            if constexpr (thread_safe){
                gb_thread_safe_add_ppu_handler(gb,&callback_handler);
            }
            else{
                gb_add_ppu_handler(gb,&callback_handler);
            }
        }
        else{
            if constexpr (thread_safe){
                gb_thread_safe_remove_ppu_handler(gb,&callback_handler);
            }
            else{
                gb_remove_ppu_handler(gb,&callback_handler);
            }
        }
    }

    bool get_open() const {
        return open;
    }
};
