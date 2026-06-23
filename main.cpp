#include "core/gb.h"
#include "gui/utils.hpp"
#include "gui/object_viewer.hpp"
#include "gui/palette_viewer.hpp"
#include "gui/tilemap_viewer.hpp"


class file_selector_t {
public:
    enum{
        gigabytes = 0x01 << 0x1E,
        megabytes = 0x01 << 0x14,
        kilobytes = 0x01 << 0x0A
    };

    enum filter_type_t {
        filter_all_files,
        filter_gb_rom_files
    };

    struct path_part_t {
        std::string name;
        std::filesystem::path path;

        path_part_t(std::string _name,std::filesystem::path _path):
        name(_name),
        path(_path)
        {}
    };

    struct directory_entry_t {
        std::string name;
        std::filesystem::path path;
        uintmax_t size;
        std::time_t last_write_time;
        bool is_directory;

        directory_entry_t(std::string _name,std::string _path,uintmax_t _size,std::time_t _last_write_time,bool _is_directory):
        name(_name),
        path(_path),
        size(_size),
        last_write_time(_last_write_time),
        is_directory(_is_directory)
        {}
    };

    gb_t* gb = nullptr;

    bool opened = false;
        
    std::filesystem::path current_path;
    std::vector<path_part_t> current_path_parts;
    std::vector<directory_entry_t> current_directory_entries;
    
    std::chrono::steady_clock::time_point last_update;

    uint8_t sort_column_index;
    bool sort_ascending;

    char name_buffer[256] = {0};
    int current_filter = filter_gb_rom_files;

    const char* filters_name[2] = {
        "All files",
        "GB ROM files"
    };

    bool popup_opened;
    ImVec2 popup_pos;
    std::filesystem::path path_not_exists;

    int window_remaining_content_height;
    ImVec2 window_min;
    ImVec2 window_max;

    file_selector_t(gb_t* gb):gb(gb){
        
        set_current_path(std::filesystem::current_path());

        sort_column_index = 0;
        sort_ascending = true;

        popup_opened = false;

        ImGuiStyle style = ImGui::GetStyle();

        window_remaining_content_height = ImGui::GetFrameHeightWithSpacing() * 3.0f;

        window_min.x = style.WindowMinSize.x;
        window_min.y = window_remaining_content_height + 200 + (style.WindowPadding.y * 2.0f);

        window_max.x = FLT_MAX;
        window_max.y = FLT_MAX;
    }


    void set_current_path(std::filesystem::path path){
        current_path = path;
        
        current_path_parts.clear();
        
        std::filesystem::path current;

        #ifdef _WIN32
        auto it = current_path.begin();
        auto end = current_path.end();
        
        current /= *it++; //driver exmaple "C:"
        current /= *it++; //root path "\\"
        current_path_parts.emplace_back(current_path.begin()->string(),current);

        for(;it != end; ++it){
            current /= *it;
            current_path_parts.emplace_back(it->string(),current);
        }
        #else
        for(auto it = current_path.begin(); it != current_path.end(); ++it){
            current /= *it;
            current_path_parts.emplace_back(it->string(),current);
        }
        #endif
    }

    uintmax_t number_of_entries_in_directory(std::filesystem::path directory){
        uintmax_t n = 0;
        for(auto& entry : std::filesystem::directory_iterator(directory,std::filesystem::directory_options::skip_permission_denied)){
            n++;
        }
        return n;
    }

    std::time_t entry_last_write_time(std::filesystem::path entry){

        std::chrono::time_point sys_time_point = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            std::filesystem::last_write_time(entry) - 
            std::filesystem::file_time_type::clock::now() + 
            std::chrono::system_clock::now()
        );

        return std::chrono::system_clock::to_time_t(sys_time_point);
    }

    void sort_current_directory_entries(){
        std::sort(current_directory_entries.begin(),current_directory_entries.end(),[this](const directory_entry_t& a,const directory_entry_t& b){
            bool result = false;

            switch(sort_column_index){
                //Name
                case 0:{
                    std::string name1 = a.path.filename().string();
                    std::string name2 = b.path.filename().string();
                    result = (sort_ascending) ? name1 < name2 : name1 > name2;
                    break;
                }
                //Size
                case 1:{
                    result = (sort_ascending) ? a.size < b.size : a.size > b.size;
                    break;
                }
                //Date
                case 2:{
                    result = (sort_ascending) ? a.last_write_time < b.last_write_time : a.last_write_time > b.last_write_time;
                    break;
                }
            }

            return result;
        });
    }

    void load_current_directory_entries(){

        current_directory_entries.clear();

        for(auto& entry : std::filesystem::directory_iterator(current_path,std::filesystem::directory_options::skip_permission_denied)){
            
            std::filesystem::path entry_path = entry.path();

            bool is_directory = std::filesystem::is_directory(entry_path);

            std::string extension = entry_path.extension().string();

            if(is_directory || ((current_filter == filter_all_files) || (extension == ".gb" || extension == ".gbc"))){

                current_directory_entries.emplace_back(
                    (is_directory ? "[DIR] " : "[FILE] ") + entry_path.filename().string(),
                    entry_path,
                    is_directory ? number_of_entries_in_directory(entry_path) : std::filesystem::file_size(entry_path),
                    entry_last_write_time(entry_path),
                    is_directory
                );
            }
        }

        last_update = std::chrono::steady_clock::now();
    }

    const char* entry_date_formated(const directory_entry_t& entry){
        static char buffer[32] = {0};

        tm* local_timer = std::localtime(&entry.last_write_time);
        
        std::strftime(buffer,sizeof(buffer),"%d/%m/%Y %H:%M",local_timer);

        return buffer;
    }


    void render_directory(){

        directory_entry_t* selected_directory = nullptr;

        ImGuiListClipper clipper;
        clipper.Begin(current_directory_entries.size(),ImGui::GetTextLineHeightWithSpacing());

        while(clipper.Step()){

            for(int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row){
                
                directory_entry_t& entry = current_directory_entries[row];

                if(entry.is_directory){

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();

                    if(ImGui::Selectable(entry.name.c_str())){
                        selected_directory = &entry;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%lu itens",entry.size);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s",entry_date_formated(entry));
                }
                else{
                    
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();

                    if(ImGui::Selectable(entry.name.c_str())){
                        std::cout << entry.path << std::endl;
                        gb_insert_cartridge(gb,entry.path.c_str());
                        opened = false;
                    }

                    ImGui::TableNextColumn();
                    
                    if(entry.size >= gigabytes){
                        ImGui::Text("%.1f GB",(float)entry.size / (float)gigabytes);
                    }
                    else if(entry.size >= megabytes){
                        ImGui::Text("%.1f MB",(float)entry.size / (float)megabytes);
                    }
                    else if(entry.size >= kilobytes){
                        ImGui::Text("%.1f KB",(float)entry.size / (float)kilobytes);
                    }
                    else{
                        ImGui::Text("%lu B",entry.size);
                    }

                    ImGui::TableNextColumn();

                    ImGui::Text("%s",entry_date_formated(entry));
                }
            }
        }

        if(selected_directory != nullptr){
            set_current_path(selected_directory->path);
            load_current_directory_entries();
            sort_current_directory_entries();
        }
    }

    void render(){
        if(!opened) return;

        ImGui::SetNextWindowSizeConstraints(window_min,window_max);

        if(ImGui::Begin("Select File",&opened)){

            ImGuiStyle& style = ImGui::GetStyle();

            ImVec2 browser_table_size = ImVec2(
                0.0f,
                ImGui::GetContentRegionAvail().y - window_remaining_content_height
            );

            if(current_path_parts.size() > 0){
                
                auto it = current_path_parts.begin();
                auto end = current_path_parts.end();
                int id = 0;

                while(true){

                    ImGui::PushID(id++);
                    bool selected = ImGui::Button(it->name.c_str());
                    ImGui::PopID();

                    if(selected){
                        set_current_path(it->path);
                        load_current_directory_entries();
                        sort_current_directory_entries();
                        break;
                    }

                    if(++it != end){
                        ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
                    }
                    else{
                        break;
                    }
                    
                }
            }

            if(ImGui::BeginTable("BrowserTable",3,ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter,browser_table_size)){
                
                ImGui::TableSetupColumn("Name",ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Size",ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Date",ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableHeadersRow();

                ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();

                if(specs->SpecsDirty){
                    sort_column_index = specs->Specs->ColumnIndex;
                    sort_ascending = (specs->Specs->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
                    specs->SpecsDirty = false;
                    sort_current_directory_entries();
                }

                render_directory();

                ImGui::EndTable();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Name:");
            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if(ImGui::InputText("##NameInputText",name_buffer,sizeof(name_buffer),ImGuiInputTextFlags_EnterReturnsTrue)){
                
                std::filesystem::path path = name_buffer;

                if(!path.has_parent_path()){
                    path = current_path / path;
                }

                //removing ".",".."
                path = path.lexically_normal();

                //removing last "/" in path. example: before /a/b/ after /a/b
                if(!path.has_filename()){
                    path = path.parent_path();
                }

                if(std::filesystem::exists(path)){
                    if(std::filesystem::is_directory(path)){
                        set_current_path(path);
                        load_current_directory_entries();
                        sort_current_directory_entries();
                    }
                    else{
                        std::cout << path << std::endl;
                        gb_insert_cartridge(gb,path.c_str());
                        opened = false;
                    }
                }
                else{
                    path_not_exists = path;

                    ImGui::OpenPopup("The file could not be opened");

                    popup_opened = true;

                    ImVec2 window_pos = ImGui::GetWindowPos();
                    ImVec2 window_size = ImGui::GetWindowSize();
                    popup_pos = ImVec2(window_pos.x + window_size.x * 0.5f,window_pos.y + window_size.y * 0.5f);
                }
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Filters:");
            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if(ImGui::Combo("##FiltersCombo",&current_filter,filters_name,2)){
                load_current_directory_entries();
                sort_current_directory_entries();
            }


            ImGui::SetNextWindowPos(popup_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));

            if(ImGui::BeginPopupModal("The file could not be opened",&popup_opened,ImGuiWindowFlags_AlwaysAutoResize)){
                
                ImGui::Text("The file \"%s\" cannot be found",path_not_exists.c_str());
                
                ImGui::EndPopup();
            }

            if(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_update).count() >= 2){
                load_current_directory_entries();
                sort_current_directory_entries();
            }
        }

        ImGui::End();
    }
    
private:
};


class wave_form_t {
private:
    struct channel_frame_t {
        int16_t samples[gb_audio_frame_samples];
        int count;
    };

    gb_t *gb = nullptr;

    channel_frame_t square1 = {0};
    channel_frame_t square2 = {0};
    channel_frame_t wave = {0};
    channel_frame_t noise = {0};

    bool open = false;

    static void frame_callback(void* data){
        wave_form_t* wf = (wave_form_t*)data;
        gb_apu_t* apu = &wf->gb->apu;

        memcpy(wf->square1.samples,apu->square1_frame.samples,sizeof(wf->square1.samples));
        wf->square1.count = apu->square1_frame.samples_count;

        memcpy(wf->square2.samples,apu->square2_frame.samples,sizeof(wf->square2.samples));
        wf->square2.count = apu->square2_frame.samples_count;

        memcpy(wf->wave.samples,apu->wave_frame.samples,sizeof(wf->wave.samples));
        wf->wave.count = apu->wave_frame.samples_count;

        memcpy(wf->noise.samples,apu->noise_frame.samples,sizeof(wf->noise.samples));
        wf->noise.count = apu->noise_frame.samples_count;
    }

    static float get_sample(void* data,int idx){
        channel_frame_t* cf = (channel_frame_t*)data;
        return (float)cf->samples[idx]; 
    }

public:

    wave_form_t(gb_t* gb):gb(gb){}

    ~wave_form_t(){
        gb_remove_apu_callback(gb);
    }

    void render(){
        if(!open) return;

        bool _open = open;

        if(ImGui::Begin("Wave Form",&_open)){

            ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 table_size = ImGui::GetContentRegionAvail();
            float row_height = table_size.y / 2.0f;
            ImVec2 graph_size = ImVec2(-FLT_MIN,row_height - ImGui::GetFrameHeight() - style.CellPadding.y * 2.0f - style.ItemSpacing.y);

            if(ImGui::BeginTable("WavesTable",2,ImGuiTableFlags_None,table_size)){

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Checkbox("Square1",&gb->apu.square1.external_enabled);
                ImGui::PlotLines(
                    "##GraphSquare1",
                    get_sample,
                    &square1,
                    square1.count,
                    0,
                    nullptr,
                    gb_audio_channel_min_output,
                    gb_audio_channel_max_output,
                    graph_size
                );

                ImGui::TableNextColumn();
                ImGui::Checkbox("Square2",&gb->apu.square2.external_enabled);
                ImGui::PlotLines(
                    "##GraphSquare2",
                    get_sample,
                    &square2,
                    square2.count,
                    0,
                    nullptr,
                    gb_audio_channel_min_output,
                    gb_audio_channel_max_output,
                    graph_size
                );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Checkbox("Wave",&gb->apu.wave.external_enabled);
                ImGui::PlotLines(
                    "##GraphWave",
                    get_sample,
                    &wave,
                    wave.count,
                    0,
                    nullptr,
                    gb_audio_channel_min_output,
                    gb_audio_channel_max_output,
                    graph_size
                );

                ImGui::TableNextColumn();
                ImGui::Checkbox("Noise",&gb->apu.noise.external_enabled);
                ImGui::PlotLines(
                    "##GraphNoise",
                    get_sample,
                    &noise,
                    noise.count,
                    0,
                    nullptr,
                    gb_audio_channel_min_output,
                    gb_audio_channel_max_output,
                    graph_size
                );
                
                ImGui::EndTable();
            }

        }
        ImGui::End();

        set_open(_open);
    }

    void set_open(bool _open){
        if(open == _open) return;
        open = _open;
        if(open){
            gb_set_apu_callback(gb,frame_callback,this);
        }
        else{
            gb_remove_apu_callback(gb);
        }
    }

    bool get_open() const {
        return open;
    }
};


class screen_t {
public:
    enum {
        embedded_mode = 0,
        floating_mode = 1,
    };
    
    bool mode = embedded_mode;
    SDL_Texture* texture = nullptr;
    
    SDL_Rect embedded_rect{0};
    int embedded_scale = 0;

    ImVec2 floating_min_size;
    ImVec2 floating_max_size;

    ImVec2 floating_pos{0.0f,0.0f};
    ImVec2 floating_size{0.0f,0.0f};
    ImVec2 last_evail_size{0.0f,0.0f};

    screen_t(SDL_Renderer* renderer){
        texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STREAMING,gb_screen_width,gb_screen_height);

        ImGuiStyle& style = ImGui::GetStyle();

        floating_min_size.x = gb_screen_width + style.WindowPadding.x * 2.0f;
        floating_min_size.y = ImGui::GetFrameHeight() + gb_screen_height + style.WindowPadding.y * 2.0f;

        floating_max_size.x = FLT_MAX;
        floating_max_size.y = FLT_MAX;
    }

    ~screen_t(){
        SDL_DestroyTexture(texture);
    }

    void set_embedded_scale(SDL_Window* window,int new_scale){

        if(new_scale == embedded_scale) return;

        embedded_scale = new_scale;

        uint32_t flags = SDL_GetWindowFlags(window);
        
        if(flags & SDL_WINDOW_MAXIMIZED){
            SDL_RestoreWindow(window);
        }
        else if(flags & SDL_WINDOW_FULLSCREEN){
            SDL_SetWindowFullscreen(window,0);
        }

        int main_menu_bar_height = ImGui::GetFrameHeight();

        embedded_rect.x = 0;
        embedded_rect.y = main_menu_bar_height;
        embedded_rect.w = gb_screen_width * embedded_scale;
        embedded_rect.h = gb_screen_height * embedded_scale;

        SDL_SetWindowSize(window,embedded_rect.w,embedded_rect.h + main_menu_bar_height);
    }

    void update_embedded_size(SDL_Window* window){
        int window_width = 0;
        int window_height = 0;

        SDL_GetWindowSize(window,&window_width,&window_height);

        int main_menu_bar_height = ImGui::GetFrameHeight();

        window_height -= main_menu_bar_height;

        float ratio_scaleX = (float)window_width / gb_screen_width;
        float ratio_scaleY = (float)window_height / gb_screen_height;

        float ratio_scale = gb_min(ratio_scaleX,ratio_scaleY);

        embedded_rect.w = gb_screen_width * ratio_scale;
        embedded_rect.h = gb_screen_height * ratio_scale;

        embedded_rect.x = (window_width - embedded_rect.w) / 2;
        embedded_rect.y = main_menu_bar_height + (window_height - embedded_rect.h) / 2;
    }

    void clear(){
        uint8_t* pixels = nullptr;
        int pitch = 0;
        SDL_LockTexture(texture,NULL,(void**)&pixels,&pitch);
        memset(pixels,0,pitch * gb_screen_height);
        SDL_UnlockTexture(texture);
    }

    void render(){
        if(mode != floating_mode) return;

        ImGui::SetNextWindowSizeConstraints(floating_min_size,floating_max_size);

        if(ImGui::Begin("Screen",nullptr)){
            
            ImVec2 evail_size = ImGui::GetContentRegionAvail();
            
            if(evail_size.x != last_evail_size.x || evail_size.y != last_evail_size.y){

                float ratio_scale_x = evail_size.x / gb_screen_width;
                float ratio_scale_y = evail_size.y / gb_screen_height;

                float ratio_scale = gb_min(ratio_scale_x,ratio_scale_y);

                floating_size.x = gb_screen_width * ratio_scale;
                floating_size.y = gb_screen_height * ratio_scale;

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
};


class nanoboy_t {
    gb_t* gb = nullptr;

    nanoboy_t(){
        gb = gb_new();
    }

    ~nanoboy_t(){
        gb_delete(gb);
    }
};


void joypad_callback(void* data,gb_joypad_key_t* key){
    const uint8_t* keyboard = SDL_GetKeyboardState(NULL);
    key->down = keyboard[SDL_SCANCODE_S];
    key->up = keyboard[SDL_SCANCODE_W];
    key->left = keyboard[SDL_SCANCODE_A];
    key->right = keyboard[SDL_SCANCODE_D];
    key->start = keyboard[SDL_SCANCODE_P];
    key->select = keyboard[SDL_SCANCODE_O];
    key->a = keyboard[SDL_SCANCODE_L];
    key->b = keyboard[SDL_SCANCODE_K];
}

void audio_callback(void* userdata,uint8_t* data,int len){
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


int main(int n_args,char** args){

    gb_t* gb = gb_new();
    gb_set_joypad_callback(gb,joypad_callback,NULL);

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window = SDL_CreateWindow("NanoBoy - (0.0 fps)",0,0,0,0,SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window,renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();

    SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);

    SDL_AudioSpec audio_spec = {0};
    audio_spec.freq = gb_audio_sample_rate;
    audio_spec.format = AUDIO_S16;
    audio_spec.channels = gb_audio_channels;
    audio_spec.samples = 512;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = &gb->apu;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL,0,&audio_spec,NULL,0);

    SDL_PauseAudioDevice(audio_device,0);

    screen_t* screen = new screen_t(renderer);
    screen->set_embedded_scale(window,4);

    file_selector_t* file_selector = new file_selector_t(gb);
    
    tilemap_viewer_t* tilemap_viewer = new tilemap_viewer_t(gb,renderer);
    object_viewer_t* object_viewer = new object_viewer_t(gb,renderer);
    palette_viewer_t* palette_viewer = new palette_viewer_t(gb,renderer);

    wave_form_t* wave_form = new wave_form_t(gb);

    bool running = true;
    bool paused = false;
    SDL_Event event = {0};

    uint32_t frame_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    char buffer[256] = {0};

    while(running){

        if(!paused && gb->cartridge_inserted){
            uint64_t frame = gb->ppu.frame_count;
            while(frame == gb->ppu.frame_count){
                gb_cpu_execute(&gb->cpu);
            }
            uint8_t* pixels = NULL;
            int pitch = 0;
            SDL_LockTexture(screen->texture,NULL,(void**)&pixels,&pitch);
            memcpy(pixels,gb->ppu.screen,sizeof(gb->ppu.screen));
            SDL_UnlockTexture(screen->texture);
        }

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
                                gb_reset(gb);
                            }
                        }
                        else{
                            if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
                                paused = !paused;
                            }
                            else if(event.key.keysym.scancode == SDL_SCANCODE_EQUALS){
                                gb_set_speed(gb,gb->speed + gb_speed_step);
                                printf("speed: %f\n",gb->speed);
                            }
                            else if(event.key.keysym.scancode == SDL_SCANCODE_MINUS){
                                gb_set_speed(gb,gb->speed - gb_speed_step);
                                printf("speed: %f\n",gb->speed);
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

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();

        ImGui::NewFrame();

        if(ImGui::BeginMainMenuBar()){
            
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
                    paused = !paused;
                }

                if(ImGui::MenuItem("Reset","Ctrl+R",nullptr,gb->cartridge_inserted)){
                    gb_reset(gb);
                }

                if(ImGui::MenuItem("Increase speed","=",nullptr,gb->cartridge_inserted)){
                    gb_set_speed(gb,gb->speed + gb_speed_step);
                }
                
                if(ImGui::MenuItem("Decrease speed","-",nullptr,gb->cartridge_inserted)){
                    gb_set_speed(gb,gb->speed - gb_speed_step);
                }
                
                if(ImGui::MenuItem("Power off",nullptr,nullptr,gb->cartridge_inserted)){
                    gb_remove_cartridge(gb);

                    screen->clear();
                    
                    tilemap_viewer->set_open(false);
                    object_viewer->set_open(false);
                    palette_viewer->set_open(false);
                    wave_form->set_open(false);
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

                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu("Debug")){
                if(ImGui::MenuItem("Tilemap Viewer",nullptr,nullptr,gb->cartridge_inserted)){
                    tilemap_viewer->set_open(true);
                }
                if(ImGui::MenuItem("Object Viewer",nullptr,nullptr,gb->cartridge_inserted)){
                    object_viewer->set_open(true);
                }
                if(ImGui::MenuItem("Palette Viewer",nullptr,nullptr,gb->cartridge_inserted)){
                    palette_viewer->set_open(true);
                }
                if(ImGui::MenuItem("Wave Form",nullptr,nullptr,gb->cartridge_inserted)){
                    wave_form->set_open(true);
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        screen->render();

        file_selector->render();

        tilemap_viewer->render();
        object_viewer->render();
        palette_viewer->render();

        wave_form->render();

        ImGui::Render();

        SDL_RenderClear(renderer);
        
        if(screen->mode == screen->embedded_mode){
            SDL_RenderCopy(renderer,screen->texture,NULL,&screen->embedded_rect);
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        
        SDL_RenderPresent(renderer);

        frame_count++;
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time);
        
        if(elapsed.count() > 1000){
            last_time = current_time;
            float fps = frame_count / (elapsed.count() / 1000.0f);
            snprintf(buffer,sizeof(buffer),"NanoBoy - (%.1f fps)",fps);
            SDL_SetWindowTitle(window,buffer);
            frame_count = 0;
        }
    }

    delete wave_form;
    
    delete palette_viewer;
    delete object_viewer;
    delete tilemap_viewer;

    delete file_selector;

    delete screen;

    SDL_CloseAudioDevice(audio_device);

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    gb_delete(gb);

    return 0;
}