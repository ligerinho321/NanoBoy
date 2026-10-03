#include <gui/utils/utils.hpp>

void palette_t::update_texture(bool cgb_mode){
    
    int rows = cgb_mode ? gb_cgb_palettes : (is_obj ? gb_dmg_obj_palettes : gb_dmg_bg_palettes);

    gb_rgb_t color{};

    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,(void**)&pixels,&pitch);

    for(int row = 0; row < rows; ++row){
        for(int col = 0; col < gb_palette_colors; ++col){

            if(cgb_mode){
                color = get_cgb_color(row,col);
            }
            else{
                color = get_dmg_color(row,col);
            }

            for(int y = 0; y < gb_tile_size; ++y){
                for(int x = 0; x < gb_tile_size; ++x){
                    uint8_t* pixel = pixels + ((row * gb_tile_size) | y) * pitch + ((col * gb_tile_size) | x) * texture_bytes_per_pixel;
                    pixel[0] = color.r;
                    pixel[1] = color.g;
                    pixel[2] = color.b;
                }
            }

        }
    }

    SDL_UnlockTexture(texture);
}

void bg_palette_t::clear(){
    clear_texture(texture,texture_max_height);
    bgp = 0;
    memset(colors,0,sizeof(colors));
}

void obj_palette_t::clear(){
    clear_texture(texture,texture_max_height);
    obp[0] = 0;
    obp[1] = 0;
    memset(colors,0,sizeof(colors));
}


void binding_capture_popup_t::render(){

    static const char* popup_title_text[] = {
        "Set button binding",
        "Set key binding"
    };

    static const char* popup_text[] = {
        "Press any button on the controller.",
        "Press any key on the keyboard."
    };

    if(open_needed){
        
        open_needed = false;

        _open = true;

        ImGui::OpenPopup(popup_title_text[is_keyboard]);

        ImVec2 viewerport_size = ImGui::GetMainViewport()->Size;

        start_pos.x = viewerport_size.x * 0.5f;
        start_pos.y = viewerport_size.y * 0.5f;

        ImGuiStyle& style = ImGui::GetStyle();

        size.x = ImGui::CalcTextSize(popup_text[is_keyboard]).x + style.WindowPadding.x * 2.0f;
        size.y = ImGui::GetFrameHeight() + ImGui::GetTextLineHeight() + style.WindowPadding.y * 2.0f;
    }

    if(!_open) return;

    ImGui::SetNextWindowPos(start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSizeConstraints(size,size);

    if(!ImGui::BeginPopupModal(popup_title_text[is_keyboard],&_open,ImGuiWindowFlags_NoResize)) return;

    ImGui::SetNextFrameWantCaptureKeyboard(true);

    ImGui::Text(popup_text[is_keyboard]);

    ImGui::EndPopup();
}


const char* controller_binding_type_names[controller_binding_count] = {
    "None",
    "Button",
    "Axis"
};

int get_controller_binding_type_from_string(const char* string){
    for(int i = 0; i < controller_binding_count; ++i){
        if(!strcmp(controller_binding_type_names[i],string)){
            return i;
        }
    }
    return -1;
}


void save_keyboard_binding(cJSON* keyboard_binding_object,keyboard_binding_t& keyboard){
    cJSON* scancode_number = cJSON_CreateNumber(keyboard.scancode);
    cJSON_AddItemToObjectCS(keyboard_binding_object,"Scancode",scancode_number);

    cJSON* modifiers_number = cJSON_CreateNumber(keyboard.modifiers);
    cJSON_AddItemToObjectCS(keyboard_binding_object,"Modifiers",modifiers_number);
}

bool load_keyboard_binding(cJSON* keyboard_binding_object,keyboard_binding_t& keyboard){

    cJSON* scancode_number = cJSON_GetObjectItemCaseSensitive(keyboard_binding_object,"Scancode");

    if(!cJSON_IsNumber(scancode_number)) return false;

    cJSON* modifiers_number = cJSON_GetObjectItemCaseSensitive(keyboard_binding_object,"Modifiers");

    if(!cJSON_IsNumber(modifiers_number)) return false;

    int scancode = cJSON_GetNumberValue(scancode_number);

    if(scancode <= SDL_SCANCODE_UNKNOWN || scancode >= SDL_NUM_SCANCODES) return false;

    keyboard.scancode = (SDL_Scancode)scancode;
    keyboard.modifiers = keyboard_binding_normalize_modifiers(cJSON_GetNumberValue(modifiers_number));

    return true;
}


void save_controller_binding(cJSON* controller_binding_object,controller_binding_t& controller){
    cJSON* type_string = cJSON_CreateStringReference(controller_binding_type_names[controller.type]);
    cJSON_AddItemToObjectCS(controller_binding_object,"Type",type_string);

    switch(controller.type){
        case controller_binding_button:{
            cJSON* value_number = cJSON_CreateNumber(controller.button);
            cJSON_AddItemToObjectCS(controller_binding_object,"Value",value_number);
            break;
        }
        case controller_binding_axis:{
            cJSON* index_number = cJSON_CreateNumber(controller.axis.index);
            cJSON_AddItemToObjectCS(controller_binding_object,"Index",index_number);

            cJSON* negative_bool = cJSON_CreateBool(controller.axis.negative);
            cJSON_AddItemToObjectCS(controller_binding_object,"Negative",negative_bool);
            break;
        }
    }
}

bool load_controller_binding(cJSON* controller_binding_object,controller_binding_t& controller){

    cJSON* type_string = cJSON_GetObjectItemCaseSensitive(controller_binding_object,"Type");

    if(!cJSON_IsString(type_string)) return false;

    int type = get_controller_binding_type_from_string(cJSON_GetStringValue(type_string));

    if(type < controller_binding_none || type >= controller_binding_count) return false;

    switch(type){
        case controller_binding_button:{
            cJSON* value_number = cJSON_GetObjectItemCaseSensitive(controller_binding_object,"Value");

            if(!cJSON_IsNumber(value_number)) return false;

            controller.button = (uint8_t)cJSON_GetNumberValue(value_number);
            break;
        }
        case controller_binding_axis:{
            cJSON* index_number = cJSON_GetObjectItemCaseSensitive(controller_binding_object,"Index");

            if(!cJSON_IsNumber(index_number)) return false;

            int index = cJSON_GetNumberValue(index_number);

            if(index < SDL_CONTROLLER_AXIS_LEFTX || index >= SDL_CONTROLLER_AXIS_MAX) return false;

            cJSON* negative_bool = cJSON_GetObjectItemCaseSensitive(controller_binding_object,"Negative");

            if(!cJSON_IsBool(negative_bool)) return false;
            
            controller.axis.index = (uint8_t)index;
            controller.axis.negative = cJSON_IsTrue(negative_bool);
            break;
        }
    }

    controller.type = type;

    return true;
}


std::string get_keyboard_binding_string(const keyboard_binding_t& keyboard){
    std::string str;

    if(keyboard.modifiers & KMOD_CTRL) str += "Ctrl+";

    if(keyboard.modifiers & KMOD_SHIFT) str += "Shift+";

    if(keyboard.modifiers & KMOD_ALT) str += "Alt+";

    if(keyboard.modifiers & KMOD_GUI) str += "GUI+";

    str += SDL_GetScancodeName(keyboard.scancode);

    return str;
}

std::string get_controller_binding_string(const controller_binding_t& controller){
    std::string str;

    switch(controller.type){
        case controller_binding_button:{
            str = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)controller.button);
            break;
        }
        case controller_binding_axis:{
            str = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)controller.axis.index);
            break;
        }
    }

    return str;
}


void render_size_text(size_t size){

    if(size >= gigabytes){
        ImGui::Text("%.1f GB",(double)size / (double)gigabytes);
    }
    else if(size >= megabytes){
        ImGui::Text("%.1f MB",(double)size / (double)megabytes);
    }
    else if(size >= kilobytes){
        ImGui::Text("%.1f KB",(double)size / (double)kilobytes);
    }
    else{
        ImGui::Text("%zu B",size);
    }
}

void render_hertz_text(size_t hertz){
    if(hertz >= gigahertz){
        ImGui::Text("%.1f gHz",(double)hertz / (double)gigahertz);
    }
    else if(hertz >= megahertz){
        ImGui::Text("%.1f mHz",(double)hertz / (double)megahertz);
    }
    else if(hertz >= kilohertz){
        ImGui::Text("%.1f kHz",(double)hertz / (double)kilohertz);
    }
    else{
        ImGui::Text("%zu Hz",hertz);
    }
}

time_t get_file_last_write_time(std::filesystem::path file){

    std::chrono::time_point sys_time_point = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        std::filesystem::last_write_time(file) - 
        std::filesystem::file_time_type::clock::now() + 
        std::chrono::system_clock::now()
    );

    return std::chrono::system_clock::to_time_t(sys_time_point);
}

const char* get_time_formated(time_t time){
    static char buffer[32] = {0};

    tm* local_time = localtime(&time);
    
    std::strftime(buffer,sizeof(buffer),"%d/%m/%Y %H:%M",local_time);

    return buffer;
}


void clear_texture(SDL_Texture* texture,int height){
    void* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,&pixels,&pitch);
    memset(pixels,0,pitch * height);
    SDL_UnlockTexture(texture);
}


const char* image_extensions[] = {
    "PNG\0.png",
    "BMP\0.bmp",
    "TGA\0.tga",
    "JPG\0.jpg"
};

const int image_extensions_count = sizeof(image_extensions) / sizeof(image_extensions[0]);