#include <nanoboy.hpp>

static void joypad_callback(void* data,gb_joypad_state_t* state){

    nanoboy_t* nanoboy = (nanoboy_t*)data;

    if(ImGui::GetIO().WantCaptureKeyboard && !nanoboy->screen->focused()) return;

    input_settings_t* input_settings = nanoboy->input_settings;

    state->down = input_settings->button_pressed(gb_button_down);
    state->up = input_settings->button_pressed(gb_button_up);
    state->left = input_settings->button_pressed(gb_button_left);
    state->right = input_settings->button_pressed(gb_button_right);
    state->start = input_settings->button_pressed(gb_button_start);
    state->select = input_settings->button_pressed(gb_button_select);
    state->a = input_settings->button_pressed(gb_button_a);
    state->b = input_settings->button_pressed(gb_button_b);
}

static void audio_callback(void* userdata,uint8_t* data,int len){
    gb_t* gb = (gb_t*)userdata;
    size_t result = gb_read_samples(gb,data,len);
    if(result < (size_t)len){
        memset(data + result,0,(size_t)len - result);
    }
}


static const char* file_selector_extensions[] = {
    "All files\0.*",
    "GB ROM files\0.gb;.gbc"
};

static const int file_selector_extensions_count = sizeof(file_selector_extensions) / sizeof(file_selector_extensions[0]);


static void file_selector_callback(void* userdata,std::filesystem::path path){
    nanoboy_t* nanoboy = (nanoboy_t*)userdata;
    nanoboy->insert_cartridge(path);
}


nanoboy_t::nanoboy_t(){
    
    gb = gb_new();
    gb_joypad_set_callback(gb,joypad_callback,this);

    init_directories();
    init_sdl();
    init_imgui();

    int window_min_width = gb_screen_width;
    int window_min_height = gb_screen_height + ImGui::GetFrameHeight();

    SDL_SetWindowMinimumSize(window,window_min_width,window_min_height);

    SDL_SetWindowSize(window,window_min_width,window_min_height);

    SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);

    boot_settings = new boot_settings_t(gb);
    input_settings = new input_settings_t();

    file_selector = new file_selector_t();
    file_selector->set_extensions(file_selector_extensions,file_selector_extensions_count);
    file_selector->set_current_extension(1);
    file_selector->set_callback(file_selector_callback,this);

    savestate = new savestate_t(gb,renderer);

    screen = new screen_t(gb,window,renderer);

    cheats = new cheats_t(gb);

    printer = new printer_t(gb,renderer);

    debugger = new debugger_t(gb);
    tilemap_viewer = new tilemap_viewer_t(gb,renderer);
    tile_viewer = new tile_viewer_t(gb,renderer);
    object_viewer = new object_viewer_t(gb,renderer);
    palette_viewer = new palette_viewer_t(gb,renderer);
    wave_form = new wave_form_t(gb);

    running = true;

    load_settings();
}

nanoboy_t::~nanoboy_t(){

    remove_cartridge();

    save_settings();
    save_imgui_ini_settings();

    delete wave_form;
    delete palette_viewer;
    delete object_viewer;
    delete tile_viewer;
    delete tilemap_viewer;
    delete debugger;
    delete printer;
    delete cheats;
    delete screen;
    delete savestate;
    delete file_selector;
    delete input_settings;
    delete boot_settings;

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    ImGui::DestroyContext();

    SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    gb_delete(gb);
}


void nanoboy_t::init_directories(){
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    
    if(!home){
        home = "~";
    }
    
    main_folder_path = home;
    main_folder_path /= "AppData";
    main_folder_path /= "Roaming";
    main_folder_path /= "nanoboy";

    //printf("windows main folder path: %s\n",main_folder_path.u8string().c_str());
#else
    const char* home = getenv("HOME");
    
    if(!home){
        home = "~";
    }
    
    main_folder_path = home;
    main_folder_path /= ".config";
    main_folder_path /= "nanoboy";

    //printf("linux main folder path: %s\n",main_folder_path.u8string().c_str());
#endif

    saves_path = main_folder_path / "saves";
    savestates_path = main_folder_path / "savestates";
    cheats_path = main_folder_path / "cheats";
    screenshot_path = main_folder_path / "screenshot";

    std::filesystem::create_directories(main_folder_path);
    std::filesystem::create_directories(saves_path);
    std::filesystem::create_directories(savestates_path);
    std::filesystem::create_directories(cheats_path);
    std::filesystem::create_directories(screenshot_path);
}

void nanoboy_t::init_sdl(){

    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow("NanoBoy",0,0,0,0,SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    SDL_AudioSpec audio_spec = {};
    audio_spec.freq = gb_audio_sample_rate;
    audio_spec.format = AUDIO_S16;
    audio_spec.channels = gb_audio_channels;
    audio_spec.samples = 512;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = gb;

    audio_device = SDL_OpenAudioDevice(nullptr,0,&audio_spec,nullptr,0);
}

void nanoboy_t::init_imgui(){
    
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    load_imgui_ini_settings();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window,renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
}


void nanoboy_t::save_window_settings(cJSON* settings_object){
    
    int width,height;
    SDL_GetWindowSize(window,&width,&height);

    int x,y;
    SDL_GetWindowPosition(window,&x,&y);

    cJSON* window_object = cJSON_CreateObject();
    cJSON_AddItemToObjectCS(settings_object,"Window",window_object);

    cJSON* x_number = cJSON_CreateNumber(x);
    cJSON_AddItemToObjectCS(window_object,"X",x_number);

    cJSON* y_number = cJSON_CreateNumber(y);
    cJSON_AddItemToObjectCS(window_object,"Y",y_number);

    cJSON* width_number = cJSON_CreateNumber(width);
    cJSON_AddItemToObjectCS(window_object,"Width",width_number);

    cJSON* height_number = cJSON_CreateNumber(height);
    cJSON_AddItemToObjectCS(window_object,"Height",height_number);

    cJSON* fullscreen_bool = cJSON_CreateBool(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
    cJSON_AddItemToObjectCS(window_object,"Fullscreen",fullscreen_bool);
}

void nanoboy_t::load_window_settings(cJSON* settings_object){

    int x,y,w,h;
    SDL_GetWindowPosition(window,&x,&y);
    SDL_GetWindowSize(window,&w,&h);

    bool fullscreen = false;

    cJSON* window_object = cJSON_GetObjectItemCaseSensitive(settings_object,"Window");
    
    if(!window_object || !cJSON_IsObject(window_object)) return;

    cJSON* x_number = cJSON_GetObjectItemCaseSensitive(window_object,"X");

    if(x_number && cJSON_IsNumber(x_number)){
        x = (int)cJSON_GetNumberValue(x_number);
    }

    cJSON* y_number = cJSON_GetObjectItemCaseSensitive(window_object,"Y");

    if(y_number && cJSON_IsNumber(y_number)){
        y = (int)cJSON_GetNumberValue(y_number);
    }

    cJSON* w_number = cJSON_GetObjectItemCaseSensitive(window_object,"Width");

    if(w_number && cJSON_IsNumber(w_number)){
        w = (int)cJSON_GetNumberValue(w_number);
    }

    cJSON* h_number = cJSON_GetObjectItemCaseSensitive(window_object,"Height");

    if(h_number && cJSON_IsNumber(h_number)){
        h = (int)cJSON_GetNumberValue(h_number);
    }

    cJSON* fullscreen_bool = cJSON_GetObjectItemCaseSensitive(window_object,"Fullscreen");

    if(fullscreen_bool && cJSON_IsBool(fullscreen_bool)){
        fullscreen = cJSON_IsTrue(fullscreen_bool);
    }

    SDL_SetWindowSize(window,w,h);
    SDL_SetWindowPosition(window,x,y);
    SDL_SetWindowFullscreen(window,fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}


void nanoboy_t::save_settings(){

    cJSON* settings_object = cJSON_CreateObject();

    if(!settings_object){
        gb_printf_error("cJSON_CreateObject failed");
        return;
    }

    save_window_settings(settings_object);

    file_selector->save(settings_object);
    screen->save(settings_object);
    boot_settings->save(settings_object);
    input_settings->save(settings_object);

    char* settings_string = cJSON_Print(settings_object);

    gb_save_file(get_settings_path().c_str(),settings_string,strlen(settings_string));

    free(settings_string);
    
    cJSON_Delete(settings_object);
}

void nanoboy_t::load_settings(){
    char* data = nullptr;
    size_t len = 0;

    if(!gb_load_file(get_settings_path().c_str(),(void**)&data,&len)){
        return;
    }

    cJSON* settings_object = cJSON_ParseWithLength(data,len);

    if(!settings_object){
        gb_printf_error("cJSON_ParserWidthLength failed");
        goto end;
    }

    load_window_settings(settings_object);

    file_selector->load(settings_object);
    screen->load(settings_object);
    boot_settings->load(settings_object);
    input_settings->load(settings_object);

    end:
    free(data);
    cJSON_Delete(settings_object);
}


void nanoboy_t::save_imgui_ini_settings(){
    ImGuiIO& io = ImGui::GetIO();

    if(io.WantSaveIniSettings){
        std::string imgui_ini_path = get_imgui_ini_path();

        //printf("save imgui ini settings from \"%s\"\n", imgui_ini_path.c_str());

        ImGui::SaveIniSettingsToDisk((const char*)get_imgui_ini_path().c_str());

        io.WantSaveIniSettings = false;
    }
}

void nanoboy_t::load_imgui_ini_settings(){
    std::string imgui_ini_path = get_imgui_ini_path();

    //printf("load imgui ini settings from \"%s\"\n", imgui_ini_path.c_str());

    ImGui::LoadIniSettingsFromDisk((const char*)imgui_ini_path.c_str());
}


void nanoboy_t::take_screenshot(){
    char timestamp[64] = {0};
    
    time_t current_time = time(nullptr);
    struct tm* lt = localtime(&current_time);
    
    strftime(timestamp,sizeof(timestamp),"%d%m%Y_%H%M%S",lt);
    
    std::filesystem::path path = screenshot_path / (rom_name + "_" + timestamp + ".png");

    if(!stbi_write_png(path.u8string().c_str(),gb_screen_width,gb_screen_height,gb_screen_bytes_per_pixel,gb_get_render_buffer(gb),gb_screen_pitch)){
        gb_printf_error("stbi_write_png failed");
    }
}


void nanoboy_t::insert_cartridge(std::filesystem::path path){
    
    remove_cartridge();

    if(!gb_insert_cartridge(gb,(const char*)path.u8string().c_str())){
        return;
    }

    rom_path = path;
    rom_name = path.filename().replace_extension("").u8string();

    gb_load_ram(gb,get_rom_save_path().c_str());

    gb_load_rtc(gb,get_rom_rtc_path().c_str());

    savestate->load(savestates_path,rom_name);

    cheats->load(get_rom_cheat_path().c_str(),false);

    SDL_PauseAudioDevice(audio_device,false);

    gb_thread_start(gb);
}

void nanoboy_t::remove_cartridge(){

    if(!gb->cartridge_inserted) return;

    gb_thread_stop(gb);

    SDL_PauseAudioDevice(audio_device,true);

    gb_save_ram(gb,get_rom_save_path().c_str());
    
    gb_save_rtc(gb,get_rom_rtc_path().c_str());

    savestate->unload();

    cheats->save(get_rom_cheat_path().c_str());
    cheats->clear(false);

    printer->clear();

    tilemap_viewer->clear();
    tile_viewer->clear();
    object_viewer->clear();
    palette_viewer->clear();

    wave_form->clear();
    
    screen->clear();

    rom_path.clear();
    rom_name.clear();

    gb_remove_cartridge(gb);

    SDL_SetWindowTitle(window,"NanoBoy");
}

void nanoboy_t::set_speed(float speed){
    gb_thread_stop(gb);

    gb_set_speed(gb,speed);

    gb_thread_start(gb);
}

void nanoboy_t::pause(){
    gb_thread_stop(gb);

    gb_pause(gb,!gb->paused);

    gb_thread_start(gb);
}

void nanoboy_t::reset(){
    gb_thread_stop(gb);

    SDL_LockAudioDevice(audio_device);

    gb_reset(gb);

    SDL_UnlockAudioDevice(audio_device);

    gb_thread_start(gb);
}


void nanoboy_t::event(){
    SDL_Event event{0};
    
    while(SDL_PollEvent(&event)){
        
        ImGui_ImplSDL2_ProcessEvent(&event);

        input_settings->event(event);
        savestate->event(event);
        screen->event(event);

        switch(event.type){
            case SDL_QUIT:{
                running = false;
                break;
            }
            case SDL_KEYDOWN:{
                if(gb->cartridge_inserted){
                    if(SDL_GetModState() & KMOD_CTRL){
                        if(event.key.keysym.scancode == SDL_SCANCODE_R){
                            reset();
                        }
                    }
                    else{
                        if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
                            pause();
                        }
                        else if(event.key.keysym.scancode == SDL_SCANCODE_EQUALS){
                            set_speed(gb->speed + gb_speed_step);
                        }
                        else if(event.key.keysym.scancode == SDL_SCANCODE_MINUS){
                            set_speed(gb->speed - gb_speed_step);
                        }
                        else if(event.key.keysym.scancode == SDL_SCANCODE_F12){
                            take_screenshot();
                        }
                    }
                }
                break;
            }
        }
    }
}


void nanoboy_t::gb_run(){
    if(!gb->cartridge_inserted) return;

    gb_execute_frame(gb);

    screen->update_screen();
}


void nanoboy_t::render_main_menu_bar(){

    bool fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;

    if(fullscreen && gb->cartridge_inserted) return;

    if(!ImGui::BeginMainMenuBar()) return;
        
    if(ImGui::BeginMenu("File")){
        
        if(ImGui::MenuItem("Open File")){
            file_selector->set_open(true);
        }

        if(ImGui::MenuItem("Take Screenshot","F12",nullptr,gb->cartridge_inserted)){
            take_screenshot();
        }

        ImGui::Separator();

        savestate->render_menu_bar();

        ImGui::Separator();

        if(ImGui::MenuItem("Exit")){
            running = false;
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Game")){
        
        if(ImGui::MenuItem("Pause","Esq",nullptr,gb->cartridge_inserted)){
            pause();
        }

        if(ImGui::MenuItem("Reset","Ctrl+R",nullptr,gb->cartridge_inserted)){
            reset();
        }

        if(ImGui::MenuItem("Increase speed","=",nullptr,gb->cartridge_inserted)){
            set_speed(gb->speed + gb_speed_step);
        }
        
        if(ImGui::MenuItem("Decrease speed","-",nullptr,gb->cartridge_inserted)){
            set_speed(gb->speed - gb_speed_step);
        }
        
        if(ImGui::MenuItem("Cheats",nullptr,nullptr,gb->cartridge_inserted)){
            cheats->set_open(true);
        }

        if(ImGui::MenuItem("Printer",nullptr,nullptr,gb->cartridge_inserted)){
            printer->set_open(true);
        }

        if(ImGui::MenuItem("Power off",nullptr,nullptr,gb->cartridge_inserted)){
            remove_cartridge();
        }
        
        ImGui::EndMenu();
    }
    
    if(ImGui::BeginMenu("Settings")){

        screen->render_menu_bar();

        if(ImGui::BeginMenu("Model")){
            
            if(ImGui::MenuItem("Game Boy (DMG)",nullptr,!gb->is_cgb_pending) && gb->is_cgb_pending){
                gb->is_cgb_pending = false;
            }
            if(ImGui::MenuItem("Game Boy Color (CGB)",nullptr,gb->is_cgb_pending) && !gb->is_cgb_pending){
                gb->is_cgb_pending = true;
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Execution Mode")){
            if(ImGui::MenuItem("Single Thread",nullptr,!gb->multi_thread) && gb->multi_thread){
                gb_thread_stop(gb);
                gb->multi_thread = false;
            }
            if(ImGui::MenuItem("Multi Thread",nullptr,gb->multi_thread) && !gb->multi_thread){
                gb->multi_thread = true;
                gb_thread_start(gb);
            }
            ImGui::EndMenu();
        }

        if(ImGui::MenuItem("Boot")){
            boot_settings->open();
        }

        if(ImGui::MenuItem("Input")){
            input_settings->open();
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Debug")){
        if(ImGui::MenuItem("Debugger",nullptr,nullptr,gb->cartridge_inserted)){
            debugger->set_open(true);
        }
        if(ImGui::MenuItem("Tilemap Viewer",nullptr,nullptr,gb->cartridge_inserted)){
            tilemap_viewer->set_open(true);
        }
        if(ImGui::MenuItem("Tile Viewer",nullptr,nullptr,gb->cartridge_inserted)){
            tile_viewer->set_open(true);
        }
        if(ImGui::MenuItem("Object Viewer",nullptr,nullptr,gb->cartridge_inserted)){
            object_viewer->set_open(true);
        }
        if(ImGui::MenuItem("Palette Viewer",nullptr,nullptr,gb->cartridge_inserted)){
            palette_viewer->set_open(true);
        }
        if(ImGui::MenuItem("Wave Form",nullptr,nullptr,gb->cartridge_inserted)){
            wave_form->set_open(true,false);
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void nanoboy_t::imgui_render(){

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    render_main_menu_bar();

    boot_settings->render();
    input_settings->render();

    file_selector->render();

    savestate->render();
    
    screen->render_floating();
    
    cheats->render();
    printer->render();
    
    debugger->render();
    tilemap_viewer->render();
    tile_viewer->render();
    object_viewer->render();
    palette_viewer->render();
    wave_form->render();

    ImGui::Render();
}

void nanoboy_t::sdl_render(){

    SDL_SetRenderDrawColor(renderer,0,0,0,0);

    SDL_RenderClear(renderer);

    screen->render_embedded();

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    if(gb->cartridge_inserted){
        static char buffer[256] = {0};

        snprintf(buffer,sizeof(buffer),"NanoBoy - %s (%.1f fps)",rom_name.c_str(),gb_get_fps(gb));

        SDL_SetWindowTitle(window,buffer);
    }
}


void nanoboy_t::run(){
    
    while(running){

        event();

        gb_run();

        imgui_render();

        sdl_render();
    } 
}