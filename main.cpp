#include "core/gb.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL2/SDL.h>

#include <iostream>
#include <chrono>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <string>
#include <atomic>

constexpr uintmax_t gigabytes = 0x01 << 0x1E;
constexpr uintmax_t megabytes = 0x01 << 0x14;
constexpr uintmax_t kilobytes = 0x01 << 0x0A;

const char* filters_name[2] = {
    "All files",
    "GB ROM files"
};

class file_selector_t {
public:
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

    bool opened;
        
    std::filesystem::path current_path;
    std::vector<path_part_t> current_path_parts;
    std::vector<directory_entry_t> current_directory_entries;
    
    std::chrono::steady_clock::time_point last_update;

    uint8_t sort_column_index;
    bool sort_ascending;

    char name_buffer[256];
    int current_filter;

    bool popup_opened;
    ImVec2 popup_pos;
    std::filesystem::path path_not_exists;

    int window_remaining_content_height;
    ImVec2 window_min;
    ImVec2 window_max;

    file_selector_t(){
        opened = false;
        
        current_filter = filter_gb_rom_files;

        memset(name_buffer,0,sizeof(name_buffer));

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

            if(is_directory || ((current_filter == filter_all_files) || (entry_path.extension().string() == ".gb"))){

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

class tilemap_viewer_t {
public:
    gb_t* gb = nullptr;

    bool opened = false;

    SDL_Texture* tilemap_texture[2];
    
    ImU32 grid_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,0.5f));
    ImU32 scroll_overlay_border_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,1.0f));
    ImU32 scroll_overlay_background_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,0.2f));

    bool show_tile_grid = false;
    bool show_scroll_overlay = false;

    const float min_scale = 1.0f;
    const float max_scale = 10.0f;
    float scale = min_scale;

    bool cgb_mode;
    bool bg_and_window_tiledata_area;
    uint8_t scx = 0;
    uint8_t scy = 0;
    uint8_t bgp;
    gb_rgb_t cgb_bg_cram_converted[0x20];
    uint8_t vram[0x02][0x2000];

    std::atomic_bool update;

    tilemap_viewer_t(SDL_Renderer* renderer){
        tilemap_texture[0] = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB888,SDL_TEXTUREACCESS_STREAMING,256,256);
        tilemap_texture[1] = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB888,SDL_TEXTUREACCESS_STREAMING,256,256);
    }

    ~tilemap_viewer_t(){
        SDL_DestroyTexture(tilemap_texture[0]);
        SDL_DestroyTexture(tilemap_texture[1]);
    }

    static void callback(void* data){
        tilemap_viewer_t* tmv = (tilemap_viewer_t*)data;
        gb_t* gb = tmv->gb;
        tmv->cgb_mode = gb->cgb_mode;
        tmv->bg_and_window_tiledata_area = gb->ppu.lcdc.bg_and_window_tiledata_area;
        tmv->scx = gb->ppu.scx;
        tmv->scy = gb->ppu.scy;
        tmv->bgp = gb->palette.bgp;
        memcpy(tmv->cgb_bg_cram_converted,gb->palette.cgb_bg_cram_converted,sizeof(tmv->cgb_bg_cram_converted));
        memcpy(tmv->vram,gb->memory.vram,sizeof(tmv->vram));
        tmv->update.store(true,std::memory_order_relaxed);
    }

    gb_rgb_t get_dmg_color(uint8_t palette_index){
        return dmg_palette[(bgp >> ((palette_index & 0x03) << 0x01)) & 0x03];
    }
    
    gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index){
        return cgb_bg_cram_converted[((palette_index & 0x07) << 0x02) | (color_index & 0x03)];
    }
    
    gb_rgb_t get_cgb_dmg_color(uint8_t palette_index){
        return cgb_bg_cram_converted[(bgp >> ((palette_index & 0x03) << 0x01)) & 0x03];
    }

    void update_tilemap_texture(uint8_t map_index){
        uint16_t base_address = (map_index == 0x01) ? 0x0C00 : 0x0800;
        uint8_t* map = vram[0x00] + base_address;
        uint8_t* map_attribute = vram[0x01] + base_address;

        gb_rgb_t color = {0};

        uint8_t* pixels = NULL;
        int pitch = 0;
        SDL_LockTexture(tilemap_texture[map_index],NULL,(void**)&pixels,&pitch);

        for(int row = 0; row < 32; ++row){
            for(int col = 0; col < 32; ++col){
                
                uint8_t index = (row << 0x05) | col;
                
                uint8_t tile_index = map[index];

                uint8_t attribute = cgb_mode ? map_attribute[index] : 0x00;

                uint16_t tile_address = (bg_and_window_tiledata_area ? 0x1000 + ((int8_t)tile_index << 0x04) : tile_index << 0x04);

                uint8_t* tile_data = vram[(attribute & 0x08) ? 0x01 : 0x00] + tile_address;

                for(int y = 0; y < 8; ++y){
                    
                    uint8_t tile_byte_address = ((attribute & 0x40) ? 0x07 ^ y : y) << 0x01;

                    uint8_t lo = tile_data[tile_byte_address + 0x00];
                    uint8_t hi = tile_data[tile_byte_address + 0x01];
                    
                    for(int x = 0; x < 8; ++x){
                        uint8_t bit = 1 << ((attribute & 0x20) ? x : 0x07 ^ x);
                        uint8_t index = ((hi & bit) ? 0x02 : 0x00) | ((lo & bit) ? 0x01 : 0x00);

                        if(gb->type == gb_cgb){
                            if(cgb_mode){
                                color = get_cgb_color(attribute & 0x07,index);
                            }
                            else{
                                color = get_cgb_dmg_color(index);
                            }
                        }
                        else{
                            color = get_dmg_color(index);
                        }

                        uint8_t* pixel = pixels + (((row << 0x03) | y) * pitch) + (((col << 0x03) | x) * 3);
                        pixel[0] = color.r;
                        pixel[1] = color.g;
                        pixel[2] = color.b;
                    }
                }
            }
        }

        SDL_UnlockTexture(tilemap_texture[map_index]);
    }

    void render_grid(){
        ImVec2 start = ImGui::GetItemRectMin();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        for(int i = 0; i < 33; ++i){
            draw_list->AddLineH(start.x,start.x + 256 * scale,start.y + i * 8 * scale,grid_color);
            draw_list->AddLineV(start.x + i * 8 * scale,start.y,start.y + 256 * scale,grid_color);
        }
    }

    void render_scroll_overlay(){
        ImVec2 start = ImGui::GetItemRectMin();
        ImVec2 end = ImGui::GetItemRectMax();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        int scx_right = scx + 143;
        int scy_bottom = scy + 159;
        ImVec2 min;
        ImVec2 max;

        if(scx_right > 255 && scy_bottom > 255){
            min = ImVec2(start.x,start.y);
            max = ImVec2(min.x + (scx_right - 255) * scale,min.y + (scy_bottom - 255) * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);

            min = ImVec2(start.x + scx * scale,start.y);
            max = ImVec2(end.x,min.y + (scy_bottom - 255) * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);

            min = ImVec2(start.x,start.y + scy * scale);
            max = ImVec2(min.x + (scx_right - 255) * scale,end.y);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);

            min = ImVec2(start.x + scx * scale,start.y + scy * scale);
            max = end;
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);
        }
        else if(scx_right > 255){
            min = ImVec2(start.x,start.y + scy * scale);
            max = ImVec2(start.x + (scx_right - 255) * scale,min.y + 160 * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);

            min = ImVec2(start.x + scx * scale,start.y + scy * scale);
            max = ImVec2(end.x,min.y + 160 * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);
        }
        else if(scy_bottom > 255){
            min = ImVec2(start.x + scx * scale,start.y);
            max = ImVec2(min.x + 144 * scale,start.y + (scy_bottom - 255) * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);

            min = ImVec2(start.x + scx * scale,start.y + scy * scale);
            max = ImVec2(min.x + 144 * scale,end.y);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);
        }
        else{
            min = ImVec2(start.x + scx * scale,start.y + scy * scale);
            max = ImVec2(min.x + 144 * scale,min.y + 160 * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);
        }

    }

    void render(){
        
        if(!opened) return;

        if(ImGui::Begin("Tilemap Viewer",&opened)){

            if(ImGui::BeginTable("LayoutTable",2,ImGuiTableFlags_Borders)){

                ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();

                if(ImGui::BeginTabBar("AddressPointer")){
                    
                    if(ImGui::BeginTabItem("9800")){
                        
                        if(ImGui::BeginChild("Tilemap",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
                            
                            ImGui::Image((ImTextureRef)tilemap_texture[0],ImVec2(256.0f * scale,256.0f * scale));
                            
                            if(show_tile_grid) render_grid();
                            if(show_scroll_overlay) render_scroll_overlay();
                        }
                        ImGui::EndChild();
                        
                        ImGui::EndTabItem();
                    }
                    
                    if(ImGui::BeginTabItem("9C00")){
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::TableNextColumn();

                ImGui::Checkbox("Show Tile Grid",&show_tile_grid);
                ImGui::Checkbox("Shwo Scroll Overlay",&show_scroll_overlay);

                ImGui::EndTable();
            }

            if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && scale < max_scale){
                scale += 1.0f;
            }
            if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && scale > min_scale){
                scale -= 1.0f;
            }

            ImGuiIO& io = ImGui::GetIO();
            if(io.KeyCtrl){
                if(io.MouseWheel > 0.0f && scale < max_scale){
                    scale += 1.0f;
                }
                else if(io.MouseWheel < 0.0f && scale > min_scale){
                    scale -= 1.0f;
                }
            }

        }
        ImGui::End();
    }

private:
};

int main(int n_args,char** args){

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window = SDL_CreateWindow("NanoBoy",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640,480,SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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

    bool running = true;

    file_selector_t file_selector;
    tilemap_viewer_t tilemap_viewer(renderer);
    
    SDL_Event event;
    while(running){
        SDL_PollEvent(&event);
        ImGui_ImplSDL2_ProcessEvent(&event);

        if(event.type == SDL_QUIT) running = false;

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();

        ImGui::NewFrame();

        if(ImGui::BeginMainMenuBar()){
            if(ImGui::BeginMenu("File")){
                if(ImGui::MenuItem("Open File")){
                    file_selector.opened = true;
                }
                if(ImGui::MenuItem("Exit")){
                    running = false;
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Debug")){
                if(ImGui::MenuItem("Tilemap Viewer")){
                    tilemap_viewer.opened = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        file_selector.render();
        
        tilemap_viewer.render();

        ImGui::Render();

        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}