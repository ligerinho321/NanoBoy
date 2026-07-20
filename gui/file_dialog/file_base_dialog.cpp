#include <gui/file_dialog/file_base_dialog.hpp>

file_base_dialog_t::file_base_dialog_t(){
    set_current_path(std::filesystem::current_path());

    ImGuiStyle style = ImGui::GetStyle();

    window_remaining_content_height = ImGui::GetFrameHeightWithSpacing() * 4.0f;

    window_min.x = style.WindowMinSize.x;
    window_min.y = window_remaining_content_height + 200 + (style.WindowPadding.y * 2.0f);

    window_max.x = FLT_MAX;
    window_max.y = FLT_MAX;
}


void file_base_dialog_t::set_current_path(std::filesystem::path path){
    current_path = path;
    
    current_path_parts.clear();
    
    std::filesystem::path current;

#ifdef _WIN32
    auto it = current_path.begin();
    auto end = current_path.end();
    
    current /= *it++; //driver exemple "C:"
    current /= *it++; //root path "\\"
    current_path_parts.emplace_back(current_path.begin()->u8string(),current);

    for(;it != end; ++it){
        current /= *it;
        current_path_parts.emplace_back(it->u8string(),current);
    }
#else
    for(auto it = current_path.begin(); it != current_path.end(); ++it){
        current /= *it;
        current_path_parts.emplace_back(it->u8string(),current);
    }
#endif
}


std::uintmax_t file_base_dialog_t::number_of_entries_in_directory(const std::filesystem::path directory){
    uintmax_t n = 0;
    std::error_code error;

    for(auto& entry : std::filesystem::directory_iterator(directory,std::filesystem::directory_options::skip_permission_denied,error)){
        n++;
    }

    return n;
}

std::time_t file_base_dialog_t::get_entry_last_write_time(const std::filesystem::path entry){

    std::chrono::time_point sys_time_point = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        std::filesystem::last_write_time(entry) - 
        std::filesystem::file_time_type::clock::now() + 
        std::chrono::system_clock::now()
    );

    return std::chrono::system_clock::to_time_t(sys_time_point);
}

const char* file_base_dialog_t::get_entry_date_formated(const directory_entry_t& entry){
    static char buffer[32] = {0};

    tm* local_timer = std::localtime(&entry.last_write_time);
    
    std::strftime(buffer,sizeof(buffer),"%d/%m/%Y %H:%M",local_timer);

    return buffer;
}


const char* file_base_dialog_t::get_current_extension() const noexcept {
    const char* extension_ptr = nullptr;

    if(extensions != nullptr && extensions_count > 0){
        const char* name_ptr = extensions[current_extension];
        extension_ptr = name_ptr + strlen(name_ptr) + 1;
    }

    return extension_ptr;
}


void file_base_dialog_t::sort_current_directory_entries(){
    std::sort(current_directory_entries.begin(),current_directory_entries.end(),[this](const directory_entry_t& a,const directory_entry_t& b){
        bool result = false;

        switch(sort_column_index){
            //Name
            case 0:{
                std::string name1 = a.path.filename().u8string();
                std::string name2 = b.path.filename().u8string();
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

void file_base_dialog_t::load_current_directory_entries(){

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
        else if(std::filesystem::is_regular_file(entry_path) && is_current_extension(entry_path.extension().u8string())){
            
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


void file_base_dialog_t::update_directory_entries(){
    if(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_update).count() >= 2){
        load_current_directory_entries();
        sort_current_directory_entries();
    }
}


std::filesystem::path file_base_dialog_t::get_name_buffer_formated(){
    std::filesystem::path path = name_buffer;

    if(path.is_relative()){
        path = current_path / path;
    }

    //removing ".",".."
    path = path.lexically_normal();

    //removing last "/" in path. example: before /a/b/ after /a/b
    if(!path.has_filename()){
        path = path.parent_path();
    }

    return path;
}


void file_base_dialog_t::render_current_path_parts(){
    if(!current_path_parts.size()) return;

    ImGuiStyle& style = ImGui::GetStyle();

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


void file_base_dialog_t::render_directory(){

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
                    select_file(entry.path);
                }

                ImGui::TableNextColumn();
                
                render_size_text(entry.size);

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

void file_base_dialog_t::render_browser_table(ImVec2 size){

    if(!ImGui::BeginTable("BrowserTable",3,ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter,size)) return;
        
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

void file_base_dialog_t::render_name_input(){
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::AlignTextToFramePadding();
    
    ImGui::TextUnformatted("Name:");
    
    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
    
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    bool result = ImGui::InputText("##NameInputText",name_buffer,sizeof(name_buffer),ImGuiInputTextFlags_EnterReturnsTrue);

    if(ImGui::IsItemEdited()){
        name_buffer_length = strlen(name_buffer);
    }

    if(result && name_buffer_length > 0){
        send_name_buffer();
    }
}

void file_base_dialog_t::render_extension_combo(){
    if(!extensions || extensions_count <= 0) return;

    ImGuiStyle& style = ImGui::GetStyle();
    
    ImGui::AlignTextToFramePadding();
    
    ImGui::Text("Extension:");
    
    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
    
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if(ImGui::Combo("##ExtensionCombo",&current_extension,extensions,extensions_count)){
        load_current_directory_entries();
        sort_current_directory_entries();
    }
}

void file_base_dialog_t::render_send_and_cancel(const char* send,const char* cancel){

    ImGuiStyle& style = ImGui::GetStyle();
    
    float button_send_width = ImGui::CalcTextSize(send).x + style.FramePadding.x * 2.0f;
    float button_cancel_width = ImGui::CalcTextSize(cancel).x + style.FramePadding.x * 2.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - button_send_width - button_cancel_width - style.ItemSpacing.x);

    ImGui::BeginDisabled(!name_buffer_length);

    if(ImGui::Button(send)){
        send_name_buffer();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();
    
    if(ImGui::Button(cancel)){
        open = false;
    }
}

void file_base_dialog_t::render(const char* name,const char* send,const char* cancel){

    if(request_open){
        request_open = false;

        ImGui::OpenPopup(name);
        open = true;

        ImVec2 size = ImGui::GetMainViewport()->Size;
        
        window_start_pos.x = size.x * 0.5f;
        window_start_pos.y = size.y * 0.5f;
    }

    if(!open) return;

    ImGui::SetNextWindowPos(window_start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSizeConstraints(window_min,window_max);

    if(!ImGui::BeginPopupModal(name,&open)) return;

    ImVec2 browser_table_size = ImVec2(
        0.0f,
        ImGui::GetContentRegionAvail().y - window_remaining_content_height
    );

    render_current_path_parts();

    render_browser_table(browser_table_size);

    render_name_input();

    render_extension_combo();

    render_send_and_cancel(send,cancel);

    update_directory_entries();

    render_popup_modal();
    
    ImGui::EndPopup();
}
