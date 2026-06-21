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
#include <thread>

float get_input_scalar_width(){
    ImGuiStyle& style = ImGui::GetStyle();
    return ImGui::CalcTextSize("0000").x + style.FramePadding.x * 2.0f + (ImGui::GetFrameHeight() + style.ItemInnerSpacing.x) * 2.0f;
}


bool mouse_in_rect(ImVec2 m,ImVec2 rmin,ImVec2 rmax){
    return (m.x >= rmin.x && m.x <= rmax.x) && (m.y >= rmin.y && m.y <= rmax.y);
}


struct bg_palette_t{
    uint8_t bgp = 0;
    gb_rgb_t colors[gb_cgb_colors] = {0};

    gb_rgb_t get_dmg_color(uint8_t index){
        return dmg_colors[(bgp >> ((index & 0x03) << 0x01)) & 0x03];
    }

    gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index){
        return colors[((palette_index & 0x07) << 0x02) | (color_index & 0x03)];
    }

    gb_rgb_t get_cgb_dmg_color(uint8_t index){
        return colors[(bgp >> ((index & 0x03) << 0x01)) & 0x03];
    }
};

struct obj_palette_t{
    uint8_t obp[2] = {0};
    gb_rgb_t colors[gb_cgb_colors] = {0};

    gb_rgb_t get_dmg_color(uint8_t obp_index,uint8_t index){
        return dmg_colors[(obp[obp_index & 0x01] >> ((index & 0x03) << 0x01)) & 0x03];
    }

    gb_rgb_t get_cgb_color(uint8_t palette_index,uint8_t color_index){
        return colors[((palette_index & 0x07) << 0x02) | (color_index & 0x03)];
    }

    gb_rgb_t get_cgb_dmg_color(uint8_t obp_index,uint8_t index){
        return colors[((obp_index & 0x01) << 0x02) | ((obp[obp_index & 0x01] >> ((index & 0x03) << 0x01)) & 0x03)];
    }
};


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


class tilemap_viewer_t {
private:
    enum{
        tile_size = 8,
        
        map_columns = 32,
        map_rows = 32,
        
        texture_width = map_columns * tile_size,
        texture_height = map_rows * tile_size,

        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_bytes_per_pixel = SDL_BYTESPERPIXEL(texture_format),
        texture_access = SDL_TEXTUREACCESS_STREAMING,
    };

    gb_t* gb = nullptr;

    SDL_Texture* tilemap_texture[2] = {nullptr};

    ImU32 grid_color = 0;
    ImU32 scroll_overlay_border_color = 0;
    ImU32 scroll_overlay_background_color = 0;

    bool show_tile_grid = false;
    bool show_scroll_overlay = false;

    const float min_scale = 1.0f;
    const float max_scale = 10.0f;
    float scale = min_scale;

    float input_scalar_width = 0.0f;
    int input_scalar_step = 1;
    int input_scalar_step_fast = 100;

    bool cgb_mode = false;
    bool tiledata_area = false;
    uint8_t scx = 0;
    uint8_t scy = 0;
    bg_palette_t bg_palette;
    uint8_t vram[gb_vram_length] = {0};

    gb_ppu_callback_handler_t callback_handler = {callback,this,gb_vblank_scanline,0,nullptr};

    bool open = false;


    static void callback(void* data){
        tilemap_viewer_t* tmv = (tilemap_viewer_t*)data;

        gb_t* gb = tmv->gb;

        tmv->cgb_mode = gb->cgb_mode;
        
        tmv->tiledata_area = gb->ppu.lcdc.tiledata_area;
        
        tmv->scx = gb->ppu.scx;
        tmv->scy = gb->ppu.scy;

        tmv->bg_palette.bgp = gb->palette.bgp;
        memcpy(tmv->bg_palette.colors,gb->palette.bg_cram_converted,sizeof(tmv->bg_palette.colors));

        memcpy(tmv->vram,gb->ppu.vram,sizeof(tmv->vram));
    }


    void update_tilemap_texture(uint8_t map_index){
        uint16_t base_address = (map_index == 0x01) ? 0x1C00 : 0x1800;
        uint8_t* map = vram + base_address;
        uint8_t* map_attribute = vram + (0x2000 | base_address);

        gb_rgb_t color = {0};

        uint8_t* pixels = nullptr;
        int pitch = 0;
        SDL_LockTexture(tilemap_texture[map_index],nullptr,(void**)&pixels,&pitch);

        for(int row = 0; row < map_rows; ++row){
            for(int col = 0; col < map_columns; ++col){
                
                uint16_t index = (row << 0x05) | col;
                
                uint8_t tile_index = map[index];

                uint8_t attribute = cgb_mode ? map_attribute[index] : 0x00;

                uint16_t tile_address = tiledata_area ? tile_index << 0x04 : 0x1000 + ((int8_t)tile_index << 0x04);

                uint8_t* tile_data = vram + (((attribute & 0x08) ? 0x2000 : 0x0000) | tile_address);

                for(int y = 0; y < 8; ++y){
                    
                    uint8_t tile_byte_address = ((attribute & 0x40) ? 0x07 ^ y : y) << 0x01;

                    uint8_t lo = tile_data[tile_byte_address + 0x00];
                    uint8_t hi = tile_data[tile_byte_address + 0x01];
                    
                    for(int x = 0; x < 8; ++x){
                        uint8_t bit = 0x01 << ((attribute & 0x20) ? x : 0x07 ^ x);
                        uint8_t index = ((hi & bit) ? 0x02 : 0x00) | ((lo & bit) ? 0x01 : 0x00);

                        if(gb->type == gb_cgb){
                            if(cgb_mode){
                                color = bg_palette.get_cgb_color(attribute & 0x07,index);
                            }
                            else{
                                color = bg_palette.get_cgb_dmg_color(index);
                            }
                        }
                        else{
                            color = bg_palette.get_dmg_color(index);
                        }

                        uint8_t* pixel = pixels + (((row << 0x03) | y) * pitch) + (((col << 0x03) | x) * texture_bytes_per_pixel);
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
        
        int scx_right = scx + 159;
        int scy_bottom = scy + 143;
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
            max = ImVec2(start.x + (scx_right - 255) * scale,min.y + 144 * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);

            min = ImVec2(start.x + scx * scale,start.y + scy * scale);
            max = ImVec2(end.x,min.y + 144 * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);
        }
        else if(scy_bottom > 255){
            min = ImVec2(start.x + scx * scale,start.y);
            max = ImVec2(min.x + 160 * scale,start.y + (scy_bottom - 255) * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);

            min = ImVec2(start.x + scx * scale,start.y + scy * scale);
            max = ImVec2(min.x + 160 * scale,end.y);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);
        }
        else{
            min = ImVec2(start.x + scx * scale,start.y + scy * scale);
            max = ImVec2(min.x + 160 * scale,min.y + 144 * scale);
            draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
            draw_list->AddRect(min,max,scroll_overlay_border_color);
        }

    }

public:

    tilemap_viewer_t(gb_t* gb,SDL_Renderer* renderer):gb(gb){
        
        tilemap_texture[0] = SDL_CreateTexture(renderer,texture_format,texture_access,texture_width,texture_height);
        tilemap_texture[1] = SDL_CreateTexture(renderer,texture_format,texture_access,texture_width,texture_height);

        grid_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,0.5f));
        scroll_overlay_border_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,1.0f));
        scroll_overlay_background_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,0.2f));

        input_scalar_width = get_input_scalar_width();
    }

    ~tilemap_viewer_t(){
        gb_remove_ppu_callback(gb,&callback_handler);

        SDL_DestroyTexture(tilemap_texture[0]);
        SDL_DestroyTexture(tilemap_texture[1]);
    }

    void render(){
        
        if(!open) return;

        bool _open = open;

        if(ImGui::Begin("Tilemap Viewer",&_open)){

            if(ImGui::BeginTable("LayoutTable",2)){

                ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();

                if(ImGui::BeginTabBar("AddressPointer")){
                    
                    if(ImGui::BeginTabItem("9800")){
                        
                        if(ImGui::BeginChild("Tilemap0",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
                            
                            update_tilemap_texture(0);

                            ImGui::Image((ImTextureRef)tilemap_texture[0],ImVec2(256.0f * scale,256.0f * scale));
                            
                            if(show_tile_grid) render_grid();
                            if(show_scroll_overlay) render_scroll_overlay();
                        }
                        ImGui::EndChild();
                        
                        ImGui::EndTabItem();
                    }
                    
                    if(ImGui::BeginTabItem("9C00")){

                        if(ImGui::BeginChild("Tilemap1",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
                            
                            update_tilemap_texture(1);

                            ImGui::Image((ImTextureRef)tilemap_texture[1],ImVec2(256.0f * scale,256.0f * scale));
                            
                            if(show_tile_grid) render_grid();
                            if(show_scroll_overlay) render_scroll_overlay();
                        }
                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::TableNextColumn();

                ImGui::Checkbox("Show Tile Grid",&show_tile_grid);

                ImGui::Checkbox("Shwo Scroll Overlay",&show_scroll_overlay);

                ImGui::SetNextItemWidth(input_scalar_width);
                if(ImGui::InputScalar("Refresh on scanline",ImGuiDataType_U8,&callback_handler.scanline,&input_scalar_step,&input_scalar_step_fast)){
                    if(callback_handler.scanline >= gb_scanlines){
                        callback_handler.scanline = gb_scanlines - 1;
                    }
                }

                ImGui::SetNextItemWidth(input_scalar_width);
                if(ImGui::InputScalar("Refresh on cycle",ImGuiDataType_U16,&callback_handler.cycle,&input_scalar_step,&input_scalar_step_fast)){
                    if(callback_handler.cycle >= gb_scanline_cycles){
                        callback_handler.cycle = gb_scanline_cycles - 1;
                    }
                }

                ImGui::EndTable();
            }

            if(ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){

                if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && scale < max_scale){
                    ++scale;
                }
                else if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && scale > min_scale){
                    --scale;
                }

                ImGuiIO& io = ImGui::GetIO();
                if(io.KeyCtrl){
                    if(io.MouseWheel > 0.0f && scale < max_scale){
                        ++scale;
                    }
                    else if(io.MouseWheel < 0.0f && scale > min_scale){
                        --scale;
                    }
                }
            }
        }
        ImGui::End();

        set_open(_open);
    }

    void set_open(bool _open){
        if(open == _open) return;
        open = _open;
        if(open){
            gb_add_ppu_callback(gb,&callback_handler);
        }
        else{
            gb_remove_ppu_callback(gb,&callback_handler);
        }
    }

    bool get_open() const {
        return open;
    }
};

class object_viewer_t {
private:
    enum{
        bg_texture_width = 256,
        bg_texture_height = 256,

        bg_texture_format = SDL_PIXELFORMAT_RGB24,

        palette_texture_width = gb_palette_colors * gb_tile_size,
        palette_texture_height = gb_tile_size,

        palette_texture_format = SDL_PIXELFORMAT_RGB24,

        obj_texture_format = SDL_PIXELFORMAT_RGBA32,

        texture_access = SDL_TEXTUREACCESS_STREAMING,

        on_screen_offset_x = 8,
        on_screen_offset_y = 16,

        oam_table_rows = 5,
        oam_table_columns = gb_oam_objects / oam_table_rows,
        oam_table_object_scale = 3,

        tooltip_object_scale = 8,
        tooltip_object_palette_scale = 2,
    };

    struct object_t {
        SDL_Texture* texture = nullptr;
        SDL_Texture* palette_texture = nullptr;
        uint8_t index = 0;
        uint8_t y = 0;
        uint8_t x = 0;
        uint8_t tile_index = 0;
        uint16_t tile_address = 0;
        uint8_t palette_index = 0;
        bool horizontal_flip = false;
        bool vertical_flip = false;
        bool priority = false;

        object_t(uint8_t _index,SDL_Renderer* renderer){
            texture = SDL_CreateTexture(renderer,obj_texture_format,texture_access,gb_object_width,gb_object_max_height);
            palette_texture = SDL_CreateTexture(renderer,palette_texture_format,texture_access,palette_texture_width,palette_texture_height);
            SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
            index = _index;
        }

        object_t(object_t&& v) noexcept {
            memcpy(this,&v,sizeof(object_t));
            v.texture = nullptr;
            v.palette_texture = nullptr;
        }

        object_t(const object_t& v) = delete;

        ~object_t(){
            SDL_DestroyTexture(texture);
            SDL_DestroyTexture(palette_texture);
        }
    };

    gb_t* gb = nullptr;

    SDL_Texture* bg_texture = nullptr;

    float min_scale = 1.0f;
    float max_scale = 10.0f;
    float scale = min_scale;

    bool show_offscreen = true;
    bool show_outline = true;

    float input_scalar_width = 0.0f;
    int input_scalar_step = 1;
    int input_scalar_step_fast = 100;

    ImVec2 oam_table_size;

    uint32_t outline_color;
    uint32_t outline_hovered_color;
    uint32_t obj_bg_color;
    uint32_t border_color;
    uint32_t border_hovered_color;

    ImVec2 bg_size;
    ImVec2 bg_uv0;
    ImVec2 bg_uv1;
    ImVec2 bg_offset;

    bool cgb_mode = false;
    bool obj_priority_mode = false;
    bool object_size = false;
    ImVec2 object_uv0{0.0f,0.0f};
    ImVec2 object_uv1{1.0f,1.0f};
    obj_palette_t obj_palette;
    uint8_t oam[gb_oam_length] = {0};
    uint8_t vram[gb_vram_length] = {0};

    std::vector<object_t> objects;
    std::vector<object_t*> objects_sorted;
    object_t* oam_table_object_hovered = nullptr;

    gb_ppu_callback_handler_t callback_handler = {callback,this,gb_vblank_scanline,0,nullptr};

    bool open = false;


    static void callback(void* data){
        object_viewer_t* object_viewer = (object_viewer_t*)data;
        gb_t* gb = object_viewer->gb;
        obj_palette_t& obj_palette = object_viewer->obj_palette;

        object_viewer->cgb_mode = gb->cgb_mode;
        object_viewer->obj_priority_mode = gb->obj_priority_mode;

        object_viewer->object_size = gb->ppu.lcdc.object_size;
        
        object_viewer->object_uv0.x = 0.0f;
        object_viewer->object_uv0.y = 0.0f;

        object_viewer->object_uv1.x = 1.0f;
        object_viewer->object_uv1.y = object_viewer->object_size ? 1.0f : 0.5f;

        obj_palette.obp[0] = gb->palette.obp[0];
        obj_palette.obp[1] = gb->palette.obp[1];

        memcpy(obj_palette.colors,gb->palette.obj_cram_converted,sizeof(obj_palette.colors));
        
        memcpy(object_viewer->oam,gb->ppu.oam,sizeof(object_viewer->oam));
        
        memcpy(object_viewer->vram,gb->ppu.vram,sizeof(object_viewer->vram));
    }


    void render_bg(){
        uint8_t* pixels = nullptr;
        int pitch = 0;

        SDL_LockTexture(bg_texture,nullptr,(void**)&pixels,&pitch);

        memset(pixels,0x46,pitch * bg_texture_height);

        gb_rgb_t color[2] = {
            {0,0,0},
            {255,255,255}
        };
        bool color_index = 0;

        uint8_t* on_screen_pixels = pixels + (on_screen_offset_y * pitch) + (on_screen_offset_x * 3);

        for(int row = 0, tile_y = 0; row < gb_screen_rows; ++row, tile_y += gb_tile_size){
            
            for(int col = 0, tile_x = 0; col < gb_screen_columns; ++col, tile_x += gb_tile_size){

                for(int y = 0; y < 8; ++y){

                    uint8_t* line_pixel = on_screen_pixels + (tile_y | y) * pitch + tile_x * 3;
                    
                    for(int x = 0; x < 8; ++x){

                        line_pixel[0] = color[color_index].r;
                        line_pixel[1] = color[color_index].g;
                        line_pixel[2] = color[color_index].b;

                        line_pixel += 3;
                    }
                }

                color_index = !color_index;
            }

            color_index = !color_index;
        }

        SDL_UnlockTexture(bg_texture);
    }


    void update_bg_metrics(){

        if(show_offscreen){

            bg_size = ImVec2(bg_texture_width * scale,bg_texture_height * scale);

            bg_uv0 = ImVec2(0.0f,0.0f);
            bg_uv1 = ImVec2(1.0f,1.0f);

            bg_offset = ImVec2(0.0f,0.0f); 
        }
        else{
            bg_size = ImVec2(gb_screen_width * scale,gb_screen_height * scale);

            bg_uv0 = ImVec2(
                (float)on_screen_offset_x / (float)bg_texture_width,
                (float)on_screen_offset_y / (float)bg_texture_height
            );
            
            bg_uv1 = ImVec2(
                bg_uv0.x + ((float)gb_screen_width / (float)bg_texture_width),
                bg_uv0.y + ((float)gb_screen_height / (float)bg_texture_height)
            );

            bg_offset = ImVec2(-(float)on_screen_offset_x,-(float)on_screen_offset_y);
        }
    }

    void update_object_texture(object_t& object){
        uint8_t* pixels = nullptr;
        int pitch = 0;
        gb_rgb_t color = {0};

        uint8_t object_height = object_size ? gb_object_max_height : gb_object_min_height;

        SDL_LockTexture(object.texture,nullptr,(void**)&pixels,&pitch);

        for(uint8_t y = 0; y < object_height; ++y){

            uint16_t address = object.tile_address | ((object.vertical_flip ? (object_size ? 0x0F : 0x07) ^ y : y) << 0x01);

            uint8_t lo = vram[address + 0x00];
            uint8_t hi = vram[address + 0x01];

            for(uint8_t x = 0; x < 8; ++ x){

                uint8_t bit = 0x01 << (object.horizontal_flip ? x : 0x07 ^ x);

                uint8_t color_index = ((hi & bit) ? 0x02 : 0x00) | ((lo & bit) ? 0x01 : 0x00);

                if(gb->type == gb_cgb){
                    if(cgb_mode){
                        color = obj_palette.get_cgb_color(object.palette_index,color_index);
                    }
                    else{
                        color = obj_palette.get_cgb_dmg_color(object.palette_index,color_index);
                    }
                }
                else{
                    color = obj_palette.get_dmg_color(object.palette_index,color_index);
                }

                uint8_t* pixel = pixels + y * pitch + x * 4;
                pixel[0] = color.r;
                pixel[1] = color.g;
                pixel[2] = color.b;
                pixel[3] = (color_index != 0) ? 255 : 0;
            }
        }

        SDL_UnlockTexture(object.texture);
    }

    void update_object_palette_texture(object_t& object){
        uint8_t* pixels = nullptr;
        int pitch = 0;
        gb_rgb_t color = {0};

        SDL_LockTexture(object.palette_texture,nullptr,(void**)&pixels,&pitch);

        for(int col = 0; col < gb_palette_colors; ++col){

            if(gb->type == gb_cgb){
                if(cgb_mode){
                    color = obj_palette.get_cgb_color(object.palette_index,col);
                }
                else{
                    color = obj_palette.get_cgb_dmg_color(object.palette_index,col);
                }
            }
            else{
                color = obj_palette.get_dmg_color(object.palette_index,col);
            }

            for(int y = 0; y < 8; ++y){
                for(int x = 0; x < 8; ++x){
                    uint8_t* pixel = pixels + y * pitch + ((col << 0x03) | x) * 3;
                    pixel[0] = color.r;
                    pixel[1] = color.g;
                    pixel[2] = color.b;
                }
            }

        }

        SDL_UnlockTexture(object.palette_texture);
    }

    void update_objects(){
        gb_object_t* oam_entry = (gb_object_t*)oam;

        for(auto& object : objects){

            object.y = oam_entry->y;
            object.x = oam_entry->x;
            object.tile_index = oam_entry->tile_index;

            object.tile_address = (cgb_mode && (oam_entry->attributes & 0x08)) ? 0x2000 : 0x0000;
            object.tile_address |= (object.tile_index & (object_size ? 0xFE : 0xFF)) << 0x04;

            object.palette_index = cgb_mode ? oam_entry->attributes & 0x07 : (oam_entry->attributes & 0x10) ? 0x01 : 0x00;

            object.horizontal_flip = oam_entry->attributes & 0x20;
            object.vertical_flip = oam_entry->attributes & 0x40;

            object.priority = oam_entry->attributes & 0x80;

            update_object_texture(object);

            update_object_palette_texture(object);

            ++oam_entry;
        }

        //DMG priority mode
        if(obj_priority_mode){
            std::sort(objects_sorted.begin(),objects_sorted.end(),[](object_t* a,object_t* b)->bool{
                if(a->x != b->x){
                    return a->x > b->x;
                }
                return a->index > b->index;
            });
        }
        //CGB priority mode
        else{
            std::sort(objects_sorted.begin(),objects_sorted.end(),[](object_t* a,object_t* b)->bool{
                return a->index > b->index;
            });
        }
    }


    void render_object_tooltip(object_t* object){

        if(!ImGui::BeginTooltip()) return;

        if(ImGui::BeginTable("ObjectTooltip",2)){

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            //Object
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Object");
            ImGui::TableNextColumn();

            int object_height = object_size ? gb_object_max_height : gb_object_min_height;

            ImVec2 tooltip_object_size(
                (float)gb_object_width * (float)tooltip_object_scale,
                (float)object_height * (float)tooltip_object_scale
            );

            ImVec2 p_min = ImGui::GetCursorScreenPos();
            ImVec2 p_max = ImVec2(p_min.x + tooltip_object_size.x,p_min.y + tooltip_object_size.y);

            draw_list->AddRectFilled(p_min,p_max,obj_bg_color);

            ImGui::Image((ImTextureRef)object->texture,tooltip_object_size,object_uv0,object_uv1);

            draw_list->AddRect(p_min,p_max,border_color);

            //Palette
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Palette");
            ImGui::TableNextColumn();

            ImVec2 tooltip_object_palette_size(
                (float)palette_texture_width * (float)tooltip_object_palette_scale,
                (float)palette_texture_height * (float)tooltip_object_palette_scale 
            );

            ImGui::Image((ImTextureRef)object->palette_texture,tooltip_object_palette_size);

            draw_list->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),border_color);

            //Index
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Index");
            ImGui::TableNextColumn();
            ImGui::Text("%d",object->index);

            //X, Y
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("X, Y");
            ImGui::TableNextColumn();
            ImGui::Text("%d, %d",object->x,object->y);

            //Size
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Size");
            ImGui::TableNextColumn();
            ImGui::Text("%dx%d",gb_object_width,object_height);

            //Tile index
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Tile index");
            ImGui::TableNextColumn();
            ImGui::Text("%02X",object->tile_index);

            //Tile address
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Tile address");
            ImGui::TableNextColumn();
            ImGui::Text("%04X",object->tile_address);

            //Palette index
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Palette index");
            ImGui::TableNextColumn();
            ImGui::Text("%d",object->palette_index);

            //Horizontal flip
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Horizontal flip");
            ImGui::TableNextColumn();
            (object->horizontal_flip ? ImGui::TextUnformatted("True") : ImGui::TextUnformatted("False"));

            //Vertical flip
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Vertical flip");
            ImGui::TableNextColumn();
            (object->vertical_flip ? ImGui::TextUnformatted("True") : ImGui::TextUnformatted("False"));

            //Priority
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Priority");
            ImGui::TableNextColumn();
            (object->priority ? ImGui::TextUnformatted("True") : ImGui::TextUnformatted("False"));

            ImGui::EndTable();
        }

        ImGui::EndTooltip();
    }

    void render_oam_table(){

        if(!ImGui::BeginTable("OAMTable",oam_table_columns,ImGuiTableFlags_None,oam_table_size)) return;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        auto object = objects.begin();

        int object_height = object_size ? gb_object_max_height : gb_object_min_height;

        ImVec2 table_object_size(
            (float)gb_object_width * (float)oam_table_object_scale,
            (float)object_height * (float)oam_table_object_scale
        );

        oam_table_object_hovered = nullptr;

        for(int y = 0; y < oam_table_rows; ++y){
            
            ImGui::TableNextRow();
            
            for(int x = 0; x < oam_table_columns; ++x){

                ImGui::TableNextColumn();

                ImVec2 p_min = ImGui::GetCursorScreenPos();
                ImVec2 p_max(p_min.x + table_object_size.x,p_min.y + table_object_size.y);

                draw_list->AddRectFilled(p_min,p_max,obj_bg_color);

                ImGui::Image((ImTextureRef)object->texture,table_object_size,object_uv0,object_uv1);

                if(ImGui::IsItemHovered()){

                    oam_table_object_hovered = object.base();

                    draw_list->AddRect(p_min,p_max,border_hovered_color,0.0f,ImDrawFlags_None,2.0f);

                    render_object_tooltip(object.base());
                }
                else{
                    draw_list->AddRect(p_min,p_max,border_color);
                }

                ++object;
            }
        }

        ImGui::EndTable();
    }

    void render_oam_screen(){
        if(ImGui::BeginChild("OAMScreen",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
            
            ImGui::Image((ImTextureRef)bg_texture,bg_size,bg_uv0,bg_uv1);

            bool image_hovered = ImGui::IsItemHovered();
            ImVec2 mouse_pos = ImGui::GetMousePos();

            ImVec2 start = ImGui::GetItemRectMin();
            ImVec2 end = ImGui::GetItemRectMax();

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            draw_list->PushClipRect(start,end,true);

            float object_height = object_size ? gb_object_max_height : gb_object_min_height;

            object_t* oam_screen_object_hovered = nullptr;

            for(auto object : objects_sorted){

                if(object == oam_table_object_hovered) continue;

                ImVec2 rmin (
                    start.x + (object->x + bg_offset.x) * scale,
                    start.y + (object->y + bg_offset.y) * scale
                );
                
                ImVec2 rmax (
                    rmin.x + gb_object_width * scale,
                    rmin.y + object_height * scale
                );

                draw_list->AddImage((ImTextureRef)object->texture,rmin,rmax,object_uv0,object_uv1);

                if(image_hovered && mouse_in_rect(mouse_pos,rmin,rmax)){
                    oam_screen_object_hovered = object;
                }
                
                if(show_outline){
                    draw_list->AddRect(rmin,rmax,outline_color);
                }
            }

            if(oam_screen_object_hovered != nullptr || oam_table_object_hovered != nullptr){

                object_t* object_hovered = oam_table_object_hovered ? oam_table_object_hovered : oam_screen_object_hovered;

                ImVec2 rmin (
                    start.x + (object_hovered->x + bg_offset.x) * scale,
                    start.y + (object_hovered->y + bg_offset.y) * scale
                );
                
                ImVec2 rmax (
                    rmin.x + gb_object_width * scale,
                    rmin.y + object_height * scale
                );

                if(object_hovered == oam_table_object_hovered){
                    draw_list->AddImage((ImTextureRef)object_hovered->texture,rmin,rmax,object_uv0,object_uv1);
                }

                draw_list->AddRect(rmin,rmax,outline_hovered_color,0.0f,ImDrawFlags_None,2.0f);

                if(object_hovered == oam_screen_object_hovered){
                    render_object_tooltip(object_hovered);
                }
            }

            draw_list->PopClipRect();
        }

        ImGui::EndChild();
    }

public:

    object_viewer_t(gb_t* gb,SDL_Renderer *renderer):gb(gb){
        
        bg_texture = SDL_CreateTexture(renderer,bg_texture_format,texture_access,bg_texture_width,bg_texture_height);

        render_bg();

        input_scalar_width = get_input_scalar_width();

        ImGuiStyle& style = ImGui::GetStyle();

        oam_table_size = ImVec2(
            ((gb_object_width * oam_table_object_scale) + (style.CellPadding.x * 2.0f)) * oam_table_columns,
            0.0f
        );

        outline_color = IM_COL32(120,120,120,255);
        outline_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);

        obj_bg_color = IM_COL32(128,128,128,255);

        border_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]);
        border_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);

        update_bg_metrics();

        objects.reserve(gb_oam_objects);
        objects_sorted.reserve(gb_oam_objects);

        for(int i = 0; i < gb_oam_objects; ++i){
            objects_sorted.push_back(&objects.emplace_back(i,renderer));
        }
    }

    ~object_viewer_t(){
        gb_remove_ppu_callback(gb,&callback_handler);
        
        SDL_DestroyTexture(bg_texture);
    }

    void render(){
        if(!open) return;

        bool _open = open;

        if(ImGui::Begin("Object Viewer",&_open)){

            update_objects();

            if(ImGui::BeginTable("ObjectTable1",2,ImGuiTableFlags_None)){

                ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                render_oam_screen();

                ImGui::TableNextColumn();

                render_oam_table();

                if(ImGui::Checkbox("Show offscreen",&show_offscreen)){
                    update_bg_metrics();
                }

                ImGui::Checkbox("Show outline",&show_outline);

                ImGui::SetNextItemWidth(input_scalar_width);
                if(ImGui::InputScalar("Refresh on scanline",ImGuiDataType_U8,&callback_handler.scanline,&input_scalar_step,&input_scalar_step_fast)){
                    if(callback_handler.scanline >= gb_scanlines){
                        callback_handler.scanline = gb_scanlines - 1;
                    }
                }

                ImGui::SetNextItemWidth(input_scalar_width);
                if(ImGui::InputScalar("Refresh on cycle",ImGuiDataType_U16,&callback_handler.cycle,&input_scalar_step,&input_scalar_step_fast)){
                    if(callback_handler.cycle >= gb_scanline_cycles){
                        callback_handler.cycle = gb_scanline_cycles - 1;
                    }
                }

                ImGui::EndTable();
            }

            if(ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){

                if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && scale < max_scale){
                    ++scale;
                    update_bg_metrics();
                }
                else if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && scale > min_scale){
                    --scale;
                    update_bg_metrics();
                }

                ImGuiIO& io = ImGui::GetIO();
                if(io.KeyCtrl){
                    if(io.MouseWheel > 0.0f && scale < max_scale){
                        ++scale;
                        update_bg_metrics();
                    }
                    else if(io.MouseWheel < 0.0f && scale > min_scale){
                        --scale;
                        update_bg_metrics();
                    }
                }
            }
            
        }
        ImGui::End();

        set_open(_open);
    }

    void set_open(bool _open){
        if(open == _open) return;
        open = _open;
        if(open){
            gb_add_ppu_callback(gb,&callback_handler);
        }
        else{
            gb_remove_ppu_callback(gb,&callback_handler);
        }
    }

    bool get_open() const {
        return open;
    }
};

class palette_viewer_t {
private:
    enum{
        texture_max_width = gb_palette_colors * gb_tile_size,
        texture_max_height = gb_cgb_palettes * gb_tile_size,
        
        cgb_texture_height = texture_max_height,

        dmg_bg_texture_height = gb_dmg_bg_palettes * gb_tile_size,

        dmg_obj_texture_height = gb_dmg_obj_palettes * gb_tile_size,
        
        texture_format = SDL_PIXELFORMAT_RGB24,
        texture_bytes_per_pixel = SDL_BYTESPERPIXEL(texture_format),
        texture_access = SDL_TEXTUREACCESS_STREAMING
    };

    gb_t* gb = nullptr;

    bool cgb_mode = false;
    bg_palette_t bg_palette;
    obj_palette_t obj_palette;

    SDL_Texture* bg_texture = nullptr;
    SDL_Texture* obj_texture = nullptr;

    float input_scalar_width = 0.0f;
    int input_scalar_step = 1;
    int input_scalar_step_fast = 100;

    gb_ppu_callback_handler_t callback_handler = {callback,this,gb_vblank_scanline,0,nullptr};

    bool open = false;


    static void callback(void* data){
        palette_viewer_t* pv = (palette_viewer_t*)data;
        gb_t* gb = pv->gb;

        bg_palette_t& bg_palette = pv->bg_palette;
        obj_palette_t& obj_palette = pv->obj_palette;

        pv->cgb_mode = pv->gb->cgb_mode;
        
        bg_palette.bgp = gb->palette.bgp;
        
        obj_palette.obp[0] = gb->palette.obp[0];
        obj_palette.obp[1] = gb->palette.obp[1];

        memcpy(bg_palette.colors,gb->palette.bg_cram_converted,sizeof(bg_palette.colors));
        memcpy(obj_palette.colors,gb->palette.obj_cram_converted,sizeof(obj_palette.colors));
    }


    void update_bg_texture(){
        int rows = cgb_mode ? gb_cgb_palettes : gb_dmg_bg_palettes;

        gb_rgb_t color = {0};

        uint8_t* pixels = nullptr;
        int pitch = 0;
        SDL_LockTexture(bg_texture,nullptr,(void**)&pixels,&pitch);

        for(int row = 0; row < rows; ++row){
            for(int col = 0; col < gb_palette_colors; ++col){

                if(gb->type == gb_cgb){
                    if(cgb_mode){
                        color = bg_palette.get_cgb_color(row,col);
                    }
                    else{
                        color = bg_palette.get_cgb_dmg_color(col);
                    }
                }
                else{
                    color = bg_palette.get_dmg_color(col);
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

        SDL_UnlockTexture(bg_texture);
    }

    void update_obj_texture(){
        int rows = cgb_mode ? gb_cgb_palettes : gb_dmg_obj_palettes;

        gb_rgb_t color = {0};

        uint8_t* pixels = nullptr;
        int pitch = 0;
        SDL_LockTexture(obj_texture,nullptr,(void**)&pixels,&pitch);

        for(int row = 0; row < rows; ++row){
            for(int col = 0; col < gb_palette_colors; ++col){

                if(gb->type == gb_cgb){
                    if(cgb_mode){
                        color = obj_palette.get_cgb_color(row,col);
                    }
                    else{
                        color = obj_palette.get_cgb_dmg_color(row,col);
                    }
                }
                else{
                    color = obj_palette.get_dmg_color(row,col);
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

        SDL_UnlockTexture(obj_texture);
    }

public:
    palette_viewer_t(gb_t* gb,SDL_Renderer* renderer):gb(gb){
        bg_texture = SDL_CreateTexture(renderer,texture_format,texture_access,texture_max_width,texture_max_height);
        obj_texture = SDL_CreateTexture(renderer,texture_format,texture_access,texture_max_width,texture_max_height);

        ImGuiStyle& style = ImGui::GetStyle();

        input_scalar_width = get_input_scalar_width();
    }

    ~palette_viewer_t(){
        gb_remove_ppu_callback(gb,&callback_handler);

        SDL_DestroyTexture(bg_texture);
        SDL_DestroyTexture(obj_texture);
    }

    void render(){
        if(!open) return;

        bool _open = open;

        if(ImGui::Begin("Palette Viewer",&_open)){

            if(ImGui::BeginTable("PaletteTable",3)){

                ImGui::TableSetupColumn("BackgroundColumn",ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("ObjectColumn",ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("ControlColumn",ImGuiTableColumnFlags_WidthFixed);

                ImVec2 bg_size = ImVec2(texture_max_width,cgb_mode ? cgb_texture_height : dmg_bg_texture_height);
                ImVec2 bg_uv0 = {0.0f,0.0f};
                ImVec2 bg_uv1 = {1.0f,bg_size.y / texture_max_height};
                bg_size.x *= 4.0f;
                bg_size.y *= 4.0f;

                update_bg_texture();

                ImVec2 obj_size = ImVec2(texture_max_width,cgb_mode ? cgb_texture_height : dmg_obj_texture_height);
                ImVec2 obj_uv0 = {0.0f,0.0f};
                ImVec2 obj_uv1 = {1.0f,obj_size.y / texture_max_height};
                obj_size.x *= 4.0f;
                obj_size.y *= 4.0f;

                update_obj_texture();

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Background");
                ImGui::Image((ImTextureRef)bg_texture,bg_size,bg_uv0,bg_uv1);
                
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Object");
                ImGui::Image((ImTextureRef)obj_texture,obj_size,obj_uv0,obj_uv1);

                ImGui::TableNextColumn();

                ImGui::SetNextItemWidth(input_scalar_width);
                if(ImGui::InputScalar("Refresh on scanline",ImGuiDataType_U8,&callback_handler.scanline,&input_scalar_step,&input_scalar_step_fast)){
                    if(callback_handler.scanline >= gb_scanlines){
                        callback_handler.scanline = gb_scanlines - 1;
                    }
                }

                ImGui::SetNextItemWidth(input_scalar_width);
                if(ImGui::InputScalar("Refresh on cycle",ImGuiDataType_U16,&callback_handler.cycle,&input_scalar_step,&input_scalar_step_fast)){
                    if(callback_handler.cycle >= gb_scanline_cycles){
                        callback_handler.cycle = gb_scanline_cycles - 1;
                    }
                }

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
            gb_add_ppu_callback(gb,&callback_handler);
        }
        else{
            gb_remove_ppu_callback(gb,&callback_handler);
        }
    }

    bool get_open() const {
        return open;
    }
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
    
    bool mode = floating_mode;
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