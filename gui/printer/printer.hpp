#pragma once

#include <gui/utils/utils.hpp>
#include <gui/file_dialog/file_save_dialog.hpp>

class printer_t {
private:
    enum{
        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_bytes_per_pixel = SDL_BYTESPERPIXEL(texture_format),
        texture_access = SDL_TEXTUREACCESS_STREAMING,
        texture_width = gb_screen_width,
        texture_expand_height = gb_screen_height,
        buffer_expand_size = gb_printer_image_pitch * texture_expand_height
    };

    gb_t* gb = nullptr;

    file_save_dialog_t file_save;

    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    int texture_height = 0;
    int texture_max_height = 0;

    std::mutex mutex;
    std::vector<uint8_t> buffer;
    std::atomic<bool> update_texture = false;

    bool padding_enabled = false;
    
    bool open = false;

    static void gb_printer_callback(void* userdata,const uint8_t* data,int len);

    static void file_save_callback(void* userdata,std::filesystem::path path);
    
    const char* get_print_file_name() const noexcept;

    void update();

public:
    printer_t(gb_t* _gb,SDL_Renderer* _renderer);

    ~printer_t(){
        if(texture != nullptr){
            SDL_DestroyTexture(texture);
        }
    }

    void render();

    void set_open(bool _open) noexcept;

    bool get_open() const noexcept {
        return open;
    }
};