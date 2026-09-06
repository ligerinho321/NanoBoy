#include <gui/savestate/savestate.hpp>
#include <gui/nanoboy/nanoboy.hpp>

static const char* str_save = "Save";
static const char* str_load = "Load";
static const char* str_delete = "Delete";


savestate_t::savestate_t(nanoboy_t* nanoboy):nanoboy(nanoboy){

    for(int i = 0; i < savestate_t::number_of_slots; ++i){
        slots[i].name = "Slot #" + std::to_string(i + 1);
        slots[i].shortcut_save = "Shift+F" + std::to_string(i + 1);
        slots[i].shortcut_load = "F" + std::to_string(i + 1);
        slots[i].screenshot = SDL_CreateTexture(nanoboy->renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STREAMING,gb_screen_width,gb_screen_height);
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
    
    for(int i = 0; i < savestate_t::number_of_slots; ++i){
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
        clear_texture(slot.screenshot,gb_screen_height);
    }
}


void savestate_t::save_slot(int index){    
    if(gb_savestate_serialize(nanoboy->gb,slots[index].path.u8string().c_str())){

        nanoboy->notification_manager->push_notification("State #%d Saved",index);
    }
    else{
        nanoboy->notification_manager->push_notification("Failed To Save State #%d",index);
    }
    
    update_slots();
}

void savestate_t::load_slot(int index){
    SDL_LockAudioDevice(nanoboy->audio_device);

    if(gb_savestate_deserialize(nanoboy->gb,slots[index].path.u8string().c_str())){

        nanoboy->notification_manager->push_notification("State #%d Loaded",index);
    }
    else{
        nanoboy->notification_manager->push_notification("Failed To Load State #%d",index);
    }

    SDL_UnlockAudioDevice(nanoboy->audio_device);
}

void savestate_t::delete_slot(int index){
    try{
        std::filesystem::remove(slots[index].path);

        nanoboy->notification_manager->push_notification("State #%d Deleted",index);

        update_slots();
    }
    catch(std::exception& exception){

        nanoboy->notification_manager->push_notification("Failed To Delete State #%d",index);

        gb_printf_error(exception.what());
    }
}


void savestate_t::update_slots(){

    for(auto& slot : slots){

        bool exists = std::filesystem::exists(slot.path);

        if(slot.exists && !exists){
            slot.last_write_time = (size_t)-1;
            slot.timestamp = 0;
            clear_texture(slot.screenshot,gb_screen_height);
        }

        slot.exists = exists;

        if(!slot.exists) continue;

        time_t last_write_time = get_file_last_write_time(slot.path);

        if(slot.last_write_time == last_write_time) continue;

        slot.last_write_time = last_write_time;

        gb_savestate_info_t info{};

        if(!gb_savestate_get_info(slot.path.u8string().c_str(),&info)) continue;

        slot.timestamp = info.timestamp;

        uint8_t* pixels = nullptr;
        int pitch = 0;
        
        if(SDL_LockTexture(slot.screenshot,nullptr,(void**)&pixels,&pitch) < 0){
            printf("SDL_LockTexture: %s\n",SDL_GetError());
            continue;
        }

        memcpy(pixels,info.screenshot,info.screenshot_length);

        SDL_UnlockTexture(slot.screenshot);
    }

    last_update_time = std::chrono::steady_clock::now();

}


void savestate_t::event(SDL_Event& event){

    if(!nanoboy->gb->cartridge_inserted) return;

    std::chrono::nanoseconds elapsed = std::chrono::steady_clock::now() - last_update_time;

    if(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 1){
        update_slots();
    }

    if(event.type == SDL_KEYDOWN){
        //SaveState
        if(SDL_GetModState() & KMOD_SHIFT){
            for(int i = 0; i < number_of_slots; ++i){
                if(event.key.keysym.scancode == SDL_SCANCODE_F1 + i){
                    save_slot(i);
                }
            }
        }
        //LoadState
        else{
            for(int i = 0; i < number_of_slots; ++i){
                if(slots[i].exists && event.key.keysym.scancode == SDL_SCANCODE_F1 + i){
                    load_slot(i);
                }
            }
        }
    }
}


void savestate_t::render_menu_bar(){

    if(ImGui::BeginMenu("Save State",nanoboy->gb->cartridge_inserted)){

        for(int i = 0; i < number_of_slots; ++i){

            ImGui::PushID(i);

            std::string label = std::to_string(i + 1) + ". " + (slots[i].exists ? get_time_formated(slots[i].last_write_time) : "empty");
            
            if(ImGui::MenuItem(label.c_str(),slots[i].shortcut_save.c_str())){
                save_slot(i);
            }

            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Load State",nanoboy->gb->cartridge_inserted)){

        for(int i = 0; i < number_of_slots; ++i){

            ImGui::PushID(i);

            std::string label = std::to_string(i + 1) + ". " + (slots[i].exists ?  get_time_formated(slots[i].last_write_time) : "empty");
            
            if(ImGui::MenuItem(label.c_str(),slots[i].shortcut_load.c_str(),nullptr,slots[i].exists)){
                load_slot(i);
            }

            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    if(ImGui::MenuItem("Save State Menu",nullptr,nullptr,nanoboy->gb->cartridge_inserted)){
        open = true;
    }
}

void savestate_t::render(){

    if(!open) return;

    if(!nanoboy->gb->cartridge_inserted){
        open = false;
        return;
    }

    if(ImGui::Begin("Save State Menu",&open)){

        if(ImGui::BeginTable("SlotsTable",3,ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH)){

            ImGui::TableSetupColumn("Screenshot",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Information",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Control",ImGuiTableColumnFlags_WidthFixed);

            for(int i = 0; i < savestate_t::number_of_slots; ++i){

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                
                ImGui::Image((ImTextureRef)slots[i].screenshot,screenshot_size);
                
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
                    save_slot(i);
                }

                ImGui::BeginDisabled(!slots[i].exists);

                if(ImGui::Button(str_load,button_size)){
                    load_slot(i);
                }
                
                if(ImGui::Button(str_delete,button_size)){
                    delete_slot(i);
                }

                ImGui::EndDisabled();

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

    }
    ImGui::End();
}