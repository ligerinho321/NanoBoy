#include <gui/file_dialog/file_save_dialog.hpp>

const char* file_save_dialog_t::get_current_extension() const noexcept {
    const char* extension = nullptr;

    if(formats != nullptr && formats_count > 0){
        const char* name = formats[current_format];
        extension = name + strlen(name) + 1;
    }

    return extension;
}

bool file_save_dialog_t::extension_supported(std::string extension){
    if(!formats || formats_count <= 0) return false;

    bool result = false;

    for(int i = 0; i < formats_count; ++i){

        const char* name_ptr = formats[i];
        const char* extension_ptr = name_ptr + strlen(name_ptr) + 1;

        if(!strcasecmp(extension.c_str(),extension_ptr)){
            result = true;
            break;
        }
    }

    return result;
}

void file_save_dialog_t::format_path_extension(std::filesystem::path& path){
    if(!formats || formats_count <= 0) return;

    if(!path.has_extension()){
        path.replace_extension(get_current_extension());
    }
    else if(!extension_supported(path.extension().u8string())){

        std::string s = path.u8string();
        
        s += get_current_extension();

        path = s;
    }
}


void file_save_dialog_t::load_current_directory_entries(){

    current_directory_entries.clear();

    std::error_code error;

    const char* current_extension_ptr = get_current_extension();

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
        else if(current_extension_ptr != nullptr && std::filesystem::is_regular_file(entry_path)){
            
            std::string extension = entry_path.extension().u8string();

            if(!strcasecmp(extension.c_str(),current_extension_ptr)){

                current_directory_entries.emplace_back(
                    "[FILE] " + entry_path.filename().u8string(),
                    entry_path,
                    std::filesystem::file_size(entry_path),
                    get_entry_last_write_time(entry_path)
                );
            }
        }

    }

    last_update = std::chrono::steady_clock::now();
}


void file_save_dialog_t::send_name_buffer(){

    std::filesystem::path path = get_name_buffer_formated();
    
    if(std::filesystem::exists(path)){
        if(std::filesystem::is_directory(path)){
            set_current_path(path);
            load_current_directory_entries();
            sort_current_directory_entries();
        }
        else if(std::filesystem::is_regular_file(path)){
            overwrite_file_name = path.filename().u8string();
            overwrite_file_path = path;

            request_open_popup_modal = true;
        }
    }
    else{
        if(callback != nullptr){

            format_path_extension(path);

            callback(userdata,path);
        }

        open = false;
    }

    clear_name_buffer();
}


void file_save_dialog_t::render_directory(){

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
                    
                    overwrite_file_name = entry.path.filename().u8string();
                    overwrite_file_path = entry.path;

                    request_open_popup_modal = true;
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

void file_save_dialog_t::render_popup_modal(){

    const char* popup_modal_name = "Overwrite the file?";

    if(request_open_popup_modal){
        request_open_popup_modal = false;

        ImGui::OpenPopup(popup_modal_name);
        popup_modal_open = true;

        ImVec2 size = ImGui::GetMainViewport()->Size;

        popup_modal_start_pos.x = size.x * 0.5f;
        popup_modal_start_pos.y = size.y * 0.5f;
    }

    if(!popup_modal_open) return;

    ImGui::SetNextWindowPos(popup_modal_start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));

    if(!ImGui::BeginPopupModal(popup_modal_name,&popup_modal_open,ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) return;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Text("The file \"%s\" already exists. Do you want to overwrite it?",overwrite_file_name.c_str());

    const char* overwrite = "Overwrite";
    const char* cancel = "Cancel";

    float overwrite_button_width = ImGui::CalcTextSize(overwrite).x + style.FramePadding.x * 2.0f;
    float cancel_button_width = ImGui::CalcTextSize(cancel).x + style.FramePadding.x * 2.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - overwrite_button_width - cancel_button_width - style.ItemSpacing.x);

    if(ImGui::Button(overwrite)){
        
        if(callback != nullptr){

            format_path_extension(overwrite_file_path);

            callback(userdata,overwrite_file_path);
        }

        popup_modal_open = false;
        open = false;
    }

    ImGui::SameLine();

    if(ImGui::Button(cancel)){
        popup_modal_open = false;
    }

    ImGui::EndPopup();
}

void file_save_dialog_t::render(){

    if(!open) return;

    ImGui::SetNextWindowSizeConstraints(window_min,window_max);

    if(ImGui::Begin("File Save",&open)){

        ImGuiStyle& style = ImGui::GetStyle();

        ImVec2 browser_table_size = ImVec2(
            0.0f,
            ImGui::GetContentRegionAvail().y - window_remaining_content_height
        );

        render_current_path_parts();

        render_browser_table(browser_table_size);

        render_name_input();

        if(formats != nullptr && formats_count > 0){
            
            ImGui::AlignTextToFramePadding();
            
            ImGui::Text("Format:");
            
            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
            
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            if(ImGui::Combo("##FormatCombo",&current_format,formats,formats_count)){
                load_current_directory_entries();
                sort_current_directory_entries();
            }
        }

        render_send_and_cancel("Save","Cancel");

        update_directory_entries();
    }
    ImGui::End();

    render_popup_modal();
}