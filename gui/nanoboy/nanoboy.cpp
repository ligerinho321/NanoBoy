#include <gui/nanoboy/nanoboy.hpp>

static const char* file_extensions[] = {
    "All files\0",
    "GB ROM files\0.gb;.gbc",
    "GBA ROM files\0.gba"
};

static const int file_extensions_count = sizeof(file_extensions) / sizeof(file_extensions[0]);

nanoboy_t::nanoboy_t(){
    
    init_directories();
    init_sdl();
    init_imgui();

    int window_min_width = gb_screen_width;
    int window_min_height = gb_screen_height + ImGui::GetFrameHeight();

    SDL_SetWindowMinimumSize(window,window_min_width,window_min_height);

    SDL_SetWindowSize(window,window_min_width,window_min_height);

    SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);

    notification_manager.init();
    
    dmg_palette.init(this);
    boot_settings.init(this);
    rewind_settings.init(this);
    input_settings.init();
    shortcut_settings.init();

    file_selector.init();
    file_selector.set_extensions(file_extensions,file_extensions_count);
    file_selector.set_current_extension(1);
    file_selector.set_callback(file_selector_callback,this);

    savestate.init(this);

    screen.init(this);

    running = true;

    load_settings();
}

nanoboy_t::~nanoboy_t(){

    remove_cartridge();

    if(console != nullptr){
        delete console;
        console = nullptr;
    }

    save_settings();
    save_imgui_ini_settings();

    input_settings.uninit();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    ImGui::DestroyContext();

    SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


void nanoboy_t::audio_callback(void* userdata,uint8_t* data,int len){
    nanoboy_t* nanoboy = (nanoboy_t*)userdata;

    size_t result = nanoboy->console->read_samples(data,len);

    if(result < (size_t)len){
        memset(data + result,0,(size_t)len - result);
    }
}

void nanoboy_t::file_selector_callback(void* userdata,std::filesystem::path path){
    nanoboy_t* nanoboy = (nanoboy_t*)userdata;
    nanoboy->insert_cartridge(path);
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

    assets_load_window_icon(window);
    
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    SDL_AudioSpec audio_spec = {};
    audio_spec.freq = 44100;
    audio_spec.format = AUDIO_S16;
    audio_spec.channels = 2;
    audio_spec.samples = 512;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = this;

    audio_device = SDL_OpenAudioDevice(nullptr,0,&audio_spec,nullptr,0);
}

void nanoboy_t::init_imgui(){
    
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImFontConfig font_config = {};
    font_config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(font_data,font_size,16.0f,&font_config);

    load_imgui_ini_settings();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window,renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
}


void nanoboy_t::update_gb_model(){
    if(console != nullptr && console->type() == console_gb){
        gb_t* gb = (gb_t*)console->data();
        gb->is_cgb_pending = is_cgb;
    }
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
    cJSON_AddItemToObjectCS(window_object,"FullScreen",fullscreen_bool);
}

void nanoboy_t::load_window_settings(cJSON* settings_object){

    int x,y,w,h;
    SDL_GetWindowPosition(window,&x,&y);
    SDL_GetWindowSize(window,&w,&h);

    bool fullscreen = false;

    cJSON* window_object = cJSON_GetObjectItemCaseSensitive(settings_object,"Window");
    
    if(!cJSON_IsObject(window_object)) return;

    cJSON* x_number = cJSON_GetObjectItemCaseSensitive(window_object,"X");

    if(cJSON_IsNumber(x_number)){
        x = (int)cJSON_GetNumberValue(x_number);
    }

    cJSON* y_number = cJSON_GetObjectItemCaseSensitive(window_object,"Y");

    if(cJSON_IsNumber(y_number)){
        y = (int)cJSON_GetNumberValue(y_number);
    }

    cJSON* w_number = cJSON_GetObjectItemCaseSensitive(window_object,"Width");

    if(cJSON_IsNumber(w_number)){
        w = (int)cJSON_GetNumberValue(w_number);
    }

    cJSON* h_number = cJSON_GetObjectItemCaseSensitive(window_object,"Height");

    if(cJSON_IsNumber(h_number)){
        h = (int)cJSON_GetNumberValue(h_number);
    }

    cJSON* fullscreen_bool = cJSON_GetObjectItemCaseSensitive(window_object,"FullScreen");

    if(cJSON_IsBool(fullscreen_bool)){
        fullscreen = cJSON_IsTrue(fullscreen_bool);
    }

    SDL_SetWindowSize(window,w,h);
    SDL_SetWindowPosition(window,x,y);
    SDL_SetWindowFullscreen(window,fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}


void nanoboy_t::save_recent_roms(cJSON* settings_object){
    cJSON* recent_roms_array = cJSON_CreateArray();
    cJSON_AddItemToObjectCS(settings_object,"Recent ROMs",recent_roms_array);

    for(auto& rom : recent_roms){
        cJSON* rom_string = cJSON_CreateStringReference(rom.c_str());
        cJSON_AddItemToArray(recent_roms_array,rom_string);
    }
}

void nanoboy_t::load_recent_roms(cJSON* settings_object){

    recent_roms.clear();

    cJSON* recent_roms_array = cJSON_GetObjectItemCaseSensitive(settings_object,"Recent ROMs");
    
    if(!cJSON_IsArray(recent_roms_array)) return;

    cJSON* current_child = recent_roms_array->child;

    size_t n = 0;

    while(current_child != nullptr && n < nanoboy_t::max_recent_roms){
        if(cJSON_IsString(current_child)){
            recent_roms.push_back(current_child->valuestring);
            ++n;
        }
        current_child = current_child->next;
    }
}


void nanoboy_t::save_settings(){

    cJSON* settings_object = cJSON_CreateObject();

    save_window_settings(settings_object);

    save_recent_roms(settings_object);

    file_selector.save(settings_object);

    screen.save(settings_object);

    cJSON* is_cgb_bool = cJSON_CreateBool(is_cgb);
    cJSON_AddItemToObjectCS(settings_object,"Is CGB",is_cgb_bool);

    dmg_palette.save(settings_object);

    boot_settings.save(settings_object);

    rewind_settings.save(settings_object);

    input_settings.save(settings_object);

    shortcut_settings.save(settings_object);

    
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

    cJSON* is_cgb_bool = nullptr;

    if(!settings_object){
        gb_printf_error("cJSON_ParserWidthLength failed");
        goto end;
    }

    load_window_settings(settings_object);

    load_recent_roms(settings_object);

    file_selector.load(settings_object);

    screen.load(settings_object);

    is_cgb_bool = cJSON_GetObjectItemCaseSensitive(settings_object,"Is CGB");
    if(cJSON_IsBool(is_cgb_bool)){
        is_cgb = cJSON_IsTrue(is_cgb_bool);
    }

    dmg_palette.load(settings_object);

    boot_settings.load(settings_object);

    rewind_settings.load(settings_object);

    input_settings.load(settings_object);

    shortcut_settings.load(settings_object);
    
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
    char timestamp[64] = {};
    
    time_t current_time = time(nullptr);
    struct tm* lt = localtime(&current_time);
    
    strftime(timestamp,sizeof(timestamp),"%d%m%Y_%H%M%S",lt);
    
    std::string filename = rom_name + "_" + timestamp + ".png";

    std::filesystem::path path = screenshot_path / filename;

    if(console->take_screenshot(path.u8string().c_str())){
        notification_manager.push_notification("Screenshot Saved \"%s\"",filename.c_str());
    }
    else{
        notification_manager.push_notification("Failed To Save Screenshot \"%s\"",filename.c_str());
    }
}


void nanoboy_t::insert_cartridge(std::filesystem::path path){
    
    std::string recent_rom = path.u8string();

    recent_roms.remove(recent_rom);

    remove_cartridge();

    uint8_t* data = nullptr;
    size_t length = 0;

    if(!gb_load_file(path.u8string().c_str(),(void**)&data,&length)) goto fail;

    if(gb_cartridge_rom_is_valid(data,length)){

        if(console && console->type() != console_gb){
            delete console;
            console = nullptr;
        }

        if(!console){
            console = new gb_console_t(this);
            
            input_settings.set_current_console(console_gb);

            boot_settings.apply_to_console();
            dmg_palette.apply_to_console();
            rewind_settings.apply_to_console();

            savestate.update_console();
            screen.update_console();

            update_gb_model();
        }
    }
    else if(gba_cartridge_rom_is_valid(data,length)){

        if(console && console->type() != console_gba){
            delete console;
            console = nullptr;
        }

        if(!console){
            console = new gba_console_t(this);

            input_settings.set_current_console(console_gba);

            boot_settings.apply_to_console();
            rewind_settings.apply_to_console();

            savestate.update_console();
            screen.update_console();
        }
    }
    else{
        free(data);
        goto fail;
    }

    if(!console->insert_cartridge_from_memory(data,length)) goto fail;

    rom_path = path;
    rom_name = path.filename().replace_extension("").u8string();

    console->load_ram(get_rom_save_path().c_str());
    console->load_rtc(get_rom_rtc_path().c_str());
    console->load_cheats(get_rom_cheats_path().c_str());

    savestate.load(savestates_path,rom_name);

    SDL_PauseAudioDevice(audio_device,false);

    recent_roms.push_front(recent_rom);

    if(recent_roms.size() > nanoboy_t::max_recent_roms){
        recent_roms.pop_back();
    }

    notification_manager.push_notification("Loaded \"%s\"",path.filename().u8string().c_str());

    return;

    fail:
    notification_manager.push_notification("Failed To Load \"%s\"",path.filename().u8string().c_str());
}

void nanoboy_t::remove_cartridge(){

    if(!console || !console->cartridge_inserted()) return;

    SDL_PauseAudioDevice(audio_device,true);

    console->save_ram(get_rom_save_path().c_str());
    console->save_rtc(get_rom_rtc_path().c_str());
    console->save_cheats(get_rom_cheats_path().c_str());

    savestate.unload();

    rom_path.clear();
    rom_name.clear();

    console->remove_cartridge();

    SDL_SetWindowTitle(window,"NanoBoy");
}

void nanoboy_t::increase_speed(){
    console->increase_speed();
    notification_manager.push_notification("Speed %d%%",(int)(console->get_speed() * 100.0f));
}

void nanoboy_t::decrease_speed(){
    console->decrease_speed();
    notification_manager.push_notification("Speed %d%%",(int)(console->get_speed() * 100.0f));
}

void nanoboy_t::pause(){
    console->pause();

    if(console->paused()){
        notification_manager.push_notification("Paused");
    }
    else{
        notification_manager.push_notification("Resumed");
    }
}

void nanoboy_t::reset(){
    SDL_LockAudioDevice(audio_device);

    console->reset();

    SDL_UnlockAudioDevice(audio_device);

    notification_manager.push_notification("Reseted");
}


void nanoboy_t::file_shortcut_event(){
    bool cartridge_inserted = console != nullptr && console->cartridge_inserted();

    if(shortcut_settings.down(shortcut_settings_t::open_file)){
        file_selector.set_open(true);
    }

    if(shortcut_settings.down(shortcut_settings_t::take_screenshot) && cartridge_inserted){
        take_screenshot();
    }

    savestate.shortcut_event();

    if(shortcut_settings.down(shortcut_settings_t::exit)){
        running = false;
    }
}

void nanoboy_t::game_shortcut_event(){
    if(!console || !console->cartridge_inserted()) return;

    if(shortcut_settings.down(shortcut_settings_t::pause)){
        pause();
    }
    if(shortcut_settings.down(shortcut_settings_t::reset)){
        reset();
    }
    if(shortcut_settings.down(shortcut_settings_t::increase_speed)){
        increase_speed();
    }
    if(shortcut_settings.down(shortcut_settings_t::decrease_speed)){
        decrease_speed();
    }
    if(shortcut_settings.down(shortcut_settings_t::rewind_held)){
        if(console->start_rewind()){
            SDL_LockAudioDevice(audio_device);
        }
    }
    if(shortcut_settings.released(shortcut_settings_t::rewind_held)){
        if(console->end_rewind()){
            SDL_UnlockAudioDevice(audio_device);
        }
    }
    if(shortcut_settings.down(shortcut_settings_t::power_off)){
        remove_cartridge();
    }
}

void nanoboy_t::settings_shortcut_event(){

    screen.shortcut_event();

    if(shortcut_settings.down(shortcut_settings_t::dmg_palette_settings)){
        dmg_palette.open();
    }
    if(shortcut_settings.down(shortcut_settings_t::boot_settings)){
        boot_settings.open();
    }
    if(shortcut_settings.down(shortcut_settings_t::rewind_settings)){
        rewind_settings.open();
    }
    if(shortcut_settings.down(shortcut_settings_t::input_settings)){
        input_settings.open();
    }
    if(shortcut_settings.down(shortcut_settings_t::shortcut_settings)){
        shortcut_settings.open();
    }
}

void nanoboy_t::debug_shortcut_event(){
    if(!console || !console->cartridge_inserted()) return;

    if(shortcut_settings.down(shortcut_settings_t::debugger)){
        console->open_debugger();
    }
    if(shortcut_settings.down(shortcut_settings_t::register_viewer)){
        console->open_register_viewer();
    }
    if(shortcut_settings.down(shortcut_settings_t::event_viewer)){
        console->open_event_viewer();
    }
    if(shortcut_settings.down(shortcut_settings_t::memory_viewer)){
        console->open_memory_viewer();
    }
    if(shortcut_settings.down(shortcut_settings_t::tilemap_viewer)){
        console->open_tilemap_viewer();
    }
    if(shortcut_settings.down(shortcut_settings_t::tile_viewer)){
        console->open_tile_viewer();
    }
    if(shortcut_settings.down(shortcut_settings_t::object_viewer)){
        console->open_object_viewer();
    }
    if(shortcut_settings.down(shortcut_settings_t::palette_viewer)){
        console->open_palette_viewer();
    }
    if(shortcut_settings.down(shortcut_settings_t::wave_form)){
        console->open_wave_form();
    }
}

void nanoboy_t::tools_shortcut_event(){
    if(!console || !console->cartridge_inserted()) return;

    if(shortcut_settings.down(shortcut_settings_t::cheats)){
        console->open_cheats();
    }

    if(shortcut_settings.down(shortcut_settings_t::printer) && console->type() == console_gb){
        gb_console_t* gb_console = (gb_console_t*)console;
        gb_console->open_printer();
    }
}


void nanoboy_t::event(){
    SDL_Event event = {};

    input_settings.init_binding_frame();
    shortcut_settings.init_binding_frame();

    while(SDL_PollEvent(&event)){
        
        ImGui_ImplSDL2_ProcessEvent(&event);

        input_settings.process_binding_event(event);
        shortcut_settings.process_binding_event(event);

        input_settings.process_capture_event(event);
        shortcut_settings.process_capture_event(event);

        switch(event.type){
            case SDL_QUIT:{
                running = false;
                break;
            }
            case SDL_WINDOWEVENT:{
                if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                    screen.update_embedded_size();
                }
                break;
            }
        }
    }

    if(SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS){
        cursor.update();
    }
}


void nanoboy_t::render_main_menu_bar(){

    if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) return;
    
    if(!ImGui::BeginMainMenuBar()) return;
    
    bool cartridge_inserted = console != nullptr && console->cartridge_inserted();

    if(ImGui::BeginMenu("File")){
        
        if(ImGui::MenuItem("Open File",shortcut_settings.str_keyboard(shortcut_settings_t::open_file))){
            file_selector.set_open(true);
        }

        if(ImGui::BeginMenu("Recent",recent_roms.size() > 0)){
            std::string* selected = nullptr;
            for(auto& rom : recent_roms){
                if(ImGui::Selectable(rom.c_str())){
                    selected = &rom;
                }
            }
            if(selected != nullptr){
                insert_cartridge(*selected);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if(ImGui::MenuItem("Take Screenshot",shortcut_settings.str_keyboard(shortcut_settings_t::take_screenshot),nullptr,cartridge_inserted)){
            take_screenshot();
        }

        ImGui::Separator();

        savestate.render_menu_bar();

        ImGui::Separator();

        if(ImGui::MenuItem("Exit",shortcut_settings.str_keyboard(shortcut_settings_t::exit))){
            running = false;
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Game")){
        
        if(ImGui::MenuItem("Pause",shortcut_settings.str_keyboard(shortcut_settings_t::pause),nullptr,cartridge_inserted)){
            pause();
        }

        if(ImGui::MenuItem("Reset",shortcut_settings.str_keyboard(shortcut_settings_t::reset),nullptr,cartridge_inserted)){
            reset();
        }

        if(ImGui::MenuItem("Increase Speed",shortcut_settings.str_keyboard(shortcut_settings_t::increase_speed),nullptr,cartridge_inserted)){
            increase_speed();
        }

        if(ImGui::MenuItem("Decrease Speed",shortcut_settings.str_keyboard(shortcut_settings_t::decrease_speed),nullptr,cartridge_inserted)){
            decrease_speed();
        }

        if(ImGui::MenuItem("Power Off",shortcut_settings.str_keyboard(shortcut_settings_t::power_off),nullptr,cartridge_inserted)){
            remove_cartridge();
        }
        
        ImGui::EndMenu();
    }
    
    if(ImGui::BeginMenu("Settings")){

        screen.render_menu_bar();

        if(ImGui::BeginMenu("Model")){

            if(ImGui::MenuItem("Game Boy (DMG)",nullptr,!is_cgb)){
                is_cgb = false;
                update_gb_model();
            }
            if(ImGui::MenuItem("Game Boy Color (CGB)",nullptr,is_cgb)){
                is_cgb = true;
                update_gb_model();
            }

            ImGui::EndMenu();
        }

        if(ImGui::MenuItem("DMG Palette",shortcut_settings.str_keyboard(shortcut_settings_t::dmg_palette_settings))){
            dmg_palette.open();
        }

        if(ImGui::MenuItem("Boot",shortcut_settings.str_keyboard(shortcut_settings_t::boot_settings))){
            boot_settings.open();
        }

        if(ImGui::MenuItem("Rewind",shortcut_settings.str_keyboard(shortcut_settings_t::rewind_settings))){
            rewind_settings.open();
        }

        if(ImGui::MenuItem("Input",shortcut_settings.str_keyboard(shortcut_settings_t::input_settings))){
            input_settings.open();
        }

        if(ImGui::MenuItem("Shortcut",shortcut_settings.str_keyboard(shortcut_settings_t::shortcut_settings))){
            shortcut_settings.open();
        }
        
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Debug")){

        if(ImGui::MenuItem("Debugger",shortcut_settings.str_keyboard(shortcut_settings_t::debugger),nullptr,cartridge_inserted)){
            console->open_debugger();
        }
        if(ImGui::MenuItem("Register Viewer",shortcut_settings.str_keyboard(shortcut_settings_t::register_viewer),nullptr,cartridge_inserted)){
            console->open_register_viewer();
        }
        if(ImGui::MenuItem("Event Viewer",shortcut_settings.str_keyboard(shortcut_settings_t::event_viewer),nullptr,cartridge_inserted)){
            console->open_event_viewer();
        }
        if(ImGui::MenuItem("Memory Viewer",shortcut_settings.str_keyboard(shortcut_settings_t::memory_viewer),nullptr,cartridge_inserted)){
            console->open_memory_viewer();
        }
        if(ImGui::MenuItem("Tilemap Viewer",shortcut_settings.str_keyboard(shortcut_settings_t::tilemap_viewer),nullptr,cartridge_inserted)){
            console->open_tilemap_viewer();
        }
        if(ImGui::MenuItem("Tile Viewer",shortcut_settings.str_keyboard(shortcut_settings_t::tile_viewer),nullptr,cartridge_inserted)){
            console->open_tile_viewer();
        }
        if(ImGui::MenuItem("Object Viewer",shortcut_settings.str_keyboard(shortcut_settings_t::object_viewer),nullptr,cartridge_inserted)){
            console->open_object_viewer();
        }
        if(ImGui::MenuItem("Palette Viewer",shortcut_settings.str_keyboard(shortcut_settings_t::palette_viewer),nullptr,cartridge_inserted)){
            console->open_palette_viewer();
        }
        if(ImGui::MenuItem("Wave Form",shortcut_settings.str_keyboard(shortcut_settings_t::wave_form),nullptr,cartridge_inserted)){
            console->open_wave_form();
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Tools")){

        if(ImGui::MenuItem("Cheats",shortcut_settings.str_keyboard(shortcut_settings_t::cheats),nullptr,cartridge_inserted)){
            console->open_cheats();
        }

        if(ImGui::MenuItem("Printer",shortcut_settings.str_keyboard(shortcut_settings_t::printer),nullptr,cartridge_inserted && console->type() == console_gb)){
            gb_console_t* gb_console = (gb_console_t*)console;
            gb_console->open_printer();
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void nanoboy_t::imgui_render(){

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();

    if(screen.floating()){
        ImGui::DockSpaceOverViewport();
    }

    render_main_menu_bar();

    notification_manager.render();

    dmg_palette.render();
    boot_settings.render();
    rewind_settings.render();
    input_settings.render();
    shortcut_settings.render();

    file_selector.render();

    savestate.render();
    
    screen.render_floating();

    if(console != nullptr){
        console->render_imgui_windows();
    }

    ImGui::Render();
}

void nanoboy_t::sdl_render(){

    SDL_SetRenderDrawColor(renderer,0,0,0,0);

    SDL_RenderClear(renderer);

    screen.render_embedded();

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    if(console != nullptr && console->cartridge_inserted()){
        static char buffer[256] = {0};

        snprintf(buffer,sizeof(buffer),"NanoBoy - %s (%.1f fps)",rom_name.c_str(),console->fps());

        SDL_SetWindowTitle(window,buffer);
    }
}


void nanoboy_t::run(){
    
    while(running){

        event();

        file_shortcut_event();
        game_shortcut_event();
        settings_shortcut_event();
        debug_shortcut_event();
        tools_shortcut_event();

        if(console != nullptr && console->cartridge_inserted()){
            console->execute_frame();
            console->update_screen_texture();
        }

        imgui_render();

        sdl_render();
    } 
}