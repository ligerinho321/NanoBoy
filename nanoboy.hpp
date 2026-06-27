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

    void init_directories();
    void init_sdl();
    void init_imgui();

    void event();

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

    
    void gb_run(){
        if(paused || !gb->cartridge_inserted) return;

        uint64_t frame = gb->ppu.frame_count;
        
        while(frame == gb->ppu.frame_count){
            gb_cpu_execute(&gb->cpu);
        }

        uint8_t* pixels = NULL;
        int pitch = 0;
        SDL_LockTexture(screen->texture,NULL,(void**)&pixels,&pitch);
        
        memcpy(pixels,gb->ppu.screen,sizeof(gb->ppu.screen));

        SDL_UnlockTexture(screen->texture);
    }


    std::string get_rom_save_path(){
        std::filesystem::path path = saves_path / rom_name;
        path.replace_extension(".s");
        return path.string();
    }

    std::string get_rom_savestate_path(){
        std::filesystem::path path = savestates_path / rom_name;
        path.replace_extension(".ss");
        return path.string();
    }

    std::string get_rom_cheat_path(){
        std::filesystem::path path = cheats_path / rom_name;
        path.replace_extension(".json");
        return path.string();
    }

    void insert_cartridge(std::filesystem::path path);
    void remove_cartridge();

    void render_main_menu_bar();

    void imgui_render(){
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        render_main_menu_bar();

        file_selector->render();
        screen->render();
        cheats->render();
        tilemap_viewer->render();
        object_viewer->render();
        palette_viewer->render();
        wave_form->render();

        ImGui::Render();
    }

    void sdl_render(){
        SDL_RenderClear(renderer);
        
        if(screen->mode == screen->embedded_mode){
            SDL_RenderCopy(renderer,screen->texture,NULL,&screen->embedded_rect);
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        
        SDL_RenderPresent(renderer);
    }

    void run();
};
