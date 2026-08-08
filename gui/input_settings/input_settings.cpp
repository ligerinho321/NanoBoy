#include <gui/input_settings/input_settings.hpp>

input_settings_t::input_settings_t(){
    update_window_size_constraints();
}

input_settings_t::~input_settings_t(){
    for(auto controller : controller_devices){
        SDL_GameControllerClose(controller);
    }
}


void input_settings_t::update_window_size_constraints(){
    
    ImGuiStyle& style = ImGui::GetStyle();

    float title_bar_height = ImGui::GetFrameHeight();
    
    float table_header_height = ImGui::GetTextLineHeight() + style.CellPadding.y * 2.0f;
    
    float table_content_height = (ImGui::GetFrameHeight() + style.CellPadding.y * 2.0f) * gb_button_count;
    
    float table_height = table_header_height + table_content_height;

    float button_height = ImGui::GetFrameHeightWithSpacing();

    float window_height = title_bar_height + table_height + button_height + style.WindowPadding.y * 2.0f;

    window_min_size.x = style.WindowMinSize.x;
    window_min_size.y = window_height;

    window_max_size.x = FLT_MAX;
    window_max_size.y = window_height;
}


void input_settings_t::clear_bindings(){
    for(int i = 0; i < gb_button_count; ++i){
        keyboard_bindings[i] = SDL_SCANCODE_UNKNOWN;
        controller_bindings[i].type = input_settings_t::controller_binding_none;
    }
}


static const char* controller_binding_type_names[] = {
    "None",
    "Button",
    "Axis"
};

int input_settings_t::get_controller_binding_type_from_string(const char* string){
    for(int i = 0; i < input_settings_t::controller_binding_count; ++i){
        if(!strcmp(controller_binding_type_names[i],string)){
            return i;
        }
    }
    return -1;
}

const char* input_settings_t::get_controller_binding_type_string(int type){
    if(type < input_settings_t::controller_binding_none && type >= input_settings_t::controller_binding_count){
        return "";
    }
    return controller_binding_type_names[type];
}


void input_settings_t::save_keyboard_bindings(cJSON* input_settings_object){

    cJSON* keyboard_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(input_settings_object,"Keyboard",keyboard_object);

    for(int i = 0; i < gb_button_count; ++i){

        const char* button_name = gb_joypad_get_button_name(i);

        cJSON* binding_number = cJSON_CreateNumber((double)keyboard_bindings[i]);
        cJSON_AddItemToObjectCS(keyboard_object,button_name,binding_number);
    }
}

void input_settings_t::save_controller_bindings(cJSON* input_settings_object){

    cJSON* controller_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(input_settings_object,"Controller",controller_object);

    for(int i = 0; i < gb_button_count; ++i){

        const char* button_name = gb_joypad_get_button_name(i);

        cJSON* binding_object = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(controller_object,button_name,binding_object);

        controller_binding_t* binding = controller_bindings + i;

        cJSON* type_string = cJSON_CreateStringReference(get_controller_binding_type_string(binding->type));
        cJSON_AddItemToObjectCS(binding_object,"Type",type_string);

        switch(binding->type){
            case input_settings_t::controller_binding_button:{
                cJSON* value_number = cJSON_CreateNumber((double)binding->button);
                cJSON_AddItemToObjectCS(binding_object,"Value",value_number);
                break;
            }
            case input_settings_t::controller_binding_axis:{
                cJSON* index_number = cJSON_CreateNumber((double)binding->axis.index);
                cJSON_AddItemToObjectCS(binding_object,"Index",index_number);

                cJSON* negative_bool = cJSON_CreateBool(binding->axis.negative);
                cJSON_AddItemToObjectCS(binding_object,"Negative",negative_bool);
                break;
            }
        }
    }
}

void input_settings_t::save(cJSON* object){

    cJSON* input_settings_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(object,"Input settings",input_settings_object);

    save_keyboard_bindings(input_settings_object);

    save_controller_bindings(input_settings_object);
}


void input_settings_t::load_keyboard_bindings(cJSON* input_settings_object){

    cJSON* keyboard_object = cJSON_GetObjectItemCaseSensitive(input_settings_object,"Keyboard");
    
    if(!keyboard_object || !cJSON_IsObject(keyboard_object)) return;

    for(int i = 0; i < gb_button_count; ++i){

        const char* button_name = gb_joypad_get_button_name(i);

        cJSON* binding_number = cJSON_GetObjectItemCaseSensitive(keyboard_object,button_name);

        if(!binding_number || !cJSON_IsNumber(binding_number)) continue;

        int value = cJSON_GetNumberValue(binding_number);

        if(value < SDL_SCANCODE_UNKNOWN || value >= SDL_NUM_SCANCODES) continue;

        keyboard_bindings[i] = (SDL_Scancode)value;
    }
}

void input_settings_t::load_controller_bindings(cJSON* input_settings_object){

    cJSON* controller_object = cJSON_GetObjectItemCaseSensitive(input_settings_object,"Controller");

    if(!controller_object || !cJSON_IsObject(controller_object)) return;

    for(int i = 0; i < gb_button_count; ++i){

        const char* button_name = gb_joypad_get_button_name(i);

        cJSON* binding_object = cJSON_GetObjectItemCaseSensitive(controller_object,button_name);

        if(!binding_object || !cJSON_IsObject(binding_object)) continue;

        cJSON* type_string = cJSON_GetObjectItemCaseSensitive(binding_object,"Type");

        if(!type_string || !cJSON_IsString(type_string)) continue;

        int type = get_controller_binding_type_from_string(cJSON_GetStringValue(type_string));

        if(type < input_settings_t::controller_binding_none && type >= input_settings_t::controller_binding_count) continue;

        controller_binding_t* binding = controller_bindings + i;

        switch(type){
            case input_settings_t::controller_binding_button:{
                
                cJSON* value_number = cJSON_GetObjectItemCaseSensitive(binding_object,"Value");

                if(!value_number || !cJSON_IsNumber(value_number)) continue;

                binding->button = (uint8_t)cJSON_GetNumberValue(value_number);

                break;
            }
            case input_settings_t::controller_binding_axis:{

                cJSON* index_number = cJSON_GetObjectItemCaseSensitive(binding_object,"Index");

                if(!index_number || !cJSON_IsNumber(index_number)) continue;

                int index = cJSON_GetNumberValue(index_number);

                if(index < SDL_CONTROLLER_AXIS_LEFTX || index >= SDL_CONTROLLER_AXIS_MAX) continue;

                cJSON* negative_bool = cJSON_GetObjectItemCaseSensitive(binding_object,"Negative");

                if(!negative_bool || !cJSON_IsBool(negative_bool)) continue;

                bool negative = cJSON_IsTrue(negative_bool);

                binding->axis.index = (uint8_t)index;
                binding->axis.negative = negative;

                break;
            }
        }

        binding->type = type;
    }
}

void input_settings_t::load(cJSON* object){
    
    clear_bindings();

    cJSON* input_settings_object = cJSON_GetObjectItemCaseSensitive(object,"Input settings");

    if(!input_settings_object) return;

    load_keyboard_bindings(input_settings_object);

    load_controller_bindings(input_settings_object);
}


bool input_settings_t::button_pressed(gb_joypad_button_t button){

    const uint8_t* keyboard = SDL_GetKeyboardState(nullptr);

    if(keyboard[keyboard_bindings[button]]){
        return true;
    }

    if(current_controller != nullptr){

        bool pressed = false;

        controller_binding_t* controller_binding = controller_bindings + button;

        switch(controller_binding->type){
            case input_settings_t::controller_binding_button:{
                pressed = SDL_GameControllerGetButton(current_controller,(SDL_GameControllerButton)controller_binding->button);
                break;
            }
            case input_settings_t::controller_binding_axis:{
                int16_t axis = SDL_GameControllerGetAxis(current_controller,(SDL_GameControllerAxis)controller_binding->axis.index);
                
                if(abs(axis) > input_settings_t::controller_axis_deadzone){
                    if(!controller_binding->axis.negative && axis > 0){
                        pressed = true;
                    }
                    else if(controller_binding->axis.negative && axis < 0){
                        pressed = true;
                    }
                }

                break;
            }
        }
        
        return pressed;
    }

    return false;
}


void input_settings_t::event(SDL_Event& event){

    switch(event.type){
        case SDL_CONTROLLERDEVICEADDED:{

            SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which);

            if(controller != nullptr){

                controller_devices.push_back(controller);
            }
            else{
                gb_printf_error(SDL_GetError());
            }

            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED:{

            SDL_GameController* removed_controller = nullptr;

            for(auto controller : controller_devices){
                
                SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);

                if(SDL_JoystickInstanceID(joystick) == event.cdevice.which){
                    removed_controller = controller;
                    break;
                }
            }

            if(removed_controller != nullptr){

                if(current_controller == removed_controller){
                    current_controller = nullptr;
                }

                controller_devices.remove(removed_controller);
                
                SDL_GameControllerClose(removed_controller);
            }

            break;
        }
        case SDL_KEYDOWN:{

            if(binding_type == input_settings_t::binding_keyboard && binding_button != -1){
                
                temp_keyboard_bindings[binding_button] = event.key.keysym.scancode;

                binding_type = input_settings_t::binding_none;
                binding_button = -1;
            }
            
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:{

            current_controller = SDL_GameControllerFromInstanceID(event.cbutton.which);

            if(binding_type == input_settings_t::binding_controller && binding_button != -1){

                controller_binding_t* binding = temp_controller_bindings + binding_button;

                binding->type = input_settings_t::controller_binding_button;
                binding->button = event.cbutton.button;

                binding_type = input_settings_t::binding_none;
                binding_button = -1;
            }

            break;
        }
        case SDL_CONTROLLERAXISMOTION:{

            current_controller = SDL_GameControllerFromInstanceID(event.cbutton.which);

            if(binding_type == input_settings_t::binding_controller && binding_button != -1 && abs(event.caxis.value) > input_settings_t::controller_axis_deadzone){

                controller_binding_t* binding = temp_controller_bindings + binding_button;

                binding->type = input_settings_t::controller_binding_axis;
                binding->axis.index = event.caxis.axis;
                binding->axis.negative = event.caxis.value < 0;

                binding_type = input_settings_t::binding_none;
                binding_button = -1;
            }

            break;
        }
    }
}


void input_settings_t::render(){
    if(!_open) return;

    ImGui::SetNextWindowSizeConstraints(window_min_size,window_max_size);

    if(ImGui::Begin("Input",&_open)){

        if(ImGui::BeginTable("InputTable",3,ImGuiTableFlags_Borders)){
            
            ImGui::TableSetupColumn("Button",ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Keyboard",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Controller",ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();
            
            for(int i = 0; i < gb_button_count; ++i){

                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                
                ImGui::AlignTextToFramePadding();

                ImGui::TextUnformatted(gb_joypad_get_button_name(i));
                
                ImGui::TableNextColumn();
                
                ImGui::PushID(i);
                
                ImGui::PushID("KeyboardSetup");

                if(binding_type == input_settings_t::binding_keyboard && binding_button == i){

                    ImGuiStyle& style = ImGui::GetStyle();

                    ImGui::PushStyleColor(ImGuiCol_Button,style.Colors[ImGuiCol_ButtonActive]);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,style.Colors[ImGuiCol_ButtonActive]);

                    ImGui::Button("Press a key...",ImVec2(-FLT_MIN,0.0f));

                    ImGui::PopStyleColor(2);
                }
                else{
                    if(ImGui::Button(SDL_GetScancodeName(temp_keyboard_bindings[i]),ImVec2(-FLT_MIN,0.0f))){
                        
                        binding_type = input_settings_t::binding_keyboard;
                        binding_button = i;

                        temp_keyboard_bindings[i] = SDL_SCANCODE_UNKNOWN;
                    }
                }

                ImGui::PopID();
                
                ImGui::PushID("ControllerSetup");

                ImGui::TableNextColumn();

                if(binding_type == input_settings_t::binding_controller && binding_button == i){

                    ImGuiStyle& style = ImGui::GetStyle();

                    ImGui::PushStyleColor(ImGuiCol_Button,style.Colors[ImGuiCol_ButtonActive]);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,style.Colors[ImGuiCol_ButtonActive]);

                    ImGui::Button("Press a key...",ImVec2(-FLT_MIN,0.0f));

                    ImGui::PopStyleColor(2);
                }
                else{
                    controller_binding_t* binding = temp_controller_bindings + i;

                    const char* label = "";

                    switch(binding->type){
                        case input_settings_t::controller_binding_button:{
                            label = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)binding->button);
                            break;
                        }
                        case input_settings_t::controller_binding_axis:{
                            label = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)binding->axis.index);
                            break;
                        }
                    }

                    if(ImGui::Button(label,ImVec2(-FLT_MIN,0.0f))){
                        
                        binding_type = input_settings_t::binding_controller;
                        binding_button = i;

                        temp_controller_bindings[i].type = input_settings_t::controller_binding_none;
                    }
                }

                ImGui::PopID();

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGuiStyle& style = ImGui::GetStyle();

        const char* str_ok = "Ok";
        const char* str_reset = "Reset";
        const char* str_cancel = "Cancel";

        float ok_button_width = ImGui::CalcTextSize(str_ok).x + style.FramePadding.x * 2.0f;
        float reset_button_width = ImGui::CalcTextSize(str_reset).x + style.FramePadding.x * 2.0f;
        float cancel_button_width = ImGui::CalcTextSize(str_cancel).x + style.FramePadding.x * 2.0f;

        ImGui::SetCursorPosX((ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x) - (ok_button_width + reset_button_width + cancel_button_width + style.ItemSpacing.x * 2.0f));

        if(ImGui::Button(str_ok)) close(false);

        ImGui::SameLine();

        if(ImGui::Button(str_reset)){
            binding_type = input_settings_t::binding_none;
            binding_button = -1;
            for(int i = 0; i < gb_button_count; ++i){
                temp_keyboard_bindings[i] = SDL_SCANCODE_UNKNOWN;
                temp_controller_bindings[i].type = input_settings_t::controller_binding_none;
            }
        }

        ImGui::SameLine();

        if(ImGui::Button(str_cancel)) close(true);
    }

    ImGui::End();

    if(!_open){
        close(true);
    }
}


void input_settings_t::open() noexcept {
    if(_open) return;

    _open = true;

    memcpy(temp_keyboard_bindings,keyboard_bindings,sizeof(temp_keyboard_bindings));
    memcpy(temp_controller_bindings,controller_bindings,sizeof(temp_controller_bindings));
}

void input_settings_t::close(bool discard_changes) noexcept {
    if(!_open) return;

    _open = false;

    binding_type = input_settings_t::binding_none;
    binding_button = -1;

    if(!discard_changes){
        memcpy(keyboard_bindings,temp_keyboard_bindings,sizeof(keyboard_bindings));
        memcpy(controller_bindings,temp_controller_bindings,sizeof(controller_bindings));
    }
}