#include <gui/savestate/savestate.hpp>

savestate_t::savestate_t(gb_t* gb,SDL_Renderer* renderer,SDL_AudioDeviceID audio_device):gb(gb),audio_device(audio_device){

    for(int i = 0; i < savestate_t::number_of_slots; ++i){
        slots[i].name = "Slot #" + std::to_string(i + 1);
        slots[i].shortcut_save = "Shift+F" + std::to_string(i + 1);
        slots[i].shortcut_load = "F" + std::to_string(i + 1);
        slots[i].thumbnail = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STREAMING,gb_screen_width,gb_screen_height);
    }

    ImGuiStyle& style = ImGui::GetStyle();

    float width = 0.0f;

    float save_width = ImGui::CalcTextSize(str_save).x;
    
    width = gb_max(width,save_width);
    
    float load_width = ImGui::CalcTextSize(str_load).x;
    
    width = gb_max(width,load_width);

    float delete_width = ImGui::CalcTextSize(str_delete).x;

    width = gb_max(width,delete_width);

    button_size.x = width + style.FramePadding.x * 2.0f;
    button_size.y = 0.0f;
}


void savestate_t::load(std::filesystem::path path,std::string rom_name){
    
    for(int i = 0; i < slots.size(); ++i){
        slots[i].path = path / (rom_name + "_" + std::to_string(i + 1) + ".ss");
    }

    update_slots();
}

void savestate_t::unload(){
    for(auto& slot : slots){
        slot.path.clear();
        slot.exists = false;
        slot.last_write_time = (time_t)-1;
        slot.timestamp = 0;
        clear_texture(slot.thumbnail,gb_screen_height);
    }
}


void savestate_t::save_slot(slot_t& slot){
    SDL_PauseAudioDevice(audio_device,true);
    
    gb_savestate_thread_safe_serialize(gb,slot.path.u8string().c_str());
    
    update_slots();

    SDL_PauseAudioDevice(audio_device,false);
}

void savestate_t::load_slot(slot_t& slot){
    SDL_PauseAudioDevice(audio_device,true);
    
    gb_savestate_thread_safe_deserialize(gb,slot.path.u8string().c_str());

    SDL_PauseAudioDevice(audio_device,false);
}

void savestate_t::delete_slot(slot_t& slot){
    try{
        std::filesystem::remove(slot.path);

        update_slots();
    }
    catch(std::exception& exception){
        gb_printf_error(exception.what());
    }
}


void savestate_t::update_slots(){

    for(auto& slot : slots){

        bool exists = std::filesystem::exists(slot.path);

        if(slot.exists && !exists){
            slot.last_write_time = (size_t)-1;
            slot.timestamp = 0;
            clear_texture(slot.thumbnail,gb_screen_height);
        }

        slot.exists = exists;

        if(!slot.exists) continue;

        time_t last_write_time = get_file_last_write_time(slot.path);

        if(slot.last_write_time == last_write_time) continue;

        slot.last_write_time = last_write_time;

        gb_savestate_info_t info = {0};

        if(!gb_savestate_get_info(slot.path.u8string().c_str(),&info)) continue;

        slot.timestamp = info.timestamp;

        uint8_t* pixels = nullptr;
        int pitch = 0;
        
        if(SDL_LockTexture(slot.thumbnail,nullptr,(void**)&pixels,&pitch) < 0){
            printf("SDL_LockTexture: %s\n",SDL_GetError());
            continue;
        }

        memcpy(pixels,info.thumbnail,gb_screen_length);

        SDL_UnlockTexture(slot.thumbnail);
    }

    last_update_time = std::chrono::steady_clock::now();

}


void savestate_t::event(){

    if(!gb->cartridge_inserted) return;

    ImGuiIO& io = ImGui::GetIO();

    std::chrono::nanoseconds elapsed = std::chrono::steady_clock::now() - last_update_time;

    if(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 1){
        update_slots();
    }

    //SaveState
    if(io.KeyShift){
        for(int i = 0; i < number_of_slots; ++i){
            if(ImGui::IsKeyDown((ImGuiKey)(ImGuiKey_F1 + i))){
                save_slot(slots[i]);
            }
        }
    }
    //LoadState
    else{
        for(int i = 0; i < number_of_slots; ++i){
            if(slots[i].exists && ImGui::IsKeyDown((ImGuiKey)(ImGuiKey_F1 + i))){
                load_slot(slots[i]);
            }
        }
    }
}


void savestate_t::render_menu_bar(){

    if(ImGui::BeginMenu("Save State",gb->cartridge_inserted)){

        for(int i = 0; i < number_of_slots; ++i){

            ImGui::PushID(i);

            const char* label = slots[i].exists ? get_time_formated(slots[i].last_write_time) : "Empty";
            
            if(ImGui::MenuItem(label,slots[i].shortcut_save.c_str())){
                save_slot(slots[i]);
            }

            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Load State",gb->cartridge_inserted)){

        for(int i = 0; i < number_of_slots; ++i){

            ImGui::PushID(i);

            const char* label = slots[i].exists ? get_time_formated(slots[i].last_write_time) : "Empty";
            
            if(ImGui::MenuItem(label,slots[i].shortcut_load.c_str(),nullptr,slots[i].exists)){
                load_slot(slots[i]);
            }

            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if(ImGui::MenuItem("Save State Menu",nullptr,nullptr,gb->cartridge_inserted)){
        open = true;
    }
}

void savestate_t::render(){

    if(!open) return;

    if(!gb->cartridge_inserted){
        open = false;
        return;
    }

    if(ImGui::Begin("Save State Menu",&open)){

        ImVec2 table_size(
            ImGui::GetContentRegionAvail().x,
            0.0f
        );

        if(ImGui::BeginTable("SlotsTable",3,ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH,table_size)){

            ImGui::TableSetupColumn("Thumbnail",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Information",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Control",ImGuiTableColumnFlags_WidthFixed);

            for(int i = 0; i < savestate_t::number_of_slots; ++i){

                ImGui::TableNextRow(ImGuiTableRowFlags_None);

                ImGui::TableNextColumn();
                
                ImGui::Image((ImTextureRef)slots[i].thumbnail,thumbnail_size);
                
                ImGui::TableNextColumn();

                ImGui::TextUnformatted(slots[i].name.c_str());

                if(slots[i].exists){
                    ImGui::TextUnformatted(get_time_formated(slots[i].last_write_time));
                }
                else{
                    ImGui::TextUnformatted("Empty");
                }

                ImGui::TableNextColumn();

                ImGui::PushID(i);

                if(ImGui::Button(str_save,button_size)){
                    save_slot(slots[i]);
                }

                ImGui::BeginDisabled(!slots[i].exists);

                if(ImGui::Button(str_load,button_size)){
                    load_slot(slots[i]);
                }
                
                if(ImGui::Button(str_delete,button_size)){
                    delete_slot(slots[i]);
                }

                ImGui::EndDisabled();

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

    }
    ImGui::End();
}