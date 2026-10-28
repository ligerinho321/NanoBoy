#include <gui/screen/screen.hpp>
#include <gui/nanoboy/nanoboy.hpp>

void screen_t::init(nanoboy_t* _nanoboy){
    nanoboy = _nanoboy;

    floating_max_size.x = FLT_MAX;
    floating_max_size.y = FLT_MAX;
}


void screen_t::update_console(){
    
    if(!nanoboy->console) return;

    ImGuiStyle& style = ImGui::GetStyle();

    base_width = nanoboy->console->screen_width();
    base_height = nanoboy->console->screen_height();

    floating_min_size.x = base_width + style.WindowPadding.x * 2.0f;
    floating_min_size.y = ImGui::GetFrameHeight() + base_height + style.WindowPadding.y * 2.0f;

    update_embedded_size();

    set_bilinear_filtering(bilinear_filtering);

    nanoboy->console->enable_interframe_blending(interframe_blending);

    switch(nanoboy->console->type()){
        case console_gb:{
            gb_t* gb = (gb_t*)nanoboy->console->data();
            gb->ppu.background_disabled = gb_layer.background_disabled;
            gb->ppu.objects_disabled = gb_layer.objects_disabled;
            break;
        }
        case console_gba:{
            break;
        }
    }
}


void screen_t::save(cJSON* settings_object){

    cJSON* screen_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(settings_object,"Screen",screen_object);

    cJSON* floating_bool = cJSON_CreateBool(_floating);
    cJSON_AddItemToObjectCS(screen_object,"Floating",floating_bool);

    cJSON* aspect_ratio_bool = cJSON_CreateBool(aspect_ratio);
    cJSON_AddItemToObjectCS(screen_object,"Aspect Ratio",aspect_ratio_bool);

    cJSON* interger_scale_bool = cJSON_CreateBool(interger_scale);
    cJSON_AddItemToObjectCS(screen_object,"Interger Scale",interger_scale_bool);

    cJSON* bilinear_filtering_bool = cJSON_CreateBool(bilinear_filtering);
    cJSON_AddItemToObjectCS(screen_object,"Bilinear Filtering",bilinear_filtering_bool);

    cJSON* interframe_blending_bool = cJSON_CreateBool(interframe_blending);
    cJSON_AddItemToObjectCS(screen_object,"Interframe Blending",interframe_blending_bool);


    cJSON* gb_layer_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(screen_object,"GB Layer",gb_layer_object);

    cJSON* disable_background_bool = cJSON_CreateBool(gb_layer.background_disabled);
    cJSON_AddItemToObjectCS(gb_layer_object,"Disable Background",disable_background_bool);

    cJSON* disable_objects_bool = cJSON_CreateBool(gb_layer.objects_disabled);
    cJSON_AddItemToObjectCS(gb_layer_object,"Disable Objects",disable_objects_bool);
}

void screen_t::load(cJSON* settings_object){
    cJSON* screen_object = cJSON_GetObjectItemCaseSensitive(settings_object,"Screen");
    
    if(!cJSON_IsObject(screen_object)) return;

    cJSON* floating_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Floating");

    if(cJSON_IsBool(floating_bool)){
        _floating = cJSON_IsTrue(floating_bool);
    }

    cJSON* aspect_ratio_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Aspect Ratio");

    if(cJSON_IsBool(aspect_ratio_bool)){
        aspect_ratio = cJSON_IsTrue(aspect_ratio_bool);
    }

    cJSON* interger_scale_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Interger Scale");

    if(cJSON_IsBool(interger_scale_bool)){
        interger_scale = cJSON_IsTrue(interger_scale_bool);
    }

    cJSON* bilinear_filtering_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Bilinear Filtering");

    if(cJSON_IsBool(bilinear_filtering_bool)){
        bilinear_filtering = cJSON_IsTrue(bilinear_filtering_bool);
    }

    cJSON* interframe_blending_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Interframe Blending");

    if(cJSON_IsBool(interframe_blending_bool)){
        interframe_blending = cJSON_IsTrue(interframe_blending_bool);
    }


    cJSON* gb_layer_object = cJSON_GetObjectItemCaseSensitive(screen_object,"GB Layer");

    if(cJSON_IsObject(gb_layer_object)){
        cJSON* disable_background_bool = cJSON_GetObjectItemCaseSensitive(gb_layer_object,"Disable Background");

        if(cJSON_IsBool(disable_background_bool)){
            gb_layer.background_disabled = cJSON_IsTrue(disable_background_bool);
        }

        cJSON* disable_objects_bool = cJSON_GetObjectItemCaseSensitive(gb_layer_object,"Disable Objects");
        
        if(cJSON_IsBool(disable_objects_bool)){
            gb_layer.objects_disabled = cJSON_IsTrue(disable_objects_bool);
        }
    }

    update_embedded_size();

    last_evail_size.x = 0.0f;
    last_evail_size.y = 0.0f;
}


void screen_t::set_embedded_scale(int new_scale){

    uint32_t flags = SDL_GetWindowFlags(nanoboy->window);
    
    if(flags & SDL_WINDOW_MAXIMIZED){
        SDL_RestoreWindow(nanoboy->window);
    }

    int main_menu_bar_height = 0;

    if(flags & SDL_WINDOW_FULLSCREEN_DESKTOP){
        SDL_SetWindowFullscreen(nanoboy->window,0);
    }
    else{
        main_menu_bar_height = ImGui::GetFrameHeight();
    }

    embedded_rect.x = 0;
    embedded_rect.y = main_menu_bar_height;
    embedded_rect.w = base_width * new_scale;
    embedded_rect.h = base_height * new_scale;

    SDL_SetWindowSize(nanoboy->window,embedded_rect.w,embedded_rect.h + main_menu_bar_height);
}

void screen_t::update_embedded_size(){
    int window_width = 0;
    int window_height = 0;

    SDL_GetWindowSize(nanoboy->window,&window_width,&window_height);

    int main_menu_bar_height = 0;

    if(!(SDL_GetWindowFlags(nanoboy->window) & SDL_WINDOW_FULLSCREEN_DESKTOP)){
        main_menu_bar_height = ImGui::GetFrameHeight();
    }

    window_height -= main_menu_bar_height;

    int width = window_width;
    int height = window_height;

    if(aspect_ratio){
        float ratio_scale_x = (float)width / base_width;
        float ratio_scale_y = (float)height / base_height;

        float ratio_scale = gb_min(ratio_scale_x,ratio_scale_y);

        width = base_width * ratio_scale;
        height = base_height * ratio_scale;
    }
    
    if(interger_scale){
        int scale_x = width / base_width;
        int scale_y = height / base_height;

        width = base_width * scale_x;
        height = base_height * scale_y;
    }

    embedded_rect.w = width;
    embedded_rect.h = height;
    embedded_rect.x = (window_width - embedded_rect.w) / 2;
    embedded_rect.y = main_menu_bar_height + (window_height - embedded_rect.h) / 2;
}

void screen_t::set_bilinear_filtering(bool new_bilinear_filtering){
    bilinear_filtering = new_bilinear_filtering;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,bilinear_filtering ? "1" : "0");

    nanoboy->console->create_screen_texture();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
}


void screen_t::toggle_fullscreen(){
    if(SDL_GetWindowFlags(nanoboy->window) & SDL_WINDOW_FULLSCREEN_DESKTOP){
        SDL_SetWindowFullscreen(nanoboy->window,0);
    }
    else{
        SDL_SetWindowFullscreen(nanoboy->window,SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void screen_t::toggle_floating(){
    _floating = !_floating;
}

void screen_t::toggle_acpect_ratio(){
    aspect_ratio = !aspect_ratio;

    update_embedded_size();

    last_evail_size.x = 0.0f;
    last_evail_size.y = 0.0f;
}

void screen_t::toggle_interger_scale(){
    interger_scale = !interger_scale;

    update_embedded_size();

    last_evail_size.x = 0.0f;
    last_evail_size.y = 0.0f;
}

void screen_t::toggle_interframe_blending(){
    interframe_blending = !interframe_blending;
    if(nanoboy->console != nullptr){
        nanoboy->console->enable_interframe_blending(interframe_blending);
    }
}

void screen_t::toggle_bilinear_filtering(){
    set_bilinear_filtering(!bilinear_filtering);
}


void screen_t::toggle_disable_gb_background(){
    gb_layer.background_disabled = !gb_layer.background_disabled;

    if(nanoboy->console != nullptr && nanoboy->console->type() == console_gb){
        gb_t* gb = (gb_t*)nanoboy->console->data();
        gb->ppu.background_disabled = gb_layer.background_disabled;
    }
}

void screen_t::toggle_disable_gb_objects(){
    gb_layer.objects_disabled = !gb_layer.objects_disabled;

    if(nanoboy->console != nullptr && nanoboy->console->type() == console_gb){
        gb_t* gb = (gb_t*)nanoboy->console->data();
        gb->ppu.objects_disabled = !gb->ppu.objects_disabled;
    }
}


void screen_t::shortcut_event(){

    shortcut_settings_t& shortcut = nanoboy->shortcut_settings;

    if(shortcut.down(shortcut_settings_t::set_scale_1x)){
        set_embedded_scale(1);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_2x)){
        set_embedded_scale(2);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_3x)){
        set_embedded_scale(3);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_4x)){
        set_embedded_scale(4);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_5x)){
        set_embedded_scale(5);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_6x)){
        set_embedded_scale(6);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_7x)){
        set_embedded_scale(7);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_8x)){
        set_embedded_scale(8);
    }
    if(shortcut.down(shortcut_settings_t::set_scale_9x)){
        set_embedded_scale(9);
    }
    if(shortcut.down(shortcut_settings_t::toggle_fullscreen)){
        toggle_fullscreen();
    }
    if(shortcut.down(shortcut_settings_t::toggle_floating)){
        toggle_floating();
    }
    if(shortcut.down(shortcut_settings_t::toggle_aspect_ratio)){
        toggle_acpect_ratio();
    }
    if(shortcut.down(shortcut_settings_t::toggle_interger_scale)){
        toggle_interger_scale();
    }
    if(shortcut.down(shortcut_settings_t::toggle_interframe_blending)){
        toggle_interframe_blending();
    }
    if(shortcut.down(shortcut_settings_t::toggle_bilinear_filtering)){
        toggle_bilinear_filtering();
    }
}


void screen_t::render_menu_bar(){

    if(!ImGui::BeginMenu("Screen")) return;

    shortcut_settings_t& shortcut = nanoboy->shortcut_settings;

    if(ImGui::BeginMenu("Scale")){

        int scale = 0;

        if(ImGui::MenuItem("1x",shortcut.str_keyboard(shortcut_settings_t::set_scale_1x))) scale = 1;
        if(ImGui::MenuItem("2x",shortcut.str_keyboard(shortcut_settings_t::set_scale_2x))) scale = 2;
        if(ImGui::MenuItem("3x",shortcut.str_keyboard(shortcut_settings_t::set_scale_3x))) scale = 3;
        if(ImGui::MenuItem("4x",shortcut.str_keyboard(shortcut_settings_t::set_scale_4x))) scale = 4;
        if(ImGui::MenuItem("5x",shortcut.str_keyboard(shortcut_settings_t::set_scale_5x))) scale = 5;
        if(ImGui::MenuItem("6x",shortcut.str_keyboard(shortcut_settings_t::set_scale_6x))) scale = 6;
        if(ImGui::MenuItem("7x",shortcut.str_keyboard(shortcut_settings_t::set_scale_7x))) scale = 7;
        if(ImGui::MenuItem("8x",shortcut.str_keyboard(shortcut_settings_t::set_scale_8x))) scale = 8;
        if(ImGui::MenuItem("9x",shortcut.str_keyboard(shortcut_settings_t::set_scale_9x))) scale = 9;

        if(scale > 0){
            set_embedded_scale(scale);
        }

        if(ImGui::MenuItem("FullScreen",shortcut.str_keyboard(shortcut_settings_t::toggle_fullscreen))){
            toggle_fullscreen();
        }

        ImGui::EndMenu();
    }

    if(ImGui::MenuItem("Floating",shortcut.str_keyboard(shortcut_settings_t::toggle_floating),_floating)){
        toggle_floating();
    }

    if(ImGui::MenuItem("Aspect Ratio",shortcut.str_keyboard(shortcut_settings_t::toggle_aspect_ratio),aspect_ratio)){
        toggle_acpect_ratio();
    }

    if(ImGui::MenuItem("Interger Scale",shortcut.str_keyboard(shortcut_settings_t::toggle_interger_scale),interger_scale)){
        toggle_interger_scale();
    }

    if(ImGui::MenuItem("Interframe Blending",shortcut.str_keyboard(shortcut_settings_t::toggle_interframe_blending),interframe_blending)){
        toggle_interframe_blending();
    }

    if(ImGui::MenuItem("Bilinear Filtering",shortcut.str_keyboard(shortcut_settings_t::toggle_bilinear_filtering),bilinear_filtering)){
        toggle_bilinear_filtering();
    }

    if(ImGui::BeginMenu("Layer",nanoboy->console != nullptr)){

        switch(nanoboy->console->type()){
            case console_gb:{
                if(ImGui::MenuItem("Disable Background",nullptr,gb_layer.background_disabled)){
                    toggle_disable_gb_background();
                }

                if(ImGui::MenuItem("Disable Objects",nullptr,gb_layer.objects_disabled)){
                    toggle_disable_gb_objects();
                }
                break;
            }
            case console_gba:{
                break;
            }
        }

        ImGui::EndMenu();
    }

    ImGui::EndMenu();
}

void screen_t::render_floating(){

    if(!_floating) return;

    ImGui::SetNextWindowSizeConstraints(floating_min_size,floating_max_size);

    if(ImGui::Begin("Screen",nullptr)){
        
        ImVec2 evail_size = ImGui::GetContentRegionAvail();
        
        if(evail_size.x != last_evail_size.x || evail_size.y != last_evail_size.y){

            floating_size.x = evail_size.x;
            floating_size.y = evail_size.y;

            if(aspect_ratio){
                float ratio_scale_x = evail_size.x / base_width;
                float ratio_scale_y = evail_size.y / base_height;

                float ratio_scale = gb_min(ratio_scale_x,ratio_scale_y);

                floating_size.x = base_width * ratio_scale;
                floating_size.y = base_height * ratio_scale;
            }

            if(interger_scale){
                int scale_x = floating_size.x / base_width;
                int scale_y = floating_size.y / base_height;

                floating_size.x = base_width * scale_x;
                floating_size.y = base_height * scale_y;
            }

            ImVec2 cursor = ImGui::GetCursorPos();
            
            floating_pos.x = cursor.x + (evail_size.x - floating_size.x) * 0.5f;
            floating_pos.y = cursor.y + (evail_size.y - floating_size.y) * 0.5f;

            last_evail_size = evail_size;
        }

        ImGui::SetCursorPos(floating_pos);

        ImGui::Image((ImTextureRef)nanoboy->console->screen_texture,floating_size);
    }

    ImGui::End();
}

void screen_t::render_embedded(){
    if(_floating || !nanoboy->console || !nanoboy->console->screen_texture) return;

    SDL_RenderCopy(nanoboy->renderer,nanoboy->console->screen_texture,nullptr,&embedded_rect);
}
