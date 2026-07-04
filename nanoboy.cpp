#include "nanoboy.hpp"

static void joypad_callback(void* data,gb_joypad_key_t* key){
    const uint8_t* keyboard = SDL_GetKeyboardState(NULL);

    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) return;

    key->down = keyboard[SDL_SCANCODE_S];
    key->up = keyboard[SDL_SCANCODE_W];
    key->left = keyboard[SDL_SCANCODE_A];
    key->right = keyboard[SDL_SCANCODE_D];
    key->start = keyboard[SDL_SCANCODE_P];
    key->select = keyboard[SDL_SCANCODE_O];
    key->a = keyboard[SDL_SCANCODE_L];
    key->b = keyboard[SDL_SCANCODE_K];
}

static void audio_callback(void* userdata,uint8_t* data,int len){
    gb_apu_t* apu = (gb_apu_t*)userdata;

    size_t readable = gb_ring_buffer_readable(&apu->ring_buffer);

    readable = gb_min(len,readable);

    if(readable > 0){
        gb_ring_buffer_read(&apu->ring_buffer,data,readable);
    }
    else{
        memset(data,0,len);
    }
}


nanoboy_t::nanoboy_t(){
    
    gb = gb_new();
    gb_thread_safe_set_joypad_callback(gb,joypad_callback,nullptr);

    init_directories();
    init_sdl();
    init_imgui();

    file_selector = new file_selector_t(this);

    screen = new screen_t(renderer);

    cheats = new cheats_t(gb);

    tilemap_viewer = new tilemap_viewer_t(gb,renderer);

    object_viewer = new object_viewer_t(gb,renderer);
    
    palette_viewer = new palette_viewer_t(gb,renderer);

    wave_form = new wave_form_t(gb);

    screen->set_embedded_scale(window,4);

    SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);

    running = true;
}

nanoboy_t::~nanoboy_t(){

    remove_cartridge();
    
    delete wave_form;
    delete palette_viewer;
    delete object_viewer;
    delete tilemap_viewer;
    delete cheats;
    delete screen;
    delete file_selector;

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    save_imgui_ini_settings();

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

    printf("windows main folder path: %s\n",main_folder_path.u8string().c_str());
#else
    const char* home = getenv("HOME");
    
    if(!home){
        home = "~";
    }
    
    main_folder_path = home;
    main_folder_path /= ".config";
    main_folder_path /= "nanoboy";

    printf("linux main folder path: %s\n",main_folder_path.u8string().c_str());
#endif

    saves_path = main_folder_path / "saves";
    savestates_path = main_folder_path / "savestates";
    cheats_path = main_folder_path / "cheats";

    std::filesystem::create_directories(main_folder_path);
    std::filesystem::create_directories(saves_path);
    std::filesystem::create_directories(savestates_path);
    std::filesystem::create_directories(cheats_path);
}

void nanoboy_t::init_sdl(){
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow("NanoBoy",0,0,0,0,SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    SDL_AudioSpec audio_spec = {0};
    audio_spec.freq = gb_audio_sample_rate;
    audio_spec.format = AUDIO_S16;
    audio_spec.channels = gb_audio_channels;
    audio_spec.samples = 512;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = &gb->apu;

    audio_device = SDL_OpenAudioDevice(nullptr,0,&audio_spec,nullptr,0);

    SDL_PauseAudioDevice(audio_device,0);
}

void nanoboy_t::init_imgui(){
    
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

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


void nanoboy_t::save_imgui_ini_settings(){
    ImGuiIO& io = ImGui::GetIO();

    if(io.WantSaveIniSettings){
        std::string imgui_ini_path = get_imgui_ini_path();

        printf("save imgui ini settings from \"%s\"\n", imgui_ini_path.c_str());

        ImGui::SaveIniSettingsToDisk((const char*)get_imgui_ini_path().c_str());

        io.WantSaveIniSettings = false;
    }
}

void nanoboy_t::load_imgui_ini_settings(){
    std::string imgui_ini_path = get_imgui_ini_path();

    printf("load imgui ini settings from \"%s\"\n", imgui_ini_path.c_str());

    ImGui::LoadIniSettingsFromDisk((const char*)imgui_ini_path.c_str());
}


void nanoboy_t::insert_cartridge(std::filesystem::path path){

    bool cheats_open = cheats->get_open();
    bool tilemap_viewer_open = tilemap_viewer->get_open();
    bool object_viewer_open = object_viewer->get_open();
    bool palette_viewer_open = palette_viewer->get_open();
    bool wave_form_open = wave_form->get_open();

    remove_cartridge();

    if(!gb_insert_cartridge(gb,(const char*)path.u8string().c_str())){
        return;
    }

    rom_path = path;
    rom_name = path.filename().replace_extension("").u8string();

    std::string rom_save_path = get_rom_save_path();
    std::string rom_cheat_path = get_rom_cheat_path();

    gb_load_ram(gb,rom_save_path.c_str());

    cheats->load(rom_cheat_path.c_str());

    cheats->set_open(cheats_open);
    tilemap_viewer->set_open(tilemap_viewer_open);
    object_viewer->set_open(object_viewer_open);
    palette_viewer->set_open(palette_viewer_open);
    wave_form->set_open(wave_form_open);

    gb_thread_start(gb);
}

void nanoboy_t::remove_cartridge(){

    if(!gb->cartridge_inserted) return;

    gb_thread_stop(gb);

    std::string rom_save_path = get_rom_save_path();
    std::string rom_cheat_path = get_rom_cheat_path();

    gb_save_ram(gb,rom_save_path.c_str());
    
    cheats->save(rom_cheat_path.c_str());
    cheats->clear();
    cheats->set_open(false);

    tilemap_viewer->set_open(false);
    tilemap_viewer->clear();

    object_viewer->set_open(false);
    object_viewer->clear();
    
    palette_viewer->set_open(false);
    palette_viewer->clear();

    wave_form->set_open(false);
    wave_form->clear();
    
    screen->clear();

    rom_path.clear();
    rom_name.clear();

    gb_remove_cartridge(gb);

    SDL_SetWindowTitle(window,"NanoBoy");
}


void nanoboy_t::event(){
    SDL_Event event{0};

    while(SDL_PollEvent(&event)){
        
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch(event.type){
            case SDL_QUIT:{
                running = false;
                break;
            }
            case SDL_WINDOWEVENT:{
                if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                    screen->update_embedded_size(window);
                }
                break;
            }
            case SDL_KEYDOWN:{
                if(gb->cartridge_inserted){
                    if(SDL_GetModState() & KMOD_CTRL){
                        if(event.key.keysym.scancode == SDL_SCANCODE_R){
                            gb_thread_safe_reset(gb);
                        }
                    }
                    else{
                        if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
                            gb_thread_safe_set_paused(gb,!gb->paused);
                        }
                        else if(event.key.keysym.scancode == SDL_SCANCODE_EQUALS){
                            gb_thread_safe_set_speed(gb,gb->speed + gb_speed_step);
                        }
                        else if(event.key.keysym.scancode == SDL_SCANCODE_MINUS){
                            gb_thread_safe_set_speed(gb,gb->speed - gb_speed_step);
                        }
                    }
                }
                if(SDL_GetModState() & KMOD_ALT){
                    if(event.key.keysym.scancode >= SDL_SCANCODE_1 && event.key.keysym.scancode <= SDL_SCANCODE_9){
                        int scale = (event.key.keysym.scancode - SDL_SCANCODE_1) + 1;
                        screen->set_embedded_scale(window,scale);
                    }
                }
                else{
                    if(event.key.keysym.scancode == SDL_SCANCODE_F11){
                        if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN){
                            SDL_SetWindowFullscreen(window,0);
                        }
                        else{
                            SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN);
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

    uint8_t* pixels = NULL;
    int pitch = 0;
    SDL_LockTexture(screen->texture,NULL,(void**)&pixels,&pitch);

    memcpy(pixels,gb_get_render_buffer(gb),gb_screen_length);

    SDL_UnlockTexture(screen->texture);
}


void nanoboy_t::render_main_menu_bar(){
    if(!ImGui::BeginMainMenuBar()) return;
        
    if(ImGui::BeginMenu("File")){
        
        if(ImGui::MenuItem("Open File")){
            file_selector->opened = true;
        }
        
        if(ImGui::MenuItem("Exit")){
            running = false;
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Game")){
        
        if(ImGui::MenuItem("Pause","Esq",nullptr,gb->cartridge_inserted)){
            gb_thread_safe_set_paused(gb,!gb->paused);
        }

        if(ImGui::MenuItem("Reset","Ctrl+R",nullptr,gb->cartridge_inserted)){
            gb_thread_safe_reset(gb);
        }

        if(ImGui::MenuItem("Increase speed","=",nullptr,gb->cartridge_inserted)){
            gb_thread_safe_set_speed(gb,gb->speed + gb_speed_step);
        }
        
        if(ImGui::MenuItem("Decrease speed","-",nullptr,gb->cartridge_inserted)){
            gb_thread_safe_set_speed(gb,gb->speed - gb_speed_step);
        }
        
        if(ImGui::MenuItem("Cheats",nullptr,nullptr,gb->cartridge_inserted)){
            cheats->set_open(true);
        }
        if(ImGui::MenuItem("Power off",nullptr,nullptr,gb->cartridge_inserted)){
            remove_cartridge();
        }
        
        ImGui::EndMenu();
    }
    
    if(ImGui::BeginMenu("Settings")){

        if(ImGui::BeginMenu("Screen Size")){

            bool fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
            int scale = fullscreen ? -1 : screen->embedded_scale;

            if(ImGui::MenuItem("1x","Alt+1",scale == 1)) scale = 1;
            if(ImGui::MenuItem("2x","Alt+2",scale == 2)) scale = 2;
            if(ImGui::MenuItem("3x","Alt+3",scale == 3)) scale = 3;
            if(ImGui::MenuItem("4x","Alt+4",scale == 4)) scale = 4;
            if(ImGui::MenuItem("5x","Alt+5",scale == 5)) scale = 5;
            if(ImGui::MenuItem("6x","Alt+6",scale == 6)) scale = 6;
            if(ImGui::MenuItem("7x","Alt+7",scale == 7)) scale = 7;
            if(ImGui::MenuItem("8x","Alt+8",scale == 8)) scale = 8;
            if(ImGui::MenuItem("9x","Alt+9",scale == 9)) scale = 9;

            if(scale > 0 && scale != screen->embedded_scale){
                screen->set_embedded_scale(window,scale);
            }

            if(ImGui::MenuItem("FullScreen","F11",fullscreen)){
                if(fullscreen){
                    SDL_SetWindowFullscreen(window,0);
                }
                else{
                    SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN);
                }
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Screen Mode")){
            if(ImGui::MenuItem("Embedded",nullptr,screen->mode == screen_t::embedded_mode)){
                screen->mode = screen_t::embedded_mode;
            }
            if(ImGui::MenuItem("Floating",nullptr,screen->mode == screen_t::floating_mode)){
                screen->mode = screen_t::floating_mode;
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Model")){
            
            bool type = gb->type_pending;

            if(ImGui::MenuItem("Game Boy (DMG)",nullptr,type == gb_dmg)){
                gb->type_pending = gb_dmg;
            }
            if(ImGui::MenuItem("Game Boy Color (CGB)",nullptr,type == gb_cgb)){
                gb->type_pending = gb_cgb;
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Execution Mode")){
            if(ImGui::MenuItem("Single Thread",nullptr,!gb->multi_thread)){
                gb_thread_safe_set_execution_mode(gb,false);
            }
            if(ImGui::MenuItem("Multi Thread",nullptr,gb->multi_thread)){
                gb_thread_safe_set_execution_mode(gb,true);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Debug")){
        if(ImGui::MenuItem("Tilemap Viewer",nullptr,nullptr,gb->cartridge_inserted)){
            tilemap_viewer->thread_safe_set_open(true);
        }
        if(ImGui::MenuItem("Object Viewer",nullptr,nullptr,gb->cartridge_inserted)){
            object_viewer->thread_safe_set_open(true);
        }
        if(ImGui::MenuItem("Palette Viewer",nullptr,nullptr,gb->cartridge_inserted)){
            palette_viewer->thread_safe_set_open(true);
        }
        if(ImGui::MenuItem("Wave Form",nullptr,nullptr,gb->cartridge_inserted)){
            wave_form->thread_safe_set_open(true);
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

    file_selector->render();
    screen->render();
    cheats->render();
    tilemap_viewer->render();
    object_viewer->render();
    palette_viewer->render();
    wave_form->render();

    ImGui::Render();
}

void nanoboy_t::sdl_render(){

    SDL_SetRenderDrawColor(renderer,0,0,0,0);

    SDL_RenderClear(renderer);

    if(screen->mode == screen->embedded_mode && gb->cartridge_inserted){
        SDL_RenderCopy(renderer, screen->texture, NULL, &screen->embedded_rect);
    }

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
}


void nanoboy_t::run(){

    char buffer[256] = {0};
    
    while(running){

        event();

        gb_run();

        imgui_render();
        sdl_render();

        if(gb->cartridge_inserted){

            snprintf(buffer,sizeof(buffer),"NanoBoy - %s (%.1f fps)",rom_name.c_str(),gb_get_fps(gb));

            SDL_SetWindowTitle(window,buffer);
        }            
    } 
}