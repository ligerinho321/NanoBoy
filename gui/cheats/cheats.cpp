#include <gui/cheats/cheats.hpp>

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
    clear(true);
}


void cheats_t::copy_valuestring_to_buffer(const char* valuestring,char* buffer){
    int len = strlen(valuestring);

    if(len >= buffer_length){
        len = buffer_length - 1;
    }

    memcpy(buffer,valuestring,len);

    buffer[len] = '\0';
}

void cheats_t::load_cheat(cJSON* object,bool thread_safe){

    cJSON* item = object->child;

    description_buffer_length = 0;
    codes_buffer_length = 0;
    format_type = 0;
    enabled = false;

    while(item != nullptr){

        if(cJSON_IsString(item)){
            if(item->string != nullptr && item->valuestring != nullptr){
                if(!strcmp("Description",item->string)){
                    copy_valuestring_to_buffer(item->valuestring,description_buffer);
                    description_buffer_length = strlen(description_buffer);
                }
                else if(!strcmp("Codes",item->string)){
                    copy_valuestring_to_buffer(item->valuestring,codes_buffer);
                    codes_buffer_length = strlen(codes_buffer);
                }
            }
        }
        else if(cJSON_IsBool(item)){
            if(item->string != nullptr){
                if(!strcmp("Enabled",item->string)){
                    enabled = cJSON_IsTrue(item);
                }
            }
        }
        else if(cJSON_IsNumber(item)){
            if(item->string != nullptr){
                if(!strcmp("Format",item->string)){
                    format_type = (int)item->valuedouble;
                }
            }
        }

        item = item->next;
    }

    add_cheat(thread_safe);
}

void cheats_t::load(const char* path,bool thread_safe){
    char* data = nullptr;
    size_t len = 0;

    cJSON* json = nullptr;
    cJSON* array = nullptr;
    cJSON* object = nullptr;

    if(!gb_load_file(path,(void**)&data,&len)){
        goto end;
    }

    json = cJSON_ParseWithLength(data,len);
    if(!json){
        gb_printf_error("cJSON_ParserWithLength failed\n");
        goto end;
    }

    array = cJSON_GetObjectItemCaseSensitive(json,"Cheats");
    if(!array){
        gb_printf_error("cJSON_GetObjectItemCaseSensitive failed");
        goto end;
    }

    object = array->child;
    while(object != nullptr){
        load_cheat(object,thread_safe);
        object = object->next;
    }

    end:
    if(data != nullptr) free(data);
    if(json != nullptr) cJSON_Delete(json);
}

void cheats_t::save(const char* path){
    if(cheats == nullptr) return;

    cJSON* json = cJSON_CreateObject();
    
    cJSON* array = cJSON_CreateArray();

    cJSON_AddItemToObjectCS(json,"Cheats",array);

    cheat_t* cheat = cheats;
    
    while(cheat != nullptr){

        cJSON* cheat_object = cJSON_CreateObject();

        cJSON* _description = cJSON_CreateStringReference(cheat->description_buffer);
        cJSON* _codes = cJSON_CreateStringReference(cheat->codes_buffer);
        cJSON* _format = cJSON_CreateNumber(cheat->format_type);
        cJSON* _enabled = cJSON_CreateBool(cheat->enabled);

        cJSON_AddItemToObjectCS(cheat_object,"Description",_description);
        cJSON_AddItemToObjectCS(cheat_object,"Codes",_codes);
        cJSON_AddItemToObjectCS(cheat_object,"Format",_format);
        cJSON_AddItemToObjectCS(cheat_object,"Enabled",_enabled);

        cJSON_AddItemToArray(array,cheat_object);

        cheat = cheat->next;
    }

    char* string = cJSON_Print(json);
    
    if(!gb_save_file(path,string,strlen(string))){
        gb_printf_error("gb_save_file failed");        
    }

    free(string);

    cJSON_Delete(json);
}


bool cheats_t::cheat_is_valid(){
    if(!description_buffer_length){
        current_error = error_description_empty;
        return false;
    }

    if(!codes_buffer_length){
        current_error = error_codes_empty;
        return false;
    }

    switch(format_type){
        case format_game_genie_type:
            if(!std::regex_match(codes_buffer,game_genie_pattern_text)){
                current_error = error_invalid_code_format;
                return false;
            }
            break;
        case format_game_shark_type:
            if(!std::regex_match(codes_buffer,game_shark_pattern_text)){
                current_error = error_invalid_code_format;
                return false;
            }
            break;
        default:
            return false;
    }

    return true;
}

void cheats_t::copy_codes_buffer(char* dst){
    char* src = codes_buffer;
    char* end = codes_buffer + strlen(codes_buffer);
    while(src != end){
        if(*src != ' '){
            *dst = *src;
            ++dst;
        }
        ++src;
    }
    *dst = '\0';
}

void cheats_t::load_cheat_codes(cheat_t* cheat,bool thread_safe){

    if(thread_safe) gb_thread_stop(gb);

    if(cheat->codes.size() > 0){

        for(auto& code : cheat->codes){
            gb_remove_cheat_code(gb,&code);
        }

        cheat->codes.clear();
    }

    switch(cheat->format_type){
        case format_game_genie_type:{

            std::string str_codes = cheat->codes_buffer;
            auto it = std::sregex_iterator(str_codes.begin(),str_codes.end(),game_genie_pattern_code);
            auto end = std::sregex_iterator();

            uint8_t nv,ahh,ahl,al,hvh,h,hvl;
            
            gb_cheat_code_t code{};

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
            
            std::string str_codes = cheat->codes_buffer;
            auto it = std::sregex_iterator(str_codes.begin(),str_codes.end(),game_shark_pattern_code);
            auto end = std::sregex_iterator();

            uint8_t type,value,addr_hi,addr_lo;

            gb_cheat_code_t code{};

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

    if(thread_safe) gb_thread_start(gb);
}

void cheats_t::add_cheat(bool thread_safe){

    if(!cheat_is_valid()) return;

    cheat_t* cheat = new cheat_t;
    
    strcpy(cheat->description_buffer,description_buffer);
    
    copy_codes_buffer(cheat->codes_buffer);
    
    cheat->format_type = format_type;
    
    cheat->enabled = enabled;

    load_cheat_codes(cheat,thread_safe);

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

    popup_modal_open = false;
}

void cheats_t::edit_cheat(bool thread_safe){
    if(!cheat_is_valid()) return;

    strcpy(cheat_selected->description_buffer,description_buffer);
    
    copy_codes_buffer(cheat_selected->codes_buffer);
    
    cheat_selected->format_type = format_type;
    
    cheat_selected->enabled = enabled;

    load_cheat_codes(cheat_selected,thread_safe);

    popup_modal_open = false;
}

void cheats_t::delete_cheat_selected(bool thread_safe){
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

    if(thread_safe) gb_thread_stop(gb);

    for(auto& code : cheat_selected->codes){
        gb_remove_cheat_code(gb,&code);
    }

    if(thread_safe) gb_thread_start(gb);
    
    delete cheat_selected;
    
    cheat_selected = nullptr;
}

void cheats_t::clear(bool thread_safe){
    
    cheat_t* cheat = cheats;

    if(thread_safe) gb_thread_stop(gb);

    while(cheat != nullptr){
        cheat_t* next = cheat->next;
        
        for(auto& code : cheat->codes){
            gb_remove_cheat_code(gb,&code);
        }
        
        delete cheat;
        
        cheat = next; 
    }

    if(thread_safe) gb_thread_start(gb);

    cheats = nullptr;
}


void cheats_t::open_popup(int type){
    
    request_open_popup_modal = true;

    popup_modal_type = type;

    if(popup_modal_type == popup_add_cheat_type){
        
        description_buffer[0] = '\0';
        description_buffer_length = 0;

        codes_buffer[0] = '\0';
        codes_buffer_length = 0;

        format_type = format_game_genie_type;
        
        enabled = false;
    }
    else{
        strcpy(description_buffer,cheat_selected->description_buffer);
        description_buffer_length = strlen(description_buffer);

        strcpy(codes_buffer,cheat_selected->codes_buffer);
        codes_buffer_length = strlen(codes_buffer);
        
        format_type = cheat_selected->format_type;
        
        enabled = cheat_selected->enabled;
    }

    current_error = -1;
}


void cheats_t::render_popup_modal(){
    
    if(request_open_popup_modal){
        request_open_popup_modal = false;

        ImGui::OpenPopup(popup_names[popup_modal_type]);
        popup_modal_open = true;

        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();

        popup_modal_start_pos.x = window_pos.x + window_size.x * 0.5f;
        popup_modal_start_pos.y = window_pos.y + window_size.y * 0.5f;
    }

    if(!popup_modal_open) return;

    ImGui::SetNextWindowPos(popup_modal_start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(popup_modal_start_size,ImGuiCond_Appearing);

    if(!ImGui::BeginPopupModal(popup_names[popup_modal_type],&popup_modal_open)) return;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Description:");
    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float x = ImGui::GetCursorPosX();

    ImGui::InputText("##DescriptionInputText",description_buffer,sizeof(description_buffer));
    
    if(ImGui::IsItemDeactivatedAfterEdit()){
        description_buffer_length = strlen(description_buffer);
    }

    if(current_error == error_description_empty){
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(text_error_color,"Description empty");
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Format:");
    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::Combo("##FormatCombo",&format_type,format_type_names,2);

    ImVec2 content_region_avail = ImGui::GetContentRegionAvail();

    float frames = ((current_error == error_codes_empty || current_error == error_invalid_code_format) ? 2.0f : 1.0f);
    ImVec2 text_multiline_size(
        content_region_avail.x,
        content_region_avail.y - ImGui::GetFrameHeightWithSpacing() * frames
    );

    ImGui::InputTextMultiline("##CodeInputTextMultiline",codes_buffer,sizeof(codes_buffer),text_multiline_size);

    if(ImGui::IsItemDeactivatedAfterEdit()){
        codes_buffer_length = strlen(codes_buffer);
    }

    if(!ImGui::IsItemActive() && !codes_buffer_length){

        ImVec2 cursor = ImGui::GetCursorPos();

        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        ImGui::PushClipRect(min,max,true);

        ImGui::SetCursorScreenPos(ImVec2(min.x + style.FramePadding.x,min.y + style.FramePadding.y));

        ImGui::TextDisabled("Enter the codes here...");

        ImGui::PopClipRect();

        ImGui::SetCursorPos(cursor);
    }

    if(current_error == error_codes_empty){
        ImGui::TextColored(text_error_color,"Codes empty");
    }
    else if(current_error == error_invalid_code_format){
        ImGui::TextColored(text_error_color,"Invalid code format");
    }

    ImGui::Checkbox("Enabled",&enabled);

    ImGui::SameLine();

    const char* ok = "Ok";
    const char* cancel = "Cancel";

    float ok_button_width = ImGui::CalcTextSize(ok).x + style.FramePadding.x * 2.0f;
    float cancel_button_width = ImGui::CalcTextSize(cancel).x + style.FramePadding.x * 2.0f;

    ImGui::SetCursorPosX((ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x) - (ok_button_width +  cancel_button_width + style.ItemSpacing.x));

    if(ImGui::Button(ok)){
        if(popup_modal_type == popup_add_cheat_type){
            add_cheat(true);
        }
        else{
            edit_cheat(true);
        }
    }

    ImGui::SameLine();

    if(ImGui::Button(cancel)){
        popup_modal_open = false;
    }

    ImGui::EndPopup();
}

void cheats_t::render(){

    if(!open) return;

    if(!gb->cartridge_inserted){
        open = false;
        return;
    }

    if(ImGui::Begin("Cheats",&open)){

        if(ImGui::Button("Add")) open_popup(popup_add_cheat_type);
        
        ImGui::SameLine();

        ImGui::BeginDisabled(cheat_selected == nullptr);

        if(ImGui::Button("Edit")) open_popup(popup_edit_cheat_type);

        ImGui::SameLine();
        
        if(ImGui::Button("Delete")) delete_cheat_selected(true);

        ImGui::EndDisabled();

        if(ImGui::BeginTable("CheatsTable",3,ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,ImGui::GetContentRegionAvail())){

            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight());
            ImGui::TableSetupColumn("Description",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Codes",ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            int id = 0;
            cheat_t* cheat = cheats;
            
            while(cheat != nullptr){

                ImGui::PushID(id++);

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Checkbox("##CheckBox",&cheat->enabled);

                ImGui::TableNextColumn();
                if(ImGui::Selectable(cheat->description_buffer,cheat == cheat_selected,ImGuiSelectableFlags_SpanAllColumns)){
                    cheat_selected = cheat;
                }

                ImGui::TableNextColumn();

                const char* start_ptr = cheat->codes_buffer;
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

        render_popup_modal();
    }

    ImGui::End();
}