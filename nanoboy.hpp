#pragma once

#include "core/gb.h"
#include "gui/utils.hpp"
#include "gui/cheats.hpp"
#include "gui/file_selector.hpp"
#include "gui/object_viewer.hpp"
#include "gui/palette_viewer.hpp"
#include "gui/screen.hpp"
#include "gui/tilemap_viewer.hpp"
#include "gui/wave_form.hpp"

class nanoboy_t {
private:
    std::filesystem::path main_folder_path;
    std::filesystem::path saves_path;
    std::filesystem::path savestates_path;
    std::filesystem::path cheats_path;

    std::filesystem::path rom_path;
    std::string rom_name;

    std::string get_rom_save_path() const {
        std::filesystem::path path = saves_path / rom_name;
        path.replace_extension(".s");
        return path.u8string();
    }

    std::string get_rom_savestate_path() const {
        std::filesystem::path path = savestates_path / rom_name;
        path.replace_extension(".ss");
        return path.u8string();
    }

    std::string get_rom_cheat_path() const {
        std::filesystem::path path = cheats_path / rom_name;
        path.replace_extension(".json");
        return path.u8string();
    }

    std::string get_imgui_ini_path() const {
        std::filesystem::path path = main_folder_path / "imgui.ini";
        return path.u8string();
    }

    void init_directories();
    void init_sdl();
    void init_imgui();

    void save_imgui_ini_settings();
    void load_imgui_ini_settings();

    void event();

    void gb_run();

public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_AudioDeviceID audio_device = 0;

    gb_t* gb = nullptr;
    file_selector_t* file_selector = nullptr;
    screen_t* screen = nullptr;
    cheats_t* cheats = nullptr;
    tilemap_viewer_t* tilemap_viewer = nullptr;
    object_viewer_t* object_viewer = nullptr;
    palette_viewer_t* palette_viewer = nullptr;
    wave_form_t* wave_form = nullptr;

    bool running = false;
    bool paused = false;

    nanoboy_t();
    ~nanoboy_t();

    void insert_cartridge(std::filesystem::path path);
    void remove_cartridge();

    void render_main_menu_bar();
    void imgui_render();
    void sdl_render();

    void run();
};
