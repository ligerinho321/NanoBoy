#include <gui/file_dialog/file_selector_dialog.hpp>

bool file_selector_t::extension_is_valid(std::string extension) const noexcept {
    
    if(!filters || !filters_count || !extension.length()){
        return false;
    }

    bool result = false;

    const char* first_ptr = filters[current_filter];
    size_t first_length = strlen(first_ptr);

    const char* second_ptr = first_ptr + first_length + 1;
    size_t second_length = strlen(second_ptr);

    const char* token = nullptr;
    size_t token_length = 0;

    bool start = true;

    while(true){

        if(!second_length) break;

        if(start){
            if(*second_ptr == '.'){
                start = false;
                token = second_ptr;
                token_length = 1;
            }
            else{
                break;
            }
        }
        else{
            if(*second_ptr == ';' || second_length == 1){
                
                if(second_length == 1){
                    ++token_length;
                }

                if(
                    (token_length == 2 && token[1] == '*') ||
                    (token_length == extension.length() && !strncasecmp(token,extension.c_str(),token_length))
                ){
                    result = true;
                    break;
                }

                start = true;
            }
            else{
                ++token_length;
            }
        }

        ++second_ptr;
        --second_length;
    }

    return result;
}


void file_selector_t::load_current_directory_entries(){

    current_directory_entries.clear();

    std::error_code error;

    for(auto& entry : std::filesystem::directory_iterator(current_path,std::filesystem::directory_options::skip_permission_denied,error)){
        
        std::filesystem::path entry_path = entry.path();
        
        if(std::filesystem::is_directory(entry_path)){

            current_directory_entries.emplace_back(
                "[DIR] " + entry_path.filename().u8string(),
                entry_path,
                number_of_entries_in_directory(entry_path),
                get_entry_last_write_time(entry_path)
            );

        }
        else if(std::filesystem::is_regular_file(entry_path) && extension_is_valid(entry_path.extension().u8string())){

            current_directory_entries.emplace_back(
                "[FILE] " + entry_path.filename().u8string(),
                entry_path,
                std::filesystem::file_size(entry_path),
                get_entry_last_write_time(entry_path)
            );
        }
    }

    last_update = std::chrono::steady_clock::now();
}


void file_selector_t::send_name_buffer(){
    std::filesystem::path path = get_name_buffer_formated();
    
    if(std::filesystem::exists(path)){
        if(std::filesystem::is_directory(path)){
            set_current_path(path);
            load_current_directory_entries();
            sort_current_directory_entries();
        }
        else if(std::filesystem::is_regular_file(path)){
            if(callback != nullptr){
                callback(userdata,path);
            }
            open = false;
        }
    }

    clear_name_buffer();
}


void file_selector_t::render_directory(){

    directory_entry_t* selected_directory = nullptr;

    ImGuiListClipper clipper;
    clipper.Begin(current_directory_entries.size(),ImGui::GetTextLineHeightWithSpacing());

    while(clipper.Step()){

        for(int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row){
            
            directory_entry_t& entry = current_directory_entries[row];

            if(std::filesystem::is_directory(entry.path)){

                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                if(ImGui::Selectable(entry.name.c_str())){
                    selected_directory = &entry;
                }

                ImGui::TableNextColumn();
                ImGui::Text("%lu itens",entry.size);

                ImGui::TableNextColumn();
                ImGui::Text("%s",get_entry_date_formated(entry));
            }
            else{
                
                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                if(ImGui::Selectable(entry.name.c_str())){
                    if(callback != nullptr){
                        callback(userdata,entry.path);
                    }
                    open = false;
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

                ImGui::Text("%s",get_entry_date_formated(entry));
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
    if(!open) return;

    ImGui::SetNextWindowSizeConstraints(window_min,window_max);

    if(ImGui::Begin("Select File",&open)){

        ImGuiStyle& style = ImGui::GetStyle();

        ImVec2 browser_table_size = ImVec2(
            0.0f,
            ImGui::GetContentRegionAvail().y - window_remaining_content_height
        );

        render_current_path_parts();

        render_browser_table(browser_table_size);

        render_name_input();

        if(filters != nullptr && filters_count > 0){
            ImGui::AlignTextToFramePadding();
            
            ImGui::Text("Filter:");
            
            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
            
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            if(ImGui::Combo("##FilterCombo",&current_filter,filters,filters_count)){
                load_current_directory_entries();
                sort_current_directory_entries();
            }
        }

        render_send_and_cancel("Ok","Cancel");
        
        update_directory_entries();
    }

    ImGui::End();
}