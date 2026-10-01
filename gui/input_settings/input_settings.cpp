#include <gui/input_settings/input_settings.hpp>

void input_settings_t::save_keyboard_bindings(cJSON* input_settings_object){

    cJSON* keyboard_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(input_settings_object,"Keyboard",keyboard_object);

    for(int i = 0; i < gb_button_count; ++i){
        cJSON* binding_number = cJSON_CreateNumber((double)keyboard_bindings[i]);
        cJSON_AddItemToObjectCS(keyboard_object,gb_joypad_button_names[i],binding_number);
    }
}

void input_settings_t::save_controller_bindings(cJSON* input_settings_object){

    cJSON* controller_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(input_settings_object,"Controller",controller_object);

    for(int i = 0; i < gb_button_count; ++i){
        cJSON* binding_object = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(controller_object,gb_joypad_button_names[i],binding_object);

        save_controller_binding(binding_object,controller_bindings[i]);
    }
}

void input_settings_t::save(cJSON* object){
    cJSON* input_settings_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"Input Settings",input_settings_object);

    save_keyboard_bindings(input_settings_object);

    save_controller_bindings(input_settings_object);
}


void input_settings_t::load_keyboard_bindings(cJSON* input_settings_object){

    cJSON* keyboard_object = cJSON_GetObjectItemCaseSensitive(input_settings_object,"Keyboard");
    
    if(!cJSON_IsObject(keyboard_object)) return;

    for(int i = 0; i < gb_button_count; ++i){

        cJSON* binding_number = cJSON_GetObjectItemCaseSensitive(keyboard_object,gb_joypad_button_names[i]);

        if(!cJSON_IsNumber(binding_number)) continue;

        int value = cJSON_GetNumberValue(binding_number);

        if(value < SDL_SCANCODE_UNKNOWN || value >= SDL_NUM_SCANCODES) continue;

        keyboard_bindings[i] = (SDL_Scancode)value;
    }
}

void input_settings_t::load_controller_bindings(cJSON* input_settings_object){

    cJSON* controller_object = cJSON_GetObjectItemCaseSensitive(input_settings_object,"Controller");

    if(!cJSON_IsObject(controller_object)) return;

    for(int i = 0; i < gb_button_count; ++i){

        cJSON* binding_object = cJSON_GetObjectItemCaseSensitive(controller_object,gb_joypad_button_names[i]);

        if(!cJSON_IsObject(binding_object)) continue;

        load_controller_binding(binding_object,controller_bindings[i]);
    }
}

void input_settings_t::load(cJSON* object){

    cJSON* input_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Input Settings");

    if(!cJSON_IsObject(input_settings_object)) return;

    load_keyboard_bindings(input_settings_object);

    load_controller_bindings(input_settings_object);
}


void input_settings_t::init_binding_frame(){
    if(ImGui::GetIO().WantCaptureKeyboard){
        
        for(int i = 0; i < gb_button_count; ++i){
            binding_state_t& keyboard = keyboard_bindings_state[i];
            binding_state_t& controller = controller_bindings_state[i];

            keyboard.released = keyboard.down || keyboard.pressed;
            keyboard.down = false;
            keyboard.pressed = false;

            controller.released = controller.down || controller.pressed;
            controller.down = false;
            controller.pressed = false;
        }
    }
    else{
        for(int i = 0; i < gb_button_count; ++i){
            binding_state_t& keyboard = keyboard_bindings_state[i];
            binding_state_t& controller = controller_bindings_state[i];

            keyboard.down = false;
            keyboard.released = false;

            controller.down = false;
            controller.released = false;
        }
    }
}

void input_settings_t::process_binding_event(SDL_Event& event){

    if(ImGui::GetIO().WantCaptureKeyboard) return;

    switch(event.type){
        case SDL_KEYDOWN:{
            for(int i = 0; i < gb_button_count; ++i){

                binding_state_t& binding_state = keyboard_bindings_state[i];

                if(binding_state.pressed || event.key.keysym.scancode == SDL_SCANCODE_UNKNOWN) continue;

                if(keyboard_bindings[i] == event.key.keysym.scancode){
                    binding_state.down = true;
                    binding_state.pressed = true;
                }
            }
            break;
        }
        case SDL_KEYUP:{
            for(int i = 0; i < gb_button_count; ++i){

                binding_state_t& binding_state = keyboard_bindings_state[i];

                if(!binding_state.pressed || event.key.keysym.scancode == SDL_SCANCODE_UNKNOWN) continue;

                if(keyboard_bindings[i] == event.key.keysym.scancode){
                    binding_state.pressed = false;
                    binding_state.released = true;
                }
            }
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:{
            for(int i = 0; i < gb_button_count; ++i){

                controller_binding_t& binding = controller_bindings[i];
                binding_state_t& binding_state = controller_bindings_state[i];

                if(binding_state.pressed || binding.type != controller_binding_button) continue;

                if(binding.button == event.cbutton.button){
                    binding_state.down = true;
                    binding_state.pressed = true;
                }
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP:{
            for(int i = 0; i < gb_button_count; ++i){

                controller_binding_t& binding = controller_bindings[i];
                binding_state_t& binding_state = controller_bindings_state[i];

                if(!binding_state.pressed || binding.type != controller_binding_button) continue;

                if(binding.button == event.cbutton.button){
                    binding_state.pressed = false;
                    binding_state.released = true;
                }
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION:{
            for(int i = 0; i < gb_button_count; ++i){

                controller_binding_t& binding = controller_bindings[i];
                binding_state_t& binding_state = controller_bindings_state[i];

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
    if(!_open || !popup_capture.get_open()) return;

    if(capture_is_keyboard){
        if(event.type == SDL_KEYDOWN){
            temp_keyboard_bindings[capture_binding] = event.key.keysym.scancode;

            popup_capture.close();
        }
    }
    else{
        if(event.type == SDL_CONTROLLERBUTTONDOWN){
            controller_binding_t& binding = temp_controller_bindings[capture_binding];

            binding.type = controller_binding_button;
            binding.button = event.cbutton.button;

            popup_capture.close();
        }
        else if(event.type == SDL_CONTROLLERAXISMOTION && abs(event.caxis.value) > controller_axis_deadzone){
            controller_binding_t& binding = temp_controller_bindings[capture_binding];
            
            binding.type = controller_binding_axis;
            binding.axis.index = event.caxis.axis;
            binding.axis.negative = event.caxis.value < 0;

            popup_capture.close();
        }
    }
}


void input_settings_t::render(){
    if(!_open) return;

    if(ImGui::Begin("Input",&_open)){

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

            for(int i = 0; i < gb_button_count; ++i){

                ImGui::PushID(i);

                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                
                ImGui::AlignTextToFramePadding();

                ImGui::TextUnformatted(gb_joypad_button_names[i]);
                
                ImGui::TableNextColumn();
                
                ImGui::PushID("KeyboardSetup");

                if(ImGui::Button(SDL_GetScancodeName(temp_keyboard_bindings[i]),button_size)){

                    temp_keyboard_bindings[i] = SDL_SCANCODE_UNKNOWN;

                    capture_is_keyboard = true;
                    capture_binding = i;

                    popup_capture.open(true);
                }

                ImGui::PopID();
                
                ImGui::PushID("ControllerSetup");

                ImGui::TableNextColumn();

                controller_binding_t& controller_binding = temp_controller_bindings[i];

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

                    capture_is_keyboard = false;
                    capture_binding = i;

                    popup_capture.open(false);
                }

                ImGui::PopID();

                ImGui::PopID();
            }

            ImGui::EndTable();
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

    temp_keyboard_bindings = keyboard_bindings;
    temp_controller_bindings = controller_bindings;
}

void input_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    if(!discard_changes){
        keyboard_bindings = temp_keyboard_bindings;
        controller_bindings = temp_controller_bindings;
    }
}