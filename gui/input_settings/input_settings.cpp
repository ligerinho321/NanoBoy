#include <gui/input_settings/input_settings.hpp>

void input_profile_t::init(const char** _names,int _count){
    names = _names;
    count = _count;

    keyboard_bindings = new SDL_Scancode[count];
    temp_keyboard_bindings = new SDL_Scancode[count];
    keyboard_bindings_state = new binding_state_t[count];

    controller_bindings = new controller_binding_t[count];
    temp_controller_bindings = new controller_binding_t[count];
    controller_bindings_state = new binding_state_t[count];
}

void input_profile_t::uninit(){
    delete keyboard_bindings;
    delete temp_keyboard_bindings;
    delete keyboard_bindings_state;

    delete controller_bindings;
    delete temp_controller_bindings;
    delete controller_bindings_state;
}

void input_profile_t::clear_bindings_state(){
    memset(keyboard_bindings_state,0x00,sizeof(binding_state_t) * count);
    memset(controller_bindings_state,0x00,sizeof(binding_state_t) * count);
}


void input_settings_t::init(){

    profiles[console_gb].init(gb_joypad_button_names,gb_button_count);
    profiles[console_gba].init(gba_keypad_button_names,gba_button_count);

    load_default();
}

void input_settings_t::uninit(){
    for(int i = 0; i < console_count; ++i){
        profiles[i].uninit();
    }
}


void input_settings_t::load_default(){

    SDL_Scancode* gb_keyboard = profiles[console_gb].keyboard_bindings;

    gb_keyboard[gb_button_down] = SDL_SCANCODE_S;
    gb_keyboard[gb_button_up] = SDL_SCANCODE_W;
    gb_keyboard[gb_button_left] = SDL_SCANCODE_A;
    gb_keyboard[gb_button_right] = SDL_SCANCODE_D;
    gb_keyboard[gb_button_start] = SDL_SCANCODE_P;
    gb_keyboard[gb_button_select] = SDL_SCANCODE_O;
    gb_keyboard[gb_button_b] = SDL_SCANCODE_K;
    gb_keyboard[gb_button_a] = SDL_SCANCODE_L;

    SDL_Scancode* gba_keyboard = profiles[console_gba].keyboard_bindings;

    gba_keyboard[gba_button_a] = SDL_SCANCODE_L;
    gba_keyboard[gba_button_b] = SDL_SCANCODE_K;
    gba_keyboard[gba_button_select] = SDL_SCANCODE_O;
    gba_keyboard[gba_button_start] = SDL_SCANCODE_P;
    gba_keyboard[gba_button_right] = SDL_SCANCODE_D;
    gba_keyboard[gba_button_left] = SDL_SCANCODE_A;
    gba_keyboard[gba_button_up] = SDL_SCANCODE_W;
    gba_keyboard[gba_button_down] = SDL_SCANCODE_S;
    gba_keyboard[gba_button_r] = SDL_SCANCODE_Q;
    gba_keyboard[gba_button_l] = SDL_SCANCODE_E;
}


void input_settings_t::save_keyboard_bindings(input_profile_t& profile,cJSON* console_object){
    cJSON* keyboard_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(console_object,"Keyboard",keyboard_object);

    for(int i = 0; i < profile.count; ++i){
        cJSON* keyboard_binding_number = cJSON_CreateNumber((double)profile.keyboard_bindings[i]);
        cJSON_AddItemToObjectCS(keyboard_object,profile.names[i],keyboard_binding_number);
    }
}

void input_settings_t::save_controller_bindings(input_profile_t& profile,cJSON* console_object){
    cJSON* controller_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(console_object,"Controller",controller_object);

    for(int i = 0; i < profile.count; ++i){
        cJSON* controller_binding_object = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(controller_object,profile.names[i],controller_binding_object);

        save_controller_binding(controller_binding_object,profile.controller_bindings[i]);
    }
}

void input_settings_t::save(cJSON* object){
    cJSON* input_settings_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"Input Settings",input_settings_object);

    for(int i = 0; i < console_count; ++i){

        input_profile_t& profile = profiles[i];

        cJSON* console_object = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(input_settings_object,console_names[i],console_object);

        save_keyboard_bindings(profile,console_object);

        save_controller_bindings(profile,console_object);
    }
}


void input_settings_t::load_keyboard_bindings(input_profile_t& profile,cJSON* console_object){

    cJSON* keyboard_object = cJSON_GetObjectItemCaseSensitive(console_object,"Keyboard");
    
    if(!cJSON_IsObject(keyboard_object)) return;

    for(int i = 0; i < profile.count; ++i){

        cJSON* binding_number = cJSON_GetObjectItemCaseSensitive(keyboard_object,profile.names[i]);

        if(!cJSON_IsNumber(binding_number)) continue;

        int value = cJSON_GetNumberValue(binding_number);

        if(value < SDL_SCANCODE_UNKNOWN || value >= SDL_NUM_SCANCODES) continue;

        profile.keyboard_bindings[i] = (SDL_Scancode)value;
    }
}

void input_settings_t::load_controller_bindings(input_profile_t& profile,cJSON* console_object){

    cJSON* controller_object = cJSON_GetObjectItemCaseSensitive(console_object,"Controller");

    if(!cJSON_IsObject(controller_object)) return;

    for(int i = 0; i < gb_button_count; ++i){

        cJSON* binding_object = cJSON_GetObjectItemCaseSensitive(controller_object,profile.names[i]);

        if(!cJSON_IsObject(binding_object)) continue;

        load_controller_binding(binding_object,profile.controller_bindings[i]);
    }
}

void input_settings_t::load(cJSON* object){

    cJSON* input_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Input Settings");

    if(!cJSON_IsObject(input_settings_object)) return;

    for(int i = 0; i < console_count; ++i){
        cJSON* console_object = cJSON_GetObjectItemCaseSensitive(input_settings_object,console_names[i]);

        if(!cJSON_IsObject(console_object)) continue;

        load_keyboard_bindings(profiles[i],console_object);

        load_controller_bindings(profiles[i],console_object);
    }
}


void input_settings_t::init_binding_frame(){

    if(!current_profile) return;

    if(ImGui::GetIO().WantCaptureKeyboard){
        
        for(int i = 0; i < current_profile->count; ++i){
            binding_state_t& keyboard = current_profile->keyboard_bindings_state[i];
            binding_state_t& controller = current_profile->controller_bindings_state[i];

            keyboard.released = keyboard.down || keyboard.pressed;
            keyboard.down = false;
            keyboard.pressed = false;

            controller.released = controller.down || controller.pressed;
            controller.down = false;
            controller.pressed = false;
        }
    }
    else{
        for(int i = 0; i < current_profile->count; ++i){
            binding_state_t& keyboard = current_profile->keyboard_bindings_state[i];
            binding_state_t& controller = current_profile->controller_bindings_state[i];

            keyboard.down = false;
            keyboard.released = false;

            controller.down = false;
            controller.released = false;
        }
    }
}

void input_settings_t::process_binding_event(SDL_Event& event){

    if(!current_profile || ImGui::GetIO().WantCaptureKeyboard) return;

    switch(event.type){
        case SDL_KEYDOWN:{
            for(int i = 0; i < current_profile->count; ++i){

                binding_state_t& binding_state = current_profile->keyboard_bindings_state[i];

                if(binding_state.pressed || event.key.keysym.scancode == SDL_SCANCODE_UNKNOWN) continue;

                if(current_profile->keyboard_bindings[i] == event.key.keysym.scancode){
                    binding_state.down = true;
                    binding_state.pressed = true;
                }
            }
            break;
        }
        case SDL_KEYUP:{
            for(int i = 0; i < current_profile->count; ++i){

                binding_state_t& binding_state = current_profile->keyboard_bindings_state[i];

                if(!binding_state.pressed || event.key.keysym.scancode == SDL_SCANCODE_UNKNOWN) continue;

                if(current_profile->keyboard_bindings[i] == event.key.keysym.scancode){
                    binding_state.pressed = false;
                    binding_state.released = true;
                }
            }
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:{
            for(int i = 0; i < current_profile->count; ++i){

                controller_binding_t& binding = current_profile->controller_bindings[i];
                binding_state_t& binding_state = current_profile->controller_bindings_state[i];

                if(binding_state.pressed || binding.type != controller_binding_button) continue;

                if(binding.button == event.cbutton.button){
                    binding_state.down = true;
                    binding_state.pressed = true;
                }
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP:{
            for(int i = 0; i < current_profile->count; ++i){

                controller_binding_t& binding = current_profile->controller_bindings[i];
                binding_state_t& binding_state = current_profile->controller_bindings_state[i];

                if(!binding_state.pressed || binding.type != controller_binding_button) continue;

                if(binding.button == event.cbutton.button){
                    binding_state.pressed = false;
                    binding_state.released = true;
                }
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION:{
            for(int i = 0; i < current_profile->count; ++i){

                controller_binding_t& binding = current_profile->controller_bindings[i];
                binding_state_t& binding_state = current_profile->controller_bindings_state[i];

                if(binding.type != controller_binding_axis || binding.axis.index != event.caxis.axis) continue;

                if(binding_state.pressed){
                    if(
                        (abs(event.caxis.value) <= controller_axis_deadzone) ||
                        (binding.axis.negative && event.caxis.value >= 0) ||
                        (!binding.axis.negative && event.caxis.value < 0)
                    ){
                        binding_state.pressed = false;
                        binding_state.released = true;
                    }
                }
                else{
                    if(
                        (abs(event.caxis.value) > controller_axis_deadzone) &&
                        ((binding.axis.negative && event.caxis.value < 0 ) ||
                        (!binding.axis.negative && event.caxis.value >= 0))
                    ){
                        binding_state.down = true;
                        binding_state.pressed = true;
                    }
                }
            }
            break;
        }
    }
}

void input_settings_t::process_capture_event(SDL_Event& event){
    if(!_open || !popup_capture.get_open() || !capture_profile) return;

    if(capture_is_keyboard){
        if(event.type == SDL_KEYDOWN){
            capture_profile->temp_keyboard_bindings[capture_binding] = event.key.keysym.scancode;

            popup_capture.close();
        }
    }
    else{
        if(event.type == SDL_CONTROLLERBUTTONDOWN){
            controller_binding_t& binding = capture_profile->temp_controller_bindings[capture_binding];

            binding.type = controller_binding_button;
            binding.button = event.cbutton.button;

            popup_capture.close();
        }
        else if(event.type == SDL_CONTROLLERAXISMOTION && abs(event.caxis.value) > controller_axis_deadzone){
            controller_binding_t& binding = capture_profile->temp_controller_bindings[capture_binding];
            
            binding.type = controller_binding_axis;
            binding.axis.index = event.caxis.axis;
            binding.axis.negative = event.caxis.value < 0;

            popup_capture.close();
        }
    }
}


void input_settings_t::render_profile(input_profile_t* profile){

    ImVec2 table_size(
        0.0f,
        ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()
    );

    if(ImGui::BeginTable("InputTable",3,ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,table_size)){
        
        ImGui::TableSetupColumn("Button",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Keyboard",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Controller",ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();
        
        ImVec2 button_size(-FLT_MIN,0.0f);

        for(int i = 0; i < profile->count; ++i){

            ImGui::PushID(i);

            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            
            ImGui::AlignTextToFramePadding();

            ImGui::TextUnformatted(profile->names[i]);
            
            ImGui::TableNextColumn();
            
            ImGui::PushID("KeyboardSetup");

            if(ImGui::Button(SDL_GetScancodeName(profile->temp_keyboard_bindings[i]),button_size)){

                profile->temp_keyboard_bindings[i] = SDL_SCANCODE_UNKNOWN;

                capture_profile = profile;
                capture_is_keyboard = true;
                capture_binding = i;

                popup_capture.open(true);
            }

            ImGui::PopID();
            
            ImGui::PushID("ControllerSetup");

            ImGui::TableNextColumn();

            controller_binding_t& controller_binding = profile->temp_controller_bindings[i];

            const char* label = "";

            switch(controller_binding.type){
                case controller_binding_button:{
                    label = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)controller_binding.button);
                    break;
                }
                case controller_binding_axis:{
                    label = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)controller_binding.axis.index);
                    break;
                }
            }

            if(ImGui::Button(label,button_size)){

                controller_binding.type = controller_binding_none;

                capture_profile = profile;
                capture_is_keyboard = false;
                capture_binding = i;

                popup_capture.open(false);
            }

            ImGui::PopID();

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void input_settings_t::render(){
    if(!_open) return;

    if(ImGui::Begin("Input",&_open)){

        if(ImGui::BeginTabBar("ConsoleTabBar")){
            for(int i = 0; i < console_count; ++i){
                if(ImGui::BeginTabItem(console_names[i])){
                    render_profile(profiles + i);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        ImGuiStyle& style = ImGui::GetStyle();

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


void input_settings_t::open() noexcept {
    if(_open) return;

    _open = true;

    for(input_profile_t& profile : profiles){
        memcpy(profile.temp_keyboard_bindings,profile.keyboard_bindings,sizeof(SDL_Scancode) * profile.count);
        memcpy(profile.temp_controller_bindings,profile.controller_bindings,sizeof(controller_binding_t) * profile.count);
    }
}

void input_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        for(input_profile_t& profile : profiles){
            memcpy(profile.keyboard_bindings,profile.temp_keyboard_bindings,sizeof(SDL_Scancode) * profile.count);
            memcpy(profile.controller_bindings,profile.temp_controller_bindings,sizeof(controller_binding_t) * profile.count);
        }
    }
}