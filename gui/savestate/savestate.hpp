#pragma once

#include <gui/utils/utils.hpp>

class savestate_t {
private:
    enum{
        number_of_slots = 10,
    };

    struct slot_t {
        std::string name;
        std::string shortcut_save;
        std::string shortcut_load;

        std::filesystem::path path;
        
        bool exists = false;
        time_t last_write_time = (time_t)-1;

        uint64_t timestamp = 0;
        SDL_Texture* screenshot = nullptr;

        ~slot_t(){
            if(screenshot != nullptr){
                SDL_DestroyTexture(screenshot);
            }
        }
    };

    struct slot_shortcurt_t {
        std::string save;
        std::string load;
    };

    gb_t* gb = nullptr;

    std::array<slot_t,savestate_t::number_of_slots> slots;

    ImVec2 screenshot_size{gb_screen_width,gb_screen_height};
    ImVec2 button_size{0.0f,0.0f};

    std::chrono::steady_clock::time_point last_update_time;

    bool open = false;

    void save_slot(slot_t& slot);
    void load_slot(slot_t& slot);
    void delete_slot(slot_t& slot);

    void update_slots();

public:
    savestate_t(gb_t* gb,SDL_Renderer* renderer);

    void load(std::filesystem::path path,std::string rom_name);
    void unload();

    void event(SDL_Event& event);

    void render_menu_bar();

    void render();

    void set_open(bool _open) noexcept {
        open = _open;
    }

    bool get_open() const noexcept {
        return open;
    }
};