#include "file_selector.hpp"
#include "../nanoboy.hpp"

const char* filters_name[2] = {
    "All files",
    "GB ROM files"
};

file_selector_t::file_selector_t(nanoboy_t* nanoboy):nanoboy(nanoboy){
    
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


void file_selector_t::set_current_path(std::filesystem::path path){
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

uintmax_t file_selector_t::number_of_entries_in_directory(std::filesystem::path directory){
    uintmax_t n = 0;
    for(auto& entry : std::filesystem::directory_iterator(directory,std::filesystem::directory_options::skip_permission_denied)){
        n++;
    }
    return n;
}

std::time_t file_selector_t::entry_last_write_time(std::filesystem::path entry){

    std::chrono::time_point sys_time_point = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        std::filesystem::last_write_time(entry) - 
        std::filesystem::file_time_type::clock::now() + 
        std::chrono::system_clock::now()
    );

    return std::chrono::system_clock::to_time_t(sys_time_point);
}

void file_selector_t::sort_current_directory_entries(){
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

void file_selector_t::load_current_directory_entries(){

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

const char* file_selector_t::entry_date_formated(const directory_entry_t& entry){
    static char buffer[32] = {0};

    tm* local_timer = std::localtime(&entry.last_write_time);
    
    std::strftime(buffer,sizeof(buffer),"%d/%m/%Y %H:%M",local_timer);

    return buffer;
}


void file_selector_t::render_directory(){

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
                    nanoboy->insert_cartridge(entry.path.string());
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

void file_selector_t::render(){
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
        ImGui::TextUnformatted("Name:");
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
                    nanoboy->insert_cartridge(path.string());
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