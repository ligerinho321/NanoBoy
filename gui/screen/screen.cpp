#include <gui/screen/screen.hpp>

screen_t::screen_t(gb_t* gb,SDL_Window* window,SDL_Renderer* renderer):gb(gb),window(window),renderer(renderer){

    create_texture();

    ImGuiStyle& style = ImGui::GetStyle();

    floating_min_size.x = gb_screen_width + style.WindowPadding.x * 2.0f;
    floating_min_size.y = ImGui::GetFrameHeight() + gb_screen_height + style.WindowPadding.y * 2.0f;

    floating_max_size.x = FLT_MAX;
    floating_max_size.y = FLT_MAX;
}

screen_t::~screen_t(){
    SDL_DestroyTexture(texture);
}


void screen_t::create_texture(){

    if(texture != nullptr){
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STREAMING,gb_screen_width,gb_screen_height);

    clear();
}


void screen_t::save(cJSON* settings_object){

    cJSON* screen_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(settings_object,"Screen",screen_object);

    cJSON* floating_bool = cJSON_CreateBool(_floating);
    cJSON_AddItemToObjectCS(screen_object,"Floating",floating_bool);

    cJSON* aspect_ratio_bool = cJSON_CreateBool(aspect_ratio);
    cJSON_AddItemToObjectCS(screen_object,"Aspect ratio",aspect_ratio_bool);

    cJSON* interger_scale_bool = cJSON_CreateBool(interger_scale);
    cJSON_AddItemToObjectCS(screen_object,"Interger scale",interger_scale_bool);

    cJSON* interframe_blending_bool = cJSON_CreateBool(gb->ppu.interframe_blending);
    cJSON_AddItemToObjectCS(screen_object,"Interframe blending",interframe_blending_bool);

    cJSON* bilinear_filtering_bool = cJSON_CreateBool(bilinear_filtering);
    cJSON_AddItemToObjectCS(screen_object,"Bilinear filtering",bilinear_filtering_bool);

}

void screen_t::load(cJSON* settings_object){
    cJSON* screen_object = cJSON_GetObjectItemCaseSensitive(settings_object,"Screen");
    
    if(!screen_object || !cJSON_IsObject(screen_object)) return;

    cJSON* floating_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Floating");

    if(floating_bool && cJSON_IsBool(floating_bool)){
        _floating = cJSON_IsTrue(floating_bool);
    }

    cJSON* aspect_ratio_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Aspect ratio");

    if(aspect_ratio_bool && cJSON_IsBool(aspect_ratio_bool)){
        aspect_ratio = cJSON_IsTrue(aspect_ratio_bool);
    }

    cJSON* interger_scale_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Interger scale");

    if(interger_scale_bool && cJSON_IsBool(interger_scale_bool)){
        interger_scale = cJSON_IsTrue(interger_scale_bool);
    }

    cJSON* interframe_blending_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Interframe blending");

    if(interframe_blending_bool && cJSON_IsBool(interframe_blending_bool)){
        gb->ppu.interframe_blending = cJSON_IsTrue(interframe_blending_bool);
    }

    cJSON* bilinear_filtering_bool = cJSON_GetObjectItemCaseSensitive(screen_object,"Bilinear filtering");

    if(bilinear_filtering_bool && cJSON_IsBool(bilinear_filtering_bool)){
        set_bilinear_filtering(cJSON_IsTrue(bilinear_filtering_bool));
    }

    update_embedded_size();

    last_evail_size.x = 0.0f;
    last_evail_size.y = 0.0f;
}


void screen_t::set_embedded_scale(int new_scale){

    uint32_t flags = SDL_GetWindowFlags(window);
    
    if(flags & SDL_WINDOW_MAXIMIZED){
        SDL_RestoreWindow(window);
    }
    else if(flags & SDL_WINDOW_FULLSCREEN_DESKTOP){
        SDL_SetWindowFullscreen(window,0);
    }

    int main_menu_bar_height = ImGui::GetFrameHeight();

    embedded_rect.x = 0;
    embedded_rect.y = main_menu_bar_height;
    embedded_rect.w = gb_screen_width * new_scale;
    embedded_rect.h = gb_screen_height * new_scale;

    SDL_SetWindowSize(window,embedded_rect.w,embedded_rect.h + main_menu_bar_height);
}

void screen_t::update_embedded_size(){
    int window_width = 0;
    int window_height = 0;

    SDL_GetWindowSize(window,&window_width,&window_height);

    int main_menu_bar_height = 0;

    main_menu_bar_height = ImGui::GetFrameHeight();    
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


void screen_t::update_screen(){

    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,(void**)&pixels,&pitch);

    memcpy(pixels,gb_get_render_buffer(gb),gb_screen_length);

    SDL_UnlockTexture(texture);
}


void screen_t::event(SDL_Event& event){

    switch(event.type){
        case SDL_WINDOWEVENT:{
            if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                update_embedded_size();
            }
            break;
        }
        case SDL_KEYDOWN:{
            if(SDL_GetModState() & KMOD_ALT){
                if(event.key.keysym.scancode >= SDL_SCANCODE_1 && event.key.keysym.scancode <= SDL_SCANCODE_9){
                    int scale = (event.key.keysym.scancode - SDL_SCANCODE_1) + 1;
                    set_embedded_scale(scale);
                }
            }
            else{
                if(event.key.keysym.scancode == SDL_SCANCODE_F11){
                    if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP){
                        SDL_SetWindowFullscreen(window,0);
                    }
                    else{
                        SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                }
            }
            break;
        }
    }
}


void screen_t::render_menu_bar(){

    if(!ImGui::BeginMenu("Screen")) return;

    if(ImGui::BeginMenu("Scale")){

        int scale = 0;

        if(ImGui::MenuItem("1x","Alt+1")) scale = 1;
        if(ImGui::MenuItem("2x","Alt+2")) scale = 2;
        if(ImGui::MenuItem("3x","Alt+3")) scale = 3;
        if(ImGui::MenuItem("4x","Alt+4")) scale = 4;
        if(ImGui::MenuItem("5x","Alt+5")) scale = 5;
        if(ImGui::MenuItem("6x","Alt+6")) scale = 6;
        if(ImGui::MenuItem("7x","Alt+7")) scale = 7;
        if(ImGui::MenuItem("8x","Alt+8")) scale = 8;
        if(ImGui::MenuItem("9x","Alt+9")) scale = 9;

        if(scale > 0){
            set_embedded_scale(scale);
        }

        if(ImGui::MenuItem("FullScreen","F11")){
            if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP){
                SDL_SetWindowFullscreen(window,0);
            }
            else{
                SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
        }

        ImGui::EndMenu();
    }

    if(ImGui::MenuItem("Floating",nullptr,_floating)){
        _floating = !_floating;
    }

    if(ImGui::MenuItem("Aspect ratio",nullptr,aspect_ratio)){
        aspect_ratio = !aspect_ratio;

        update_embedded_size();

        last_evail_size.x = 0.0f;
        last_evail_size.y = 0.0f;
    }

    if(ImGui::MenuItem("Interger scale",nullptr,interger_scale)){
        interger_scale = !interger_scale;

        update_embedded_size();

        last_evail_size.x = 0.0f;
        last_evail_size.y = 0.0f;
    }

    if(ImGui::MenuItem("Interframe blending",nullptr,gb->ppu.interframe_blending)){
        gb->ppu.interframe_blending = !gb->ppu.interframe_blending;
    }

    if(ImGui::MenuItem("Bilinear filtering",nullptr,bilinear_filtering)){
        set_bilinear_filtering(!bilinear_filtering);
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

    _focused = ImGui::IsWindowFocused();

    ImGui::End();
}

void screen_t::render_embedded(){
    if(_floating) return;

    SDL_RenderCopy(renderer,texture,nullptr,&embedded_rect);
}