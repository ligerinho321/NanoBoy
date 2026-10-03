#include <gui/screen/screen.hpp>
#include <gui/nanoboy/nanoboy.hpp>

void screen_t::init(nanoboy_t* _nanoboy){

    nanoboy = _nanoboy;
    gb = nanoboy->gb;

    create_texture();

    ImGuiStyle& style = ImGui::GetStyle();

    floating_min_size.x = gb_screen_width + style.WindowPadding.x * 2.0f;
    floating_min_size.y = ImGui::GetFrameHeight() + gb_screen_height + style.WindowPadding.y * 2.0f;

    floating_max_size.x = FLT_MAX;
    floating_max_size.y = FLT_MAX;
}

void screen_t::uninit(){
    SDL_DestroyTexture(texture);
}


void screen_t::create_texture(){

    if(texture != nullptr){
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    texture = SDL_CreateTexture(nanoboy->renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STREAMING,gb_screen_width,gb_screen_height);

    clear();
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

    cJSON* interframe_blending_bool = cJSON_CreateBool(gb->ppu.interframe_blending);
    cJSON_AddItemToObjectCS(screen_object,"Interframe Blending",interframe_blending_bool);

    cJSON* disable_background_bool = cJSON_CreateBool(gb->ppu.background_disabled);
    cJSON_AddItemToObjectCS(screen_object,"Disable Background",disable_background_bool);

    cJSON* disable_objects_bool = cJSON_CreateBool(gb->ppu.objects_disabled);
    cJSON_AddItemToObjectCS(screen_object,"Disable Objects",disable_objects_bool);
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
        set_bilinear_filtering(cJSON_IsTrue(bilinear_filtering_bool));
    }

    cJSON* interframe_blending_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Interframe Blending");

    if(cJSON_IsBool(interframe_blending_bool)){
        gb->ppu.interframe_blending = cJSON_IsTrue(interframe_blending_bool);
    }

    cJSON* disable_background_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Disable Background");

    if(cJSON_IsBool(disable_background_bool)){
        gb->ppu.background_disabled = cJSON_IsTrue(disable_background_bool);
    }

    cJSON* disable_objects_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Disable Objects");
    
    if(cJSON_IsBool(disable_objects_bool)){
        gb->ppu.objects_disabled = cJSON_IsTrue(disable_objects_bool);
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
    embedded_rect.w = gb_screen_width * new_scale;
    embedded_rect.h = gb_screen_height * new_scale;

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
        float ratio_scale_x = (float)width / gb_screen_width;
        float ratio_scale_y = (float)height / gb_screen_height;

        float ratio_scale = gb_min(ratio_scale_x,ratio_scale_y);

        width = gb_screen_width * ratio_scale;
        height = gb_screen_height * ratio_scale;
    }
    
    if(interger_scale){
        int scale_x = width / gb_screen_width;
        int scale_y = height / gb_screen_height;

        width = gb_screen_width * scale_x;
        height = gb_screen_height * scale_y;
    }

    embedded_rect.w = width;
    embedded_rect.h = height;
    embedded_rect.x = (window_width - embedded_rect.w) / 2;
    embedded_rect.y = main_menu_bar_height + (window_height - embedded_rect.h) / 2;
}

void screen_t::set_bilinear_filtering(bool new_bilinear_filtering){

    if(new_bilinear_filtering == bilinear_filtering) return;

    bilinear_filtering = new_bilinear_filtering;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,bilinear_filtering ? "1" : "0");

    create_texture();

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
    gb->ppu.interframe_blending = !gb->ppu.interframe_blending;
}

void screen_t::toggle_bilinear_filtering(){
    set_bilinear_filtering(!bilinear_filtering);
}

void screen_t::toggle_disable_background(){
    gb->ppu.background_disabled = !gb->ppu.background_disabled;
}

void screen_t::toggle_disable_objects(){
     gb->ppu.objects_disabled = !gb->ppu.objects_disabled;
}


void screen_t::update_screen(){

    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,(void**)&pixels,&pitch);

    memcpy(pixels,gb_ppu_get_render_buffer(gb),gb_screen_length);

    SDL_UnlockTexture(texture);
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
    if(shortcut.down(shortcut_settings_t::toggle_disable_background)){
        toggle_disable_background();
    }
    if(shortcut.down(shortcut_settings_t::toggle_disable_objects)){
        toggle_disable_objects();
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

    if(ImGui::MenuItem("Interframe Blending",shortcut.str_keyboard(shortcut_settings_t::toggle_interframe_blending),gb->ppu.interframe_blending)){
        toggle_interframe_blending();
    }

    if(ImGui::MenuItem("Bilinear Filtering",shortcut.str_keyboard(shortcut_settings_t::toggle_bilinear_filtering),bilinear_filtering)){
        toggle_bilinear_filtering();
    }

    if(ImGui::MenuItem("Disable Background",shortcut.str_keyboard(shortcut_settings_t::toggle_disable_background),gb->ppu.background_disabled)){
        toggle_disable_background();
    }

    if(ImGui::MenuItem("Disable Objects",shortcut.str_keyboard(shortcut_settings_t::toggle_disable_objects),gb->ppu.objects_disabled)){
       toggle_disable_objects();
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
                float ratio_scale_x = evail_size.x / gb_screen_width;
                float ratio_scale_y = evail_size.y / gb_screen_height;

                float ratio_scale = gb_min(ratio_scale_x,ratio_scale_y);

                floating_size.x = gb_screen_width * ratio_scale;
                floating_size.y = gb_screen_height * ratio_scale;
            }

            if(interger_scale){
                int scale_x = floating_size.x / gb_screen_width;
                int scale_y = floating_size.y / gb_screen_height;

                floating_size.x = gb_screen_width * scale_x;
                floating_size.y = gb_screen_height * scale_y;
            }

            ImVec2 cursor = ImGui::GetCursorPos();
            
            floating_pos.x = cursor.x + (evail_size.x - floating_size.x) * 0.5f;
            floating_pos.y = cursor.y + (evail_size.y - floating_size.y) * 0.5f;

            last_evail_size = evail_size;
        }

        ImGui::SetCursorPos(floating_pos);

        ImGui::Image((ImTextureRef)texture,floating_size);
    }

    ImGui::End();
}

void screen_t::render_embedded(){
    if(_floating) return;

    SDL_RenderCopy(nanoboy->renderer,texture,nullptr,&embedded_rect);
}
