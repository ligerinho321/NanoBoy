#include <gui/memory_viewer/memory_viewer.hpp>

static const char* file_extension = "All files\0";


memory_viewer_t::memory_viewer_t(gb_t* gb):gb(gb){

    file_selector.set_extensions(&file_extension,1);
    file_selector.set_callback(file_selector_callback,this);

    file_save.set_extensions(&file_extension,1);
    file_save.set_callback(file_save_callback,this);

    update_current_memory_type();

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


int memory_viewer_t::input_text_callback(ImGuiInputTextCallbackData* data){
    if(data->EventFlag == ImGuiInputTextFlags_CallbackEdit){
        memory_viewer_t* memory_viewer = (memory_viewer_t*)data->UserData;
        memory_viewer->buffer_cursor_pos = data->CursorPos;
    }
    return 0;
}

void memory_viewer_t::file_selector_callback(void* userdata,std::filesystem::path path){
    memory_viewer_t* memory_viewer = (memory_viewer_t*)userdata;
    gb_memory_type_import(memory_viewer->gb,memory_viewer->current_memory_type,path.u8string().c_str());
}

void memory_viewer_t::file_save_callback(void* userdata,std::filesystem::path path){
    memory_viewer_t* memory_viewer = (memory_viewer_t*)userdata;
    gb_memory_type_export(memory_viewer->gb,memory_viewer->current_memory_type,path.u8string().c_str());
}


void memory_viewer_t::update_current_memory_type(){
    memory_length = gb_memory_type_length(gb,current_memory_type);

    address_digit_count = snprintf(nullptr,0,"%lX",memory_length - 1);

    editing_address = 0;
    
    need_focus_editing_address = true;

    update_scroll_y = true;

    update_rows();
}

void memory_viewer_t::update_rows(){
    rows = (int)ceilf((float)memory_length / (float)columns);
}


void memory_viewer_t::render_control(){

    if(ImGui::Button("Import")){
        file_selector.set_open(true);
    }

    ImGui::SameLine();

    if(ImGui::Button("Export")){
        file_save.set_open(true);
    }

    ImGui::SeparatorText("View Options");
    
    if(ImGui::BeginCombo("Memory Type",gb_memory_type_names[current_memory_type])){
        for(int i = 0; i < gb_memory_type_count; ++i){
            if(
                (i == gb_memory_rom_type && !gb->cartridge.rom_length) ||
                (i == gb_memory_ram_type && !gb->cartridge.ram_length)
            ){
                continue;
            }

            if(ImGui::Selectable(gb_memory_type_names[i],current_memory_type == i)){
                current_memory_type = i;
                update_current_memory_type();
            }
        }
        ImGui::EndCombo();
    }
    
    int columns_step = 1;

    if(ImGui::InputScalar("Columns",ImGuiDataType_S32,&columns,&columns_step,nullptr,"%d")){
        if(columns > memory_viewer_t::max_columns){
            columns = memory_viewer_t::max_columns;
        }
        else if(columns < memory_viewer_t::min_columns){
            columns = memory_viewer_t::min_columns;
        }
        update_rows();
    }

    ImU64 editing_address_step = 1;

    if(ImGui::InputScalar("Address",ImGuiDataType_U64,&editing_address,&editing_address_step,nullptr,"%llX")){
        if(editing_address >= memory_length){
            editing_address = memory_length - 1;
        }
        update_scroll_y = true;
    }

    ImGui::SeparatorText("Data Preview");

    uint8_t data = gb_memory_type_read_byte(gb,current_memory_type,editing_address);
    
    ImGui::Text("Hexadecimal: %02X",data);
    ImGui::Text("Signed interger: %hhd",data);
    ImGui::Text("Unsigned interger: %hhu",data);
    ImGui::Text("String: %c",data < 0x20 || data >= 0x7F ? '\0' : data);
}

void memory_viewer_t::render(){
    if(!open) return;

    if(!gb->cartridge_inserted){
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

            if(ImGui::BeginChild("MemoryViewerChild",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
                
                if(ImGui::IsKeyPressed(ImGuiKey_UpArrow) && editing_address >= columns){
                    editing_address -= columns;
                    update_scroll_y = true;
                }
                else if(ImGui::IsKeyPressed(ImGuiKey_DownArrow) && editing_address < (memory_length > columns ? memory_length - columns : 0)){
                    editing_address += columns;
                    update_scroll_y = true;
                }
                else if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && editing_address > 0){
                    --editing_address;
                    update_scroll_y = true;
                }
                else if(ImGui::IsKeyPressed(ImGuiKey_RightArrow) && editing_address < (memory_length - 1)){
                    ++editing_address;
                    update_scroll_y = true;
                }

                ImGuiStyle& style = ImGui::GetStyle();

                float glyph_width = ImGui::CalcTextSize("F").x;
                float hex_content_width = (glyph_width * (address_digit_count + 1) + style.ItemSpacing.x) + (glyph_width * 2.0f + style.ItemSpacing.x) * columns;
                
                ImVec2 cursor = ImGui::GetCursorScreenPos();

                float line_x =  cursor.x + hex_content_width;

                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(line_x,cursor.y),
                    ImVec2(line_x,cursor.y + (ImGui::GetTextLineHeightWithSpacing() * rows) - style.ItemSpacing.y),
                    line_color
                );

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0.0f,0.0f));

                ImGuiListClipper clipper;
                clipper.Begin(rows,ImGui::GetTextLineHeightWithSpacing());

                while(clipper.Step()){
                    for(int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row){

                        ImU64 start_address = row * columns;

                        ImVec2 cursor_start = ImGui::GetCursorPos();

                        ImGui::PushStyleColor(ImGuiCol_Text,text_disabled_color);

                        ImGui::Text("%0*llX:",address_digit_count,start_address);
                        
                        ImGui::PopStyleColor();

                        int column = 0;
                        ImU64 address = start_address;

                        while(column < columns && address < memory_length){
                            
                            ImGui::SameLine(0.0f,style.ItemSpacing.x);

                            if(address == editing_address){

                                ImGui::PushID((void*)address);

                                ImGui::SetNextItemWidth(glyph_width * 2.0f);

                                if(need_focus_editing_address){

                                    need_focus_editing_address = false;

                                    buffer_cursor_pos = 0;

                                    snprintf(buffer,sizeof(buffer),"%02X",gb_memory_type_read_byte(gb,current_memory_type,address));

                                    ImGui::SetKeyboardFocusHere();
                                }

                                ImGui::InputText("##TextInputData",buffer,sizeof(buffer),input_text_flags,input_text_callback,this);

                                if(buffer_cursor_pos >= 2 || ImGui::IsItemDeactivatedAfterEdit()){
                                    
                                    uint8_t new_value = strtol(buffer,nullptr,16);

                                    gb_memory_type_write_byte(gb,current_memory_type,new_value,editing_address);

                                    if(buffer_cursor_pos >= 2 && editing_address < (memory_length - 1)){
                                        ++editing_address;
                                        need_focus_editing_address = true;
                                    }
                                }
                                else if(!ImGui::IsItemActive()){
                                    snprintf(buffer,sizeof(buffer),"%02X",gb_memory_type_read_byte(gb,current_memory_type,address));
                                }
                                
                                ImGui::PopID();
                            }
                            else{
                                ImGui::Text("%02X",gb_memory_type_read_byte(gb,current_memory_type,address));

                                if(ImGui::IsItemHovered() && ImGui::GetIO().MouseReleased[0]){
                                    editing_address = address;
                                    need_focus_editing_address = true;
                                }
                            }

                            ++column;
                            ++address;
                        }

                        ImGui::SetCursorPos(ImVec2(cursor_start.x + hex_content_width + style.ItemSpacing.x,cursor_start.y));

                        column = 0;
                        address = start_address;

                        while(column < columns && address < memory_length){

                            if(column > 0){
                                ImGui::SameLine(0.0f,style.ItemSpacing.x);
                            }

                            uint8_t data = gb_memory_type_read_byte(gb,current_memory_type,address);

                            char c = (data < 0x20 || data >= 0x7F) ? '.' : data;

                            ImGui::PushStyleColor(ImGuiCol_Text,data == c ? text_color : text_disabled_color);

                            ImGui::TextUnformatted(&c,&c + 1);
                            
                            ImGui::PopStyleColor();

                            ++column;
                            ++address;
                        }
                    }
                }

                ImGui::PopStyleVar();

                if(update_scroll_y){

                    update_scroll_y = false;

                    float min_window_scroll_y = ImGui::GetScrollY();
                    float min_line_scroll_y = ImGui::GetTextLineHeightWithSpacing() * (editing_address / columns) + style.WindowPadding.y;

                    if(min_line_scroll_y < min_window_scroll_y){
                        ImGui::SetScrollY(min_window_scroll_y - (min_window_scroll_y - min_line_scroll_y));
                    }
                    else{
                        float max_window_scroll_y = min_window_scroll_y + ImGui::GetWindowHeight();
                        float max_line_scroll_y = min_line_scroll_y + ImGui::GetTextLineHeight();

                        if(max_line_scroll_y > max_window_scroll_y){
                            ImGui::SetScrollY(min_window_scroll_y + (max_line_scroll_y - max_window_scroll_y));
                        }
                    }
                }

            }
            ImGui::EndChild();

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