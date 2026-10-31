#include <gui/debug/memory_viewer_base.hpp>

static const char* file_extension = "All files\0";

void memory_viewer_base_t::init(){
    
    file_selector.init();
    file_selector.set_extensions(&file_extension,1);

    file_save.init();
    file_save.set_extensions(&file_extension,1);

    ImGuiStyle& style = ImGui::GetStyle();

    input_text_flags = (
        ImGuiInputTextFlags_AutoSelectAll |
        ImGuiInputTextFlags_NoHorizontalScroll |
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_AlwaysOverwrite |
        ImGuiInputTextFlags_CallbackEdit |
        ImGuiInputTextFlags_CharsHexadecimal
    );

    line_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]);
    text_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    text_disabled_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);
}


int memory_viewer_base_t::input_text_callback(ImGuiInputTextCallbackData* data){
    if(data->EventFlag == ImGuiInputTextFlags_CallbackEdit){
        memory_viewer_base_t* memory_viewer = (memory_viewer_base_t*)data->UserData;
        memory_viewer->buffer_cursor_pos = data->CursorPos;
    }
    return 0;
}


void memory_viewer_base_t::update_current_memory_type(){
    memory_length = get_current_memory_length();

    address_digit_count = snprintf(nullptr,0,"%zX",memory_length - 1);

    editing_address = 0;
    
    need_focus_editing_address = true;

    jump_near = true;

    update_rows();
}

void memory_viewer_base_t::update_rows(){
    rows = ceil((double)memory_length / (double)columns);
}


void memory_viewer_base_t::event(){
    if(!ImGui::GetIO().WantTextInput){
        if(ImGui::IsKeyPressed(ImGuiKey_UpArrow) && editing_address >= columns){
            editing_address -= columns;
            jump_near = true;
        }
        else if(ImGui::IsKeyPressed(ImGuiKey_DownArrow) && editing_address < (memory_length > columns ? memory_length - columns : 0)){
            editing_address += columns;
            jump_near = true;
        }
        else if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && editing_address > 0){
            --editing_address;
            jump_near = true;
        }
        else if(ImGui::IsKeyPressed(ImGuiKey_RightArrow) && editing_address < (memory_length - 1)){
            ++editing_address;
            jump_near = true;
        }
    }
}


void memory_viewer_base_t::render_control(){

    if(ImGui::Button("Import")){
        file_selector.set_open(true);
    }

    ImGui::SameLine();

    if(ImGui::Button("Export")){
        file_save.set_open(true);
    }

    ImGui::SeparatorText("View Options");

    render_memory_type_combo();

    uint32_t columns_step = 1;

    if(ImGui::InputScalar("Columns",ImGuiDataType_U32,&columns,&columns_step,nullptr,"%" PRIu32)){
        if(columns > memory_viewer_base_t::max_columns){
            columns = memory_viewer_base_t::max_columns;
        }
        else if(columns < memory_viewer_base_t::min_columns){
            columns = memory_viewer_base_t::min_columns;
        }
        update_rows();
    }

    uint64_t editing_address_step = 1;

    if(ImGui::InputScalar("Address",ImGuiDataType_U64,&editing_address,&editing_address_step,nullptr,"%" PRIX64)){
        if(editing_address >= memory_length){
            editing_address = memory_length - 1;
        }
        jump_far = true;
    }

    ImGui::SeparatorText("Data Preview");

    uint8_t data = read_byte(editing_address);
    
    ImGui::Text("Hexadecimal: %02X",data);
    ImGui::Text("Signed Interger: %" SCNd8,data);
    ImGui::Text("Unsigned Interger: %" SCNu8,data);
    ImGui::Text("String: %c",data < 0x20 || data >= 0x7F ? '\0' : data);
}


void memory_viewer_base_t::render_row_hex(uint64_t address,float pair_width){
    ImGuiStyle& style = ImGui::GetStyle();

    uint32_t column = 0;

    while(column < columns && address < memory_length){
        
        ImGui::SameLine(0.0f,style.ItemSpacing.x);

        if(address == editing_address){

            ImGui::PushID((void*)address);

            ImGui::SetNextItemWidth(pair_width);

            if(need_focus_editing_address){

                need_focus_editing_address = false;

                buffer_cursor_pos = 0;

                snprintf(buffer,sizeof(buffer),"%02X",read_byte(address));

                ImGui::SetKeyboardFocusHere();
            }

            ImGui::InputText("##TextInputData",buffer,sizeof(buffer),input_text_flags,input_text_callback,this);

            if(buffer_cursor_pos >= 2 || ImGui::IsItemDeactivatedAfterEdit()){
                
                uint8_t new_value = strtol(buffer,nullptr,16);

                write_byte(new_value,editing_address);

                if(buffer_cursor_pos >= 2 && editing_address < (memory_length - 1)){
                    ++editing_address;
                    need_focus_editing_address = true;
                }
            }
            else if(!ImGui::IsItemActive()){
                snprintf(buffer,sizeof(buffer),"%02X",read_byte(address));
            }
            
            ImGui::PopID();
        }
        else{
            ImGui::Text("%02X",read_byte(address));

            if(ImGui::IsItemHovered() && ImGui::GetIO().MouseReleased[0]){
                editing_address = address;
                need_focus_editing_address = true;
            }
        }

        ++column;
        ++address;
    }
}

void memory_viewer_base_t::render_row_chr(uint64_t address){
    ImGuiStyle& style = ImGui::GetStyle();

    uint32_t column = 0;

    while(column < columns && address < memory_length){

        if(column > 0){
            ImGui::SameLine(0.0f,style.ItemSpacing.x);
        }

        uint8_t data = read_byte(address);

        char c = (data < 0x20 || data >= 0x7F) ? '.' : data;

        ImGui::PushStyleColor(ImGuiCol_Text,data == c ? text_color : text_disabled_color);

        ImGui::TextUnformatted(&c,&c + 1);
        
        ImGui::PopStyleColor();

        ++column;
        ++address;
    }
}


void memory_viewer_base_t::render_dump(){

    if(ImGui::BeginChild("MemoryViewerChild",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_HorizontalScrollbar)){

        event();

        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        float pair_width = ImGui::CalcTextSize("00").x;
        float chr_width = ImGui::CalcTextSize("0").x;

        float address_width = ImGui::CalcTextSize(std::string(address_digit_count + 1,'0').c_str()).x;
        float hex_content_width = (address_width + style.ItemSpacing.x) + (pair_width + style.ItemSpacing.x) * columns;
        float chr_content_width = (chr_width + style.ItemSpacing.x) * columns;

        float visible_width = window->InnerRect.GetWidth();
        float content_width = (hex_content_width + chr_content_width) + style.WindowPadding.x * 2.0f;

        float visible_height = window->InnerRect.GetHeight();
        float row_height = ImGui::GetTextLineHeightWithSpacing();
        double content_height = ((double)rows * row_height) + (style.WindowPadding.y * 2.0f) - style.ItemSpacing.y;

        ImVec2 clip_rect_min = window->InnerRect.Min;
        ImVec2 clip_rect_max = window->InnerRect.Max;

        bool v_active = content_height > visible_height;
        bool h_active = content_width > visible_width - (v_active ? style.ScrollbarSize : 0.0f);
        v_active = content_height > visible_height - (h_active ? style.ScrollbarSize : 0.0f);

        if(v_active){
            visible_width -= style.ScrollbarSize;
            clip_rect_max.x -= style.ScrollbarSize;
        }
        if(h_active){
            visible_height -= style.ScrollbarSize;
            clip_rect_max.y -= style.ScrollbarSize;
        }

        if(jump_near || jump_far){
            
            uint64_t row = editing_address / columns;
            double top = (double)row * row_height;
            double bottom = top + row_height - style.ItemSpacing.y;

            if(jump_near){
                if(top + style.WindowPadding.y < scroll_y){
                    scroll_y = top;
                }
                else if(bottom + style.WindowPadding.y > scroll_y + visible_height){
                    scroll_y = bottom + style.WindowPadding.y * 2.0f - visible_height;
                }
            }
            else{
                if(top + style.WindowPadding.y < scroll_y || bottom + style.WindowPadding.y > scroll_y + visible_height){
                    scroll_y = top + style.WindowPadding.y + row_height * 0.5f - visible_height * 0.5f;
                }
            }

            jump_near = false;
            jump_far = false;

        }

        double scroll_min_y = 0.0;
        double scroll_max_y = content_height - visible_height;

        if(scroll_max_y < 0.0) scroll_max_y = 0.0;

        if(scroll_y < 0.0) scroll_y = 0.0;

        if(scroll_y > scroll_max_y) scroll_y = scroll_max_y;

        uint64_t start_row = scroll_y / row_height;

        float offset = scroll_y - (double)start_row * row_height;

        uint64_t visible_rows = ceilf((visible_height + offset) / row_height);

        uint64_t end_row = start_row + visible_rows;

        if(end_row > rows) end_row = rows;


        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0.0f,0.0f));

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - offset);

        ImGui::PushClipRect(clip_rect_min,clip_rect_max,true);

        for(uint64_t row = start_row; row < end_row; ++row){

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - scroll_x);

            uint64_t start_address = row * columns;

            ImVec2 cursor_start = ImGui::GetCursorPos();

            ImGui::PushStyleColor(ImGuiCol_Text,text_disabled_color);

            ImGui::Text("%0*" PRIX64 ":",address_digit_count,start_address);
            
            ImGui::PopStyleColor();

            render_row_hex(start_address,pair_width);

            ImGui::SetCursorPos(ImVec2(cursor_start.x + hex_content_width + style.ItemSpacing.x,cursor_start.y));

            render_row_chr(start_address);
        }

        ImGui::PopClipRect();

        ImGui::PopStyleVar();

        if(content_width > visible_width){            
            ImGuiID horizontal_scrollbar_id = ImGui::GetWindowScrollbarID(window,ImGuiAxis_X);

            ImRect horizontal_scrollbar_rect(
                window->InnerRect.Min.x,
                window->InnerRect.Max.y - style.ScrollbarSize,
                window->InnerRect.Max.x - ((content_height > visible_height) ? style.ScrollbarSize : 0.0f),
                window->InnerRect.Max.y
            );

            ImS64 _scroll_x = (ImS64)scroll_x;
            ImGui::ScrollbarEx(horizontal_scrollbar_rect,horizontal_scrollbar_id,ImGuiAxis_X,&_scroll_x,visible_width,content_width);
            scroll_x = (float)_scroll_x;
        }

        if(content_height > visible_height){

            ImRect vertical_scrollbar_rect(
                window->InnerRect.Max.x - style.ScrollbarSize,
                window->InnerRect.Min.y,
                window->InnerRect.Max.x,
                window->InnerRect.Max.y - ((content_width > visible_width) ? style.ScrollbarSize : 0.0f)
            );

            float grab_height = vertical_scrollbar_rect.GetHeight() * (visible_height / content_height);
            
            if(grab_height < style.GrabMinSize){
                grab_height = style.GrabMinSize;
            }

            double inverted_scroll_y = scroll_max_y - scroll_y;

            ImGui::SetCursorScreenPos(vertical_scrollbar_rect.Min);

            ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab,       ImGui::GetColorU32(ImGuiCol_ScrollbarGrab));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive));
            
            ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding,  ImGui::GetStyle().ScrollbarRounding);
            ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, grab_height);

            if(ImGui::VSliderScalar("##ScrollY",vertical_scrollbar_rect.GetSize(),ImGuiDataType_Double,&inverted_scroll_y,&scroll_min_y,&scroll_max_y,"")){
                scroll_y = scroll_max_y - inverted_scroll_y;
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(5);
        }

    }
    ImGui::EndChild();
}

void memory_viewer_base_t::render(){
    if(!open) return;

    if(!cartridge_inserted()){
        set_open(false);
        return;
    }

    bool _open = open;

    if(ImGui::Begin("Memory Viewer",&_open)){

        if(ImGui::BeginTable("MemorViewerTable",2)){
            
            ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            render_dump();

            ImGui::TableNextColumn();

            render_control();

            ImGui::EndTable();
        }

        file_save.render();

        file_selector.render();
    }
    ImGui::End();

    set_open(_open);
}


void memory_viewer_base_t::clear(){
    editing_address = 0;
    need_focus_editing_address = false;
    buffer_cursor_pos = 0;
    buffer[0] = '\0';
    jump_near = false;
    jump_far = false;
    memory_length = 0;
    address_digit_count = 0;
    update_current_memory_type();
}