#include <gui/shortcut_settings/shortcut_settings.hpp>

static const char* shortcut_names[] = {
    "Open File",
    
    "Take Screenshot",

    "Save State Slot 1",
    "Save State Slot 2",
    "Save State Slot 3",
    "Save State Slot 4",
    "Save State Slot 5",
    "Save State Slot 6",
    "Save State Slot 7",
    "Save State Slot 8",
    "Save State Slot 9",
    "Save State Slot 10",

    "Load State Slot 1",
    "Load State Slot 2",
    "Load State Slot 3",
    "Load State Slot 4",
    "Load State Slot 5",
    "Load State Slot 6",
    "Load State Slot 7",
    "Load State Slot 8",
    "Load State Slot 9",
    "Load State Slot 10",

    "Save State Menu",

    "Exit",

    "Pause",
    "Reset",
    "Increase Speed",
    "Decrease Speed",
    "Rewind (Held)",
    "Cheats",
    "Printer",
    "Power Off",

    "Set Scale 1x",
    "Set Scale 2x",
    "Set Scale 3x",
    "Set Scale 4x",
    "Set Scale 5x",
    "Set Scale 6x",
    "Set Scale 7x",
    "Set Scale 8x",
    "Set Scale 9x",
    "Toggle FullSreen",

    "Toggle Floating",
    "Toggle Aspect Ratio",
    "Toggle Interger Scale",
    "Toggle Interframe Blending",
    "Toggle Bilinear Filtering",
    "Toggle Disable Background",
    "Toggle Disable Objects",

    "DMG Palette Settings",
    "Boot Settings",
    "Rewind Settings",
    "Input Settings",
    "Shortcut Settings",

    "Debugger",
    "Register Viewer",
    "Event Viewer",
    "Memory Viewer",
    "Tilemap Viewer",
    "Tile Viewer",
    "Object Viewer",
    "Palette Viewer",
    "Wave Form"
};


void shortcut_settings_t::load_default(){

    shortcut_t& open_file = shortcuts[shortcut_settings_t::open_file];
    open_file.keyboard.scancode = SDL_SCANCODE_O;
    open_file.keyboard.modifiers = KMOD_CTRL;
    open_file.keyboard_string = get_keyboard_binding_string(open_file.keyboard);

    shortcut_t& take_screenshot = shortcuts[shortcut_settings_t::take_screenshot];
    take_screenshot.keyboard.scancode = SDL_SCANCODE_F12;
    take_screenshot.keyboard.modifiers = KMOD_NONE;
    take_screenshot.keyboard_string = get_keyboard_binding_string(take_screenshot.keyboard);
    
    for(int i = 0; i < 10; ++i){
        SDL_Scancode scancode = (SDL_Scancode)(SDL_SCANCODE_F1 + i);

        shortcut_t& savestate_slot = shortcuts[shortcut_settings_t::savestate_slot1 + i];
        savestate_slot.keyboard.scancode = scancode;
        savestate_slot.keyboard.modifiers = KMOD_SHIFT;
        savestate_slot.keyboard_string = get_keyboard_binding_string(savestate_slot.keyboard);

        shortcut_t& loadstate_slot = shortcuts[shortcut_settings_t::loadstate_slot1 + i];
        loadstate_slot.keyboard.scancode = scancode;
        loadstate_slot.keyboard.modifiers = KMOD_NONE;
        loadstate_slot.keyboard_string = get_keyboard_binding_string(loadstate_slot.keyboard);
    }

    shortcut_t& pause = shortcuts[shortcut_settings_t::pause];
    pause.keyboard.scancode = SDL_SCANCODE_ESCAPE;
    pause.keyboard.modifiers = KMOD_NONE;
    pause.keyboard_string = get_keyboard_binding_string(pause.keyboard);

    shortcut_t& reset = shortcuts[shortcut_settings_t::reset];
    reset.keyboard.scancode = SDL_SCANCODE_R;
    reset.keyboard.modifiers = KMOD_CTRL;
    reset.keyboard_string = get_keyboard_binding_string(reset.keyboard);

    shortcut_t& increase_speed = shortcuts[shortcut_settings_t::increase_speed];
    increase_speed.keyboard.scancode = SDL_SCANCODE_EQUALS;
    increase_speed.keyboard.modifiers = KMOD_NONE;
    increase_speed.keyboard_string = get_keyboard_binding_string(increase_speed.keyboard);

    shortcut_t& decrease_speed = shortcuts[shortcut_settings_t::decrease_speed];
    decrease_speed.keyboard.scancode = SDL_SCANCODE_MINUS;
    decrease_speed.keyboard.modifiers = KMOD_NONE;
    decrease_speed.keyboard_string = get_keyboard_binding_string(decrease_speed.keyboard);

    shortcut_t& rewind_held = shortcuts[shortcut_settings_t::rewind_held];
    rewind_held.keyboard.scancode = SDL_SCANCODE_BACKSPACE;
    rewind_held.keyboard.modifiers = KMOD_NONE;
    rewind_held.keyboard_string = get_keyboard_binding_string(rewind_held.keyboard);

    for(int i = 0; i < 9; ++i){
        shortcut_t& set_scale = shortcuts[shortcut_settings_t::set_scale_1x + i];
        set_scale.keyboard.scancode = (SDL_Scancode)(SDL_SCANCODE_1 + i);
        set_scale.keyboard.modifiers = KMOD_ALT;
        set_scale.keyboard_string = get_keyboard_binding_string(set_scale.keyboard);
    }

    shortcut_t& toggle_fullscreen = shortcuts[shortcut_settings_t::toggle_fullscreen];
    toggle_fullscreen.keyboard.scancode = SDL_SCANCODE_F11;
    toggle_fullscreen.keyboard.modifiers = KMOD_NONE;
    toggle_fullscreen.keyboard_string = get_keyboard_binding_string(toggle_fullscreen.keyboard); 
}


void shortcut_settings_t::save(cJSON* object){
    cJSON* shortcut_settings_object = cJSON_CreateObject();

    cJSON_AddItemToObjectCS(object,"Shortcut Settings",shortcut_settings_object);

    for(int i = 0; i < shortcut_settings_t::id_count; ++i){
        
        shortcut_t& shortcut = shortcuts[i];

        cJSON* shortcut_object = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(shortcut_settings_object,shortcut_names[i],shortcut_object);

        cJSON* keyboard_binding_object = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(shortcut_object,"Keyboard",keyboard_binding_object);

        save_keyboard_binding(keyboard_binding_object,shortcut.keyboard);

        cJSON* controller_binding_object = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(shortcut_object,"Controller",controller_binding_object);

        save_controller_binding(controller_binding_object,shortcut.controller);
    }
}

void shortcut_settings_t::load(cJSON* object){
    cJSON* shortcut_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Shortcut Settings");

    if(!cJSON_IsObject(shortcut_settings_object)){
        load_default();
        return;
    }

    for(int i = 0; i < shortcut_settings_t::id_count; ++i){

        shortcut_t& shortcut = shortcuts[i];

        cJSON* shortcut_object = cJSON_GetObjectItemCaseSensitive(shortcut_settings_object,shortcut_names[i]);

        if(!cJSON_IsObject(shortcut_object)) return;

        cJSON* keyboard_binding_object = cJSON_GetObjectItemCaseSensitive(shortcut_object,"Keyboard");
        
        if(cJSON_IsObject(keyboard_binding_object)){
            if(load_keyboard_binding(keyboard_binding_object,shortcut.keyboard)){
                shortcut.keyboard_string = get_keyboard_binding_string(shortcut.keyboard);
            }
        }
        
        cJSON* controller_binding_object = cJSON_GetObjectItemCaseSensitive(shortcut_object,"Controller");
        
        if(cJSON_IsObject(controller_binding_object)){
            if(load_controller_binding(controller_binding_object,shortcut.controller)){
                shortcut.controller_string = get_controller_binding_string(shortcut.controller);
            }
        }
    }
}


void shortcut_settings_t::init_binding_frame(){
    
    if(ImGui::GetIO().WantCaptureKeyboard){
        for(shortcut_state_t& state : shortcuts_state){

            state.keyboard.released = state.keyboard.down || state.keyboard.pressed;
            state.keyboard.down = false;
            state.keyboard.pressed = false;

            state.controller.released = state.controller.down || state.controller.pressed;
            state.controller.down = false;
            state.controller.pressed = false;
        }
    }
    else{
        for(shortcut_state_t& state : shortcuts_state){

            state.keyboard.down = false;
            state.keyboard.released = false;

            state.controller.down = false;
            state.controller.released = false;
        }
    }
}

void shortcut_settings_t::process_binding_event(SDL_Event& event){

    if(ImGui::GetIO().WantCaptureKeyboard) return;

    switch(event.type){
        case SDL_KEYDOWN:{
            shortcut_t* shortcut = shortcuts.begin();
            shortcut_t* shortcut_end = shortcuts.end();

            shortcut_state_t* state = shortcuts_state.begin();

            SDL_Keymod modifiers = keyboard_binding_normalize_modifiers(event.key.keysym.mod);

            while(shortcut != shortcut_end){

                if(!state->keyboard.pressed && shortcut->keyboard.scancode != SDL_SCANCODE_UNKNOWN){
                    if(shortcut->keyboard.modifiers == modifiers && shortcut->keyboard.scancode == event.key.keysym.scancode){
                        state->keyboard.down = true;
                        state->keyboard.pressed = true;
                    }
                }

                ++shortcut;
                ++state;
            }
            break;
        }
        case SDL_KEYUP:{
            shortcut_t* shortcut = shortcuts.begin();
            shortcut_t* shortcut_end = shortcuts.end();

            shortcut_state_t* state = shortcuts_state.begin();

            SDL_Keymod modifiers = keyboard_binding_normalize_modifiers(event.key.keysym.mod);

            while(shortcut != shortcut_end){

                if(state->keyboard.pressed && shortcut->keyboard.scancode != SDL_SCANCODE_UNKNOWN){
                    if(shortcut->keyboard.modifiers != modifiers || shortcut->keyboard.scancode == event.key.keysym.scancode){
                        state->keyboard.pressed = false;
                        state->keyboard.released = true;
                    }
                }

                ++shortcut;
                ++state;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:{
            shortcut_t* shortcut = shortcuts.begin();
            shortcut_t* shortcut_end = shortcuts.end();

            shortcut_state_t* state = shortcuts_state.begin();

            while(shortcut != shortcut_end){

                if(!state->controller.pressed && shortcut->controller.type == controller_binding_button){
                    if(shortcut->controller.button == event.cbutton.button){
                        state->controller.down = true;
                        state->controller.pressed = true;
                    }
                }

                ++shortcut;
                ++state;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP:{
            shortcut_t* shortcut = shortcuts.begin();
            shortcut_t* shortcut_end = shortcuts.end();

            shortcut_state_t* state = shortcuts_state.begin();

            while(shortcut != shortcut_end){

                if(state->controller.pressed && shortcut->controller.type == controller_binding_button){
                    if(shortcut->controller.button == event.cbutton.button){
                        state->controller.pressed = false;
                        state->controller.released = true;
                    }
                }

                ++shortcut;
                ++state;
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION:{
            shortcut_t* shortcut = shortcuts.begin();
            shortcut_t* shortcut_end = shortcuts.end();

            shortcut_state_t* state = shortcuts_state.begin();

            while(shortcut != shortcut_end){

                if(shortcut->controller.type == controller_binding_axis && shortcut->controller.axis.index == event.caxis.axis){
                    if(state->controller.pressed){
                        if(
                            (abs(event.caxis.value) <= controller_axis_deadzone) ||
                            (shortcut->controller.axis.negative && event.caxis.value >= 0) ||
                            (!shortcut->controller.axis.negative && event.caxis.value < 0)
                        ){
                            state->controller.pressed = false;
                            state->controller.released = true;
                        }
                    }
                    else{
                        if(
                            (abs(event.caxis.value) > controller_axis_deadzone) &&
                            ((shortcut->controller.axis.negative && event.caxis.value < 0) ||
                            (!shortcut->controller.axis.negative && event.caxis.value >= 0))
                        ){
                            state->controller.down = true;
                            state->controller.pressed = true;
                        }
                    }
                }

                ++shortcut;
                ++state;
            }
            break;
        }
    }
}

void shortcut_settings_t::process_capture_event(SDL_Event& event){
    if(!_open || !popup_capture.get_open()) return;

    if(capture_is_keyboard){
        if(event.type == SDL_KEYDOWN && (event.key.keysym.scancode < SDL_SCANCODE_LCTRL || event.key.keysym.scancode > SDL_SCANCODE_RGUI)){
            shortcut_t& shortcut = temp_shortcuts[capture_shortcut];               
            
            shortcut.keyboard.scancode = event.key.keysym.scancode;
            shortcut.keyboard.modifiers = keyboard_binding_normalize_modifiers(event.key.keysym.mod);
            
            shortcut.keyboard_string = get_keyboard_binding_string(shortcut.keyboard);

            popup_capture.close();
        }
    }
    else{
        if(event.type == SDL_CONTROLLERBUTTONDOWN){
            shortcut_t& shortcut = temp_shortcuts[capture_shortcut];
            
            shortcut.controller.type = controller_binding_button;
            shortcut.controller.button = event.cbutton.button;

            shortcut.controller_string = get_controller_binding_string(shortcut.controller);

            popup_capture.close();
        }
        else if(event.type == SDL_CONTROLLERAXISMOTION && abs(event.caxis.value) > controller_axis_deadzone){
            shortcut_t& shortcut = temp_shortcuts[capture_shortcut];

            shortcut.controller.type = controller_binding_axis;
            shortcut.controller.axis.index = event.caxis.axis;
            shortcut.controller.axis.negative = event.caxis.value < 0;
            
            shortcut.controller_string = get_controller_binding_string(shortcut.controller);

            popup_capture.close();
        }
    }
}


void shortcut_settings_t::render(){
    if(!_open) return;

    if(ImGui::Begin("Shortcut",&_open)){

        ImGuiStyle& style = ImGui::GetStyle();

        ImVec2 table_size(
            0.0f,
            ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()
        );

        if(ImGui::BeginTable("ShortcutTable",3,ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,table_size)){
            
            ImGui::TableSetupColumn("Action",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Keyboard",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Controller",ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            ImVec2 button_size(-FLT_MIN,0.0f);

            for(int i = 0; i < shortcut_settings_t::id_count; ++i){
                ImGui::PushID(i);

                shortcut_t& shortcut = temp_shortcuts[i];

                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();

                ImGui::TextUnformatted(shortcut_names[i]);

                ImGui::TableNextColumn();

                ImGui::PushID("KeyboardBinding");

                if(ImGui::Button(shortcut.keyboard_string.c_str(),button_size)){
                    
                    shortcut.keyboard.scancode = SDL_SCANCODE_UNKNOWN;
                    shortcut.keyboard.modifiers = KMOD_NONE;

                    shortcut.keyboard_string.clear();

                    capture_is_keyboard = true;
                    capture_shortcut = i;

                    popup_capture.open(true);
                }

                ImGui::PopID();

                ImGui::TableNextColumn();

                ImGui::PushID("ControllerBinding");

                if(ImGui::Button(shortcut.controller_string.c_str(),button_size)){
                    
                    shortcut.controller.type = controller_binding_none;

                    shortcut.controller_string.clear();

                    capture_is_keyboard = false;
                    capture_shortcut = i;
                    
                    popup_capture.open(false);
                }

                ImGui::PopID();
                
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        const char* str_ok = "Ok";
        const char* str_cancel = "Cancel";

        float button_padding = style.FramePadding.x * 2.0f;
        float ok_button_width = ImGui::CalcTextSize(str_ok).x + button_padding;
        float cancel_button_width = ImGui::CalcTextSize(str_cancel).x + button_padding;

        ImGui::SetCursorPosX((ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x) - (ok_button_width + cancel_button_width + style.ItemSpacing.x));
        
        if(ImGui::Button(str_ok)) close(false);

        ImGui::SameLine();

        if(ImGui::Button(str_cancel)) close(true);

        popup_capture.render();
    }
    ImGui::End();

    if(!_open) close(true);
}


void shortcut_settings_t::open() noexcept {
    if(_open) return;

    _open = true;

    temp_shortcuts = shortcuts;
}

void shortcut_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        shortcuts = temp_shortcuts;
    }
}