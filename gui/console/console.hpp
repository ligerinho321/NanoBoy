#pragma once

#include <gui/utils/utils.hpp>

struct console_savestate_info_t {
    uint64_t timestamp;
    const uint8_t* screenshot;
};

class console_t {
public:
    nanoboy_t* nanoboy = nullptr;

    SDL_Texture* screen_texture = nullptr;

    console_t(nanoboy_t* _nanoboy):nanoboy(_nanoboy){}

    virtual ~console_t(){
        if(screen_texture != nullptr){
            SDL_DestroyTexture(screen_texture);
        }
    }

    void create_screen_texture();

    virtual void update_screen_texture() = 0;

    virtual bool take_screenshot(const char* path) = 0;

    virtual bool insert_cartridge(const char* path) = 0;
    virtual bool insert_cartridge_from_memory(uint8_t* rom,size_t length) = 0;
    virtual void remove_cartridge() = 0;
    
    virtual void increase_speed() = 0;
    virtual void decrease_speed() = 0;
    virtual void set_speed(float speed) = 0;
    virtual float get_speed() = 0;
    virtual void pause() = 0;
    virtual bool paused() = 0;
    virtual void reset() = 0;

    virtual void save_ram(const char* path) = 0;
    virtual void load_ram(const char* path) = 0;

    virtual void save_rtc(const char* path) = 0;
    virtual void load_rtc(const char* path) = 0;

    virtual void save_cheats(const char* path) = 0;
    virtual void load_cheats(const char* path) = 0;

    virtual bool savestate_serialize(const char* path) = 0;
    virtual bool savestate_deserialize(const char* path) = 0;
    virtual bool savestate_get_info(const char* path,console_savestate_info_t* info) = 0;

    virtual void set_rewind_enabled(bool enabled) = 0;
    virtual void set_rewind_capacity(uint32_t capacity) = 0;
    virtual void set_rewind_recording_interval(uint32_t interval) = 0;
    virtual void set_rewind_frame_time(float time) = 0;
    virtual bool start_rewind() = 0;
    virtual bool end_rewind() = 0;
    
    virtual void enable_interframe_blending(bool enabled) = 0;

    virtual size_t read_samples(uint8_t* data,int len) = 0;
    
    virtual void open_debugger() = 0;
    virtual void open_register_viewer() = 0;
    virtual void open_event_viewer() = 0;
    virtual void open_memory_viewer() = 0;
    virtual void open_tilemap_viewer() = 0;
    virtual void open_tile_viewer() = 0;
    virtual void open_object_viewer() = 0;
    virtual void open_palette_viewer() = 0;
    virtual void open_wave_form() = 0;

    virtual void open_cheats() = 0;
    
    virtual bool cartridge_inserted() const noexcept = 0;
    virtual float fps() const noexcept = 0;
    virtual int screen_width() const noexcept = 0;
    virtual int screen_height() const noexcept = 0;
    virtual int screen_bytes_per_pixel() const noexcept = 0;
    virtual int screen_pitch() const noexcept = 0;
    virtual int screen_length() const noexcept = 0;
    
    virtual int type() const noexcept = 0;

    virtual void* data() noexcept = 0;

    virtual void execute_frame() = 0;
    virtual void render_imgui_windows() = 0;
};