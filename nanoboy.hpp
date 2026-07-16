#pragma once

#include <core/gb.h>

#include <gui/utils/utils.hpp>

#include <gui/file_dialog/file_selector_dialog.hpp>

#include <gui/cheats/cheats.hpp>
#include <gui/printer/printer.hpp>

#include <gui/object_viewer/object_viewer.hpp>
#include <gui/palette_viewer/palette_viewer.hpp>
#include <gui/screen/screen.hpp>
#include <gui/tilemap_viewer/tilemap_viewer.hpp>

#include <gui/wave_form/wave_form.hpp>

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

    std::string get_rom_rtc_path() const {
        std::filesystem::path path = saves_path / rom_name;
        path.replace_extension(".rtc");
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
    gb_t* gb = nullptr;
    
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_AudioDeviceID audio_device = 0;
    
    file_selector_t* file_selector = nullptr;
    
    screen_t* screen = nullptr;
    
    cheats_t* cheats = nullptr;
    printer_t* printer = nullptr;

    tilemap_viewer_t* tilemap_viewer = nullptr;
    object_viewer_t* object_viewer = nullptr;
    palette_viewer_t* palette_viewer = nullptr;
    
    wave_form_t* wave_form = nullptr;

    bool running = false;

    nanoboy_t();
    ~nanoboy_t();

    void insert_cartridge(std::filesystem::path path);
    void remove_cartridge();

    void render_main_menu_bar();
    void imgui_render();
    void sdl_render();

    void run();
};
