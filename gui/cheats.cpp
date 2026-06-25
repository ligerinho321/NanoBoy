#include "cheats.hpp"

const char* popup_names[2] = {
    "Add Cheat",
    "Edit Cheat"
};

const char* format_type_names[2] = {
    "Game Genie",
    "Game Shark"
};

cheats_t::cheats_t(gb_t* gb):
    gb(gb),
    game_genie_pattern_text("^(?: *[0-9a-fA-F]{3}-[0-9a-fA-F]{3}-[0-9a-fA-F]{3} *(?:\r?\n|$))+$"),
    game_genie_pattern_code("[0-9a-fA-F]{3}-[0-9a-fA-F]{3}-[0-9a-fA-F]{3}"),
    game_shark_pattern_text("^(?: *[0-9a-fA-F]{8} *(?:\r?\n|$))+$"),
    game_shark_pattern_code("[0-9a-fA-F]{8}")
{}

cheats_t::~cheats_t(){
    clear_cheats();
}


void cheats_t::load_cheat_codes(cheat_t* cheat){

    if(cheat->codes.size() > 0){
        for(auto& code : cheat->codes){
            gb_remove_cheat_code(gb,&code);
        }
        cheat->codes.clear();
    }

    switch(cheat->format_type){
        case format_game_genie_type:{

            std::string str_codes = cheat->code_buffer;
            auto it = std::sregex_iterator(str_codes.begin(),str_codes.end(),game_genie_pattern_code);
            auto end = std::sregex_iterator();

            uint8_t nv,ahh,ahl,al,hvh,h,hvl;
            
            gb_cheat_code_t code{0};

            while(it != end){
                if(sscanf(it->str().c_str(),"%2hhx%1hhx-%2hhx%1hhx-%1hhx%1hhx%1hhx",&nv,&ahl,&al,&ahh,&hvh,&h,&hvl) != 7){
                    gb_printf_error("sscanf failed get values");
                }

                code.new_value = nv;
                
                uint8_t tmp = (hvh << 0x04) | hvl;
                code.old_value = (uint8_t)(((tmp << 0x06) | (tmp >> 0x02)) ^ 0xBA); 
                
                code.address = ((ahh << 0x0C) | (ahl << 0x08) | al) ^ 0xF000;

                code.enabled = &cheat->enabled;

                cheat->codes.push_back(code);

                ++it;
            }
            break;
        }
        case format_game_shark_type:{
            
            std::string str_codes = cheat->code_buffer;
            auto it = std::sregex_iterator(str_codes.begin(),str_codes.end(),game_shark_pattern_code);
            auto end = std::sregex_iterator();

            uint8_t type,value,addr_hi,addr_lo;

            gb_cheat_code_t code{0};

            while(it != end){
                if(sscanf(it->str().c_str(),"%2hhx%2hhx%2hhx%2hhx",&type,&value,&addr_lo,&addr_hi) != 4){
                    gb_printf_error("sscanf failed get values");
                }

                code.new_value = value;
                
                code.old_value = -1;
                
                code.address = (addr_hi << 0x08) | addr_lo;

                code.enabled = &cheat->enabled;

                cheat->codes.push_back(code);

                ++it;
            }
            break;
        }
    }

    for(auto& code : cheat->codes){
        gb_add_cheat_code(gb,&code);
    }
}


bool cheats_t::cheat_is_valid(){
    int description_buffer_length = strlen(description_buffer);

    if(!description_buffer_length){
        current_error = error_description_empty;
        return false;
    }

    std::string str_codes = code_buffer;

    if(
        (format_type == format_game_genie_type && !std::regex_match(str_codes,game_genie_pattern_text)) ||
        (format_type == format_game_shark_type && !std::regex_match(code_buffer,game_shark_pattern_text))
    ){
        current_error = error_invalid_code_format;
        return false;
    }

    return true;
}


void cheats_t::copy_code_buffer(char* dst){
    char* src = code_buffer;
    char* end = code_buffer + strlen(code_buffer);
    while(src != end){
        if(*src != ' '){
            *dst = *src;
            ++dst;
        }
        ++src;
    }
    *dst = '\0';
}


void cheats_t::add_cheat(){

    if(!cheat_is_valid()) return;

    cheat_t* cheat = new cheat_t;
    
    strcpy(cheat->description_buffer,description_buffer);
    
    copy_code_buffer(cheat->code_buffer);
    
    cheat->format_type = format_type;
    
    cheat->enabled = enabled;

    load_cheat_codes(cheat);

    cheat_t* current = cheats;

    if(current != nullptr){
        while(current->next != nullptr){
            current = current->next;
        }
        current->next = cheat;
    }
    else{
        cheats = cheat;
    }
    
    cheat->next = nullptr;

    popup_open = false;
}

void cheats_t::edit_cheat(){
    if(!cheat_is_valid()) return;

    strcpy(cheat_selected->description_buffer,description_buffer);
    
    copy_code_buffer(cheat_selected->code_buffer);
    
    cheat_selected->format_type = format_type;
    
    cheat_selected->enabled = enabled;

    load_cheat_codes(cheat_selected);

    popup_open = false;
}

void cheats_t::delete_cheat_selected(){
    if(cheat_selected == nullptr) return;

    cheat_t* prev = NULL;
    cheat_t* current = cheats;

    while(current != NULL){
        if(current == cheat_selected){
            if(prev != NULL){
                prev->next = cheat_selected->next;
            }
            else{
                cheats = cheat_selected->next;
            }
            break;
        }
        prev = current;
        current = current->next;
    }

    for(auto& code : cheat_selected->codes){
        gb_remove_cheat_code(gb,&code);
    }
    
    delete cheat_selected;
    
    cheat_selected = nullptr;
}

void cheats_t::clear_cheats(){
    
    cheat_t* cheat = cheats;

    while(cheat != nullptr){
        cheat_t* next = cheat->next;
        
        for(auto& code : cheat->codes){
            gb_remove_cheat_code(gb,&code);
        }
        
        delete cheat;
        
        cheat = next; 
    }

    cheats = nullptr;
}


void cheats_t::open_popup(int type){
    popup_open = true;
    popup_type = type;

    if(popup_type == popup_add_cheat_type){
        description_buffer[0] = '\0';
        code_buffer[0] = '\0';
        code_buffer_length = 0;
        enabled = false;
    }
    else{
        strcpy(description_buffer,cheat_selected->description_buffer);
        strcpy(code_buffer,cheat_selected->code_buffer);
        code_buffer_length = strlen(code_buffer);
        enabled = cheat_selected->enabled;
    }

    current_error = -1;

    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();

    popup_start_pos.x = window_pos.x + window_size.x * 0.5f;
    popup_start_pos.y = window_pos.y + window_size.y * 0.5f;

    ImGui::OpenPopup(popup_names[popup_type]);
}


void cheats_t::render_popup(){
    
    if(!popup_open) return;

    ImGui::SetNextWindowPos(popup_start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(popup_start_size,ImGuiCond_Appearing);

    if(!ImGui::BeginPopupModal(popup_names[popup_type],&popup_open)) return;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Description:");
    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float x = ImGui::GetCursorPosX();

    ImGui::InputText("##DescriptionInputText",description_buffer,sizeof(description_buffer));
    
    if(current_error == error_description_empty){
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(text_error_color,"Description empty");
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Format");
    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::Combo("##FormatCombo",&format_type,format_type_names,2);

    ImVec2 content_region_avail = ImGui::GetContentRegionAvail();

    ImVec2 text_multiline_size(
        content_region_avail.x,
        content_region_avail.y - ImGui::GetFrameHeightWithSpacing() * ((current_error == error_invalid_code_format) ? 2.0f : 1.0f)
    );

    ImGui::InputTextMultiline("##CodeInputTextMultiline",code_buffer,sizeof(code_buffer),text_multiline_size);

    if(ImGui::IsItemDeactivatedAfterEdit()){
        code_buffer_length = strlen(code_buffer);
    }

    if(!ImGui::IsItemActive() && !code_buffer_length){

        ImVec2 cursor = ImGui::GetCursorPos();

        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        ImGui::PushClipRect(min,max,true);

        ImGui::SetCursorScreenPos(ImVec2(min.x + style.FramePadding.x,min.y + style.FramePadding.y));

        ImGui::TextDisabled("Enter the codes here...");

        ImGui::PopClipRect();

        ImGui::SetCursorPos(cursor);
    }

    if(current_error == error_invalid_code_format){
        ImGui::TextColored(text_error_color,"Invalid code format");
    }

    ImGui::Checkbox("Enabled",&enabled);

    ImGui::SameLine();

    const char* ok = "Ok";
    const char* cancel = "Cancel";

    float button_ok_width = ImGui::CalcTextSize(ok).x + style.FramePadding.x * 2.0f;
    float button_cancel_width = ImGui::CalcTextSize(cancel).x + style.FramePadding.x * 2.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - button_ok_width -  button_cancel_width - style.ItemSpacing.x);

    if(ImGui::Button(ok)){
        switch(popup_type){
            case popup_add_cheat_type:{
                add_cheat();
                break;
            }
            case popup_edit_cheat_type:{
                edit_cheat();
                break;
            }
        }
    }

    ImGui::SameLine();

    if(ImGui::Button(cancel)) popup_open = false;

    ImGui::EndPopup();
}

void cheats_t::render(){
    if(!open) return;

    if(ImGui::Begin("Cheats",&open)){

        if(ImGui::Button("Add")) open_popup(popup_add_cheat_type);
        
        ImGui::SameLine();

        ImGui::BeginDisabled(cheat_selected == nullptr);

        if(ImGui::Button("Edit")) open_popup(popup_edit_cheat_type);

        ImGui::SameLine();
        
        if(ImGui::Button("Delete")) delete_cheat_selected();

        ImGui::EndDisabled();

        if(ImGui::BeginTable("CheatsTable",3,ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,ImGui::GetContentRegionAvail())){

            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight());
            ImGui::TableSetupColumn("Description",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Codes",ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            int index = 0;
            cheat_t* cheat = cheats;
            
            while(cheat != nullptr){

                ImGui::PushID(index++);

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Checkbox("##CheckBox",&cheat->enabled);

                ImGui::TableNextColumn();
                if(ImGui::Selectable(cheat->description_buffer,cheat == cheat_selected,ImGuiSelectableFlags_SpanAllColumns)){
                    cheat_selected = cheat;
                }

                ImGui::TableNextColumn();

                const char* start_ptr = cheat->code_buffer;
                while(true){
                    const char* end_ptr = strchr(start_ptr,'\n');

                    ImGui::TextUnformatted(start_ptr,end_ptr);

                    if(end_ptr != nullptr && end_ptr[1] != '\0'){
                        start_ptr = end_ptr + 1;
                        ImGui::SameLine(0.0f,0.0f);
                        ImGui::TextUnformatted(", ");
                        ImGui::SameLine(0.0f,0.0f);
                    }
                    else{
                        break;
                    }
                }

                ImGui::PopID();

                cheat = cheat->next;
            }

            ImGui::EndTable();
        }

        render_popup();
    }
    ImGui::End();
}