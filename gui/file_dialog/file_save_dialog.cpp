#include <gui/file_dialog/file_save_dialog.hpp>

bool file_save_dialog_t::is_current_extension(std::string extension) const noexcept {

    const char* current_extension_ptr = get_current_extension();

    if(!current_extension_ptr) return false;

    if(current_extension_ptr[0] == '\0') return true;

    if(!extension.length()) return false;

    if(!strcasecmp(extension.c_str(),current_extension_ptr)) return true;
    
    return false;
}

bool file_save_dialog_t::is_extension_supported(std::string extension) const noexcept {

    if(!extensions || extensions_count <= 0) return false;

    bool result = false;

    for(int i = 0; i < extensions_count; ++i){

        const char* name_ptr = extensions[i];
        const char* extension_ptr = name_ptr + strlen(name_ptr) + 1;

        if(extension_ptr[0] == '\0' || !strcasecmp(extension.c_str(),extension_ptr)){
            result = true;
            break;
        }
    }

    return result;
}

void file_save_dialog_t::format_path_extension(std::filesystem::path& path){
    if(!extensions || extensions_count <= 0) return;

    if(!path.has_extension()){
        path.replace_extension(get_current_extension());
    }
    else if(!is_extension_supported(path.extension().u8string())){

        std::string s = path.u8string();
        
        s += get_current_extension();

        path = s;
    }
}

void file_save_dialog_t::select_file(std::filesystem::path& path){

    overwrite_file_name = path.filename().u8string();
    overwrite_file_path = path;

    request_open_popup_modal = true;
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
            select_file(path);
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

void file_save_dialog_t::render_popup_modal(){

    static const char* name = "Overwrite the file?";

    if(request_open_popup_modal){
        request_open_popup_modal = false;

        ImGui::OpenPopup(name);
        popup_modal_open = true;

        ImVec2 size = ImGui::GetMainViewport()->Size;

        popup_modal_start_pos.x = size.x * 0.5f;
        popup_modal_start_pos.y = size.y * 0.5f;
    }

    if(!popup_modal_open) return;

    ImGui::SetNextWindowPos(popup_modal_start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));

    if(!ImGui::BeginPopupModal(name,&popup_modal_open,ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) return;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Text("The file \"%s\" already exists. Do you want to overwrite it?",overwrite_file_name.c_str());

    const char* overwrite = "Overwrite";
    const char* cancel = "Cancel";

    float overwrite_button_width = ImGui::CalcTextSize(overwrite).x + style.FramePadding.x * 2.0f;
    float cancel_button_width = ImGui::CalcTextSize(cancel).x + style.FramePadding.x * 2.0f;

    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 
        overwrite_button_width - cancel_button_width - 
        style.ItemSpacing.x
    );

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