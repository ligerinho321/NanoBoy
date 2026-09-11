#include <gui/event_viewer/event_viewer.hpp>

static const uint32_t pc_crosshair_color = IM_COL32(255,204,0,255);
static const uint32_t pc_marker_color = IM_COL32(172,0,230,255);
static const uint32_t pc_marker_border_color = IM_COL32(86,0,115,255);
static const uint32_t mouse_crosshair_color = IM_COL32(255,255,255,255);


event_viewer_t::event_viewer_t(gb_t* gb,SDL_Renderer* renderer):gb(gb){
    texture = SDL_CreateTexture(
        renderer,
        event_viewer_t::texture_format,
        event_viewer_t::texture_access,
        event_viewer_t::texture_width,
        event_viewer_t::texture_height
    );

    event_selected_border_color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_NavCursor));

    update_event_control_child_size();
    update_event_list_column_width();
    update_texture_size();

    init_event_infos();
}

event_viewer_t::~event_viewer_t(){
    SDL_DestroyTexture(texture);
}


void event_viewer_t::init_event_infos(){

    event_infos[gb_event_vram_type] = "$8000-$9FFF";

    event_infos[gb_event_wram_type] = "$C000-$DFFF\n$E000-$FDFF";

    event_infos[gb_event_oam_type] = "$FE00-$FE9F";

    event_infos[gb_event_hram_type] = "$FF80-$FFFE";

    char buffer[256] = {0};

    snprintf(
        buffer,
        sizeof(buffer),
        "$FF00 (%s)",
        gb_registers_label[0x00]
    );

    event_infos[gb_event_joypad_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF01 (%s)\n"
        "$FF02 (%s)",
        gb_registers_label[0x01],
        gb_registers_label[0x02]
    );

    event_infos[gb_event_serial_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF04 (%s)\n"
        "$FF05 (%s)\n"
        "$FF06 (%s)\n"
        "$FF07 (%s)",
        gb_registers_label[0x04],
        gb_registers_label[0x05],
        gb_registers_label[0x06],
        gb_registers_label[0x07]
    );

    event_infos[gb_event_timer_type] = buffer;

    
    snprintf(
        buffer,
        sizeof(buffer),
        "$FF0F (%s)\n"
        "$FFFF (%s)",
        gb_registers_label[0x0F],
        gb_registers_label[0xFF]
    );

    event_infos[gb_event_interrupt_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF10 (%s)\n"
        "$FF11 (%s)\n"
        "$FF12 (%s)\n"
        "$FF13 (%s)\n"
        "$FF14 (%s)\n",
        gb_registers_label[0x10],
        gb_registers_label[0x11],
        gb_registers_label[0x12],
        gb_registers_label[0x13],
        gb_registers_label[0x14]
    );

    event_infos[gb_event_square1_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF16 (%s)\n"
        "$FF17 (%s)\n"
        "$FF18 (%s)\n"
        "$FF19 (%s)\n",
        gb_registers_label[0x16],
        gb_registers_label[0x17],
        gb_registers_label[0x18],
        gb_registers_label[0x19]
    );

    event_infos[gb_event_square2_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF1A (%s)\n"
        "$FF1B (%s)\n"
        "$FF1C (%s)\n"
        "$FF1D (%s)\n"
        "$FF1E (%s)\n",
        gb_registers_label[0x1A],
        gb_registers_label[0x1B],
        gb_registers_label[0x1C],
        gb_registers_label[0x1D],
        gb_registers_label[0x1E]
    );

    event_infos[gb_event_wave_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF20 (%s)\n"
        "$FF21 (%s)\n"
        "$FF22 (%s)\n"
        "$FF23 (%s)\n",
        gb_registers_label[0x20],
        gb_registers_label[0x21],
        gb_registers_label[0x22],
        gb_registers_label[0x23]
    );

    event_infos[gb_event_noise_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF24 (%s)\n"
        "$FF25 (%s)\n"
        "$FF26 (%s)\n",
        gb_registers_label[0x24],
        gb_registers_label[0x25],
        gb_registers_label[0x26]
    );

    event_infos[gb_event_apu_control_type] = buffer;


    snprintf(
        buffer,
        sizeof(buffer),
        "$FF30-$FF3F (%s)",
        gb_registers_label[0x30]
    );

    event_infos[gb_event_wave_ram_type] = buffer;

    snprintf(
        buffer,
        sizeof(buffer),
        "$FF40 (%s)\n"
        "$FF41 (%s)\n"
        "$FF42 (%s)\n"
        "$FF43 (%s)\n"
        "$FF44 (%s)\n"
        "$FF45 (%s)\n"
        "$FF4A (%s)\n"
        "$FF4B (%s)",
        gb_registers_label[0x40],
        gb_registers_label[0x41],
        gb_registers_label[0x42],
        gb_registers_label[0x43],
        gb_registers_label[0x44],
        gb_registers_label[0x45],
        gb_registers_label[0x4A],
        gb_registers_label[0x4B]
    );

    event_infos[gb_event_ppu_type] = buffer;

    snprintf(
        buffer,
        sizeof(buffer),
        "$FF47 (%s)\n"
        "$FF48 (%s)\n"
        "$FF49 (%s)\n"
        "$FF68 (%s)\n"
        "$FF69 (%s)\n"
        "$FF6A (%s)\n"
        "$FF6B (%s)",
        gb_registers_label[0x47],
        gb_registers_label[0x48],
        gb_registers_label[0x49],
        gb_registers_label[0x68],
        gb_registers_label[0x69],
        gb_registers_label[0x6A],
        gb_registers_label[0x6B]
    );

    event_infos[gb_event_palette_type] = buffer;

    snprintf(
        buffer,
        sizeof(buffer),
        "$FF46 (%s)\n"
        "$FF51 (%s)\n"
        "$FF52 (%s)\n"
        "$FF53 (%s)\n"
        "$FF54 (%s)\n"
        "$FF55 (%s)",
        gb_registers_label[0x46],
        gb_registers_label[0x51],
        gb_registers_label[0x52],
        gb_registers_label[0x53],
        gb_registers_label[0x54],
        gb_registers_label[0x55]
    );

    event_infos[gb_event_dma_type] = buffer;

    snprintf(
        buffer,
        sizeof(buffer),
        "$FF4C (%s)\n"
        "$FF4D (%s)\n"
        "$FF4F (%s)\n"
        "$FF50 (%s)\n"
        "$FF56 (%s)\n"
        "$FF6C (%s)\n"
        "$FF70 (%s)\n"
        "$FF72-$FF75 (%s)\n"
        "$FF76 (%s)\n"
        "$FF77 (%s)",
        gb_registers_label[0x4C],
        gb_registers_label[0x4D],
        gb_registers_label[0x4F],
        gb_registers_label[0x50],
        gb_registers_label[0x56],
        gb_registers_label[0x6C],
        gb_registers_label[0x70],
        gb_registers_label[0x72],
        gb_registers_label[0x76],
        gb_registers_label[0x77]
    );

    event_infos[gb_event_others_type] = buffer;
}


void event_viewer_t::update_event_control_child_size(){
    ImGuiStyle& style = ImGui::GetStyle();
    
    float label_text_max_width = 0.0f;

    for(int i = 0; i < gb_event_screen_color_count; ++i){
        float width = ImGui::CalcTextSize(gb_event_screen_color_names[i]).x;
        if(width > label_text_max_width){
            label_text_max_width = width;
        }
    }

    for(int i = 1; i < gb_event_type_count; ++i){
        float width = ImGui::CalcTextSize(gb_event_type_names[i]).x;
        if(width > label_text_max_width){
            label_text_max_width = width;
        }
    }

    float write_text_width = ImGui::CalcTextSize("Write").x;
    float read_text_width = ImGui::CalcTextSize("Read").x;

    //CheckBoxFlags + ColorEdit3
    float frames_width = ImGui::GetFrameHeight() * 2.0f + style.ItemInnerSpacing.x;

    float label_column_width = label_text_max_width + style.CellPadding.x * 2.0f;
    float write_column_width = frames_width + style.ItemInnerSpacing.x + write_text_width + style.CellPadding.x * 2.0f;
    float read_column_width = frames_width + style.ItemInnerSpacing.x + read_text_width + style.CellPadding.x * 2.0f;
    
    float table_max_width = label_column_width + write_column_width + read_column_width;

    event_control_child_size.x = table_max_width + style.WindowPadding.x * 2.0f + style.ScrollbarSize;
    event_control_child_size.y = 0.0f;

    event_control_label_column_width = label_text_max_width;
}

void event_viewer_t::update_event_list_column_width(){
    
    char buffer[64] = {};

    auto column_width = [](const char* header,const char* content){
        float header_width = ImGui::CalcTextSize(header).x;
        float content_width = ImGui::CalcTextSize(content).x;
        return header_width > content_width ? header_width : content_width;
    };
    
    event_list_column_width.color = ImGui::GetFrameHeight();

    snprintf(buffer,sizeof(buffer),"%d",gb_scanlines - 1);

    event_list_column_width.scanline = column_width("Scanline",buffer);

    snprintf(buffer,sizeof(buffer),"%d",gb_scanline_cycles - 1);

    event_list_column_width.cycle = column_width("Cycle",buffer);

    snprintf(buffer,sizeof(buffer),"$%04X",gb_bus_length - 1);

    event_list_column_width.pc = column_width("PC",buffer);

    snprintf(buffer,sizeof(buffer),"$%02X",0xFF);

    event_list_column_width.value = column_width("Value",buffer);

    event_list_column_width.previous_frame = ImGui::CalcTextSize("Previous Frame").x;
}


void event_viewer_t::update_texture(){
    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture,nullptr,(void**)&pixels,&pitch);

    memcpy(pixels,gb_event_manager_screen(gb),gb_event_screen_length);

    SDL_UnlockTexture(texture);
}

void event_viewer_t::update_texture_size(){
    texture_size.x = event_viewer_t::texture_width * scale;
    texture_size.y = event_viewer_t::texture_height * scale;
}


void event_viewer_t::push_frame_in_list(const gb_event_frame_t* frame,int start_cycle,int end_cycle){

    const gb_event_range_t* range = frame->range + start_cycle;
    const gb_event_range_t* range_end = frame->range + end_cycle;

    while(range < range_end){
        const gb_event_t* event = frame->event + range->start;
        const gb_event_t* event_end = event + range->count;

        while(event < event_end){
            event_viewer_t::config_t* config = event_configs + event->type;

            if((config->flags & event->flag) != 0){
                event_list.push_back(event);
            }

            ++event;
        }

        ++range;
    }
}

void event_viewer_t::update_event_list(){
    
    event_list.clear();

    int current_cycle = gb->ppu.state.scanline * gb_scanline_cycles + gb->ppu.state.cycle;

    const gb_event_frame_t* current_frame = gb_event_manager_current_frame(gb);
    push_frame_in_list(current_frame,0,current_cycle + 1);

    if(show_previous_frame_event){
        event_list_start_previous_frame = event_list.size();

        const gb_event_frame_t* previous_frame = gb_event_manager_previous_frame(gb);

        push_frame_in_list(previous_frame,current_cycle + 1,gb_frame_cycles);
    }
}

void event_viewer_t::update_event_list_scroll(){
    if(!_update_event_list_scroll) return;

    _update_event_list_scroll = false;

    ImGuiStyle& style = ImGui::GetStyle();
    
    float min_window_scroll = ImGui::GetScrollY();

    float header_height = ImGui::GetFontSize() + style.CellPadding.y * 2.0f;
    float item_height = event_list_item_height();

    int item_index = event_selected - event_list.data();
    float min_item_scroll = style.WindowPadding.y + header_height + (item_height * item_index);

    if(min_item_scroll < min_window_scroll){
        ImGui::SetScrollY(min_window_scroll - (min_window_scroll - min_item_scroll));
    }
    else{
        float max_window_scroll = min_window_scroll + ImGui::GetWindowHeight();
        float max_item_scroll = min_item_scroll + item_height;

        if(max_item_scroll > max_window_scroll){
            ImGui::SetScrollY(min_window_scroll + (max_item_scroll - max_window_scroll));
        }
    }
}


void event_viewer_t::event(){
    if(!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) return;

    if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && scale < max_scale){
        ++scale;
        update_texture_size();
    }
    else if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && scale > min_scale){
        --scale;
        update_texture_size();
    }

    ImGuiIO& io = ImGui::GetIO();
    if(io.KeyCtrl){
        if(io.MouseWheel > 0.0f && scale < max_scale){
            ++scale;
            update_texture_size();
        }
        else if(io.MouseWheel < 0.0f && scale > min_scale){
            --scale;
            update_texture_size();
        }
    }
}


void event_viewer_t::render_pc_marker(){
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 rect_min = ImGui::GetItemRectMin();
    ImVec2 rect_max = ImGui::GetItemRectMax();

    ImVec2 pos(
        rect_min.x + (gb->ppu.state.cycle * scale),
        rect_min.y + (gb->ppu.state.scanline * scale)
    );

    draw_list->AddRectFilled(ImVec2(rect_min.x,pos.y),ImVec2(rect_max.x,pos.y + scale),pc_crosshair_color);
    draw_list->AddRectFilled(ImVec2(pos.x,rect_min.y),ImVec2(pos.x + scale,rect_max.y),pc_crosshair_color);

    draw_list->AddRectFilled(ImVec2(pos.x - scale,pos.y - scale),ImVec2(pos.x + scale * 2.0f,pos.y + scale * 2.0f),pc_marker_border_color);
    draw_list->AddRectFilled(pos,ImVec2(pos.x + scale,pos.y + scale),pc_marker_color);
}


void event_viewer_t::render_mouse_marker(){

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 rect_min = ImGui::GetItemRectMin();
    ImVec2 rect_max = ImGui::GetItemRectMax();

    int x,y;

    if(event_hovered){
        x = event_hovered->cycle;
        y = event_hovered->scanline;
    }
    else{
        ImVec2 mouse = ImGui::GetMousePos();
    
        x = (mouse.x - rect_min.x) / scale;
        y = (mouse.y - rect_min.y) / scale;
    }

    ImVec2 pos(
        rect_min.x + (x * scale),
        rect_min.y + (y * scale)
    );

    draw_list->AddLine(ImVec2(rect_min.x,pos.y),ImVec2(rect_max.x,pos.y),mouse_crosshair_color);
    draw_list->AddLine(ImVec2(rect_min.x,pos.y + scale),ImVec2(rect_max.x,pos.y + scale),mouse_crosshair_color);
    draw_list->AddLine(ImVec2(pos.x,rect_min.y),ImVec2(pos.x,rect_max.y),mouse_crosshair_color);
    draw_list->AddLine(ImVec2(pos.x + scale,rect_min.y),ImVec2(pos.x + scale,rect_max.y),mouse_crosshair_color);

    if(event_hovered) return;

    if(!ImGui::BeginTooltip()) return;

    if(ImGui::BeginTable("MouseCoodinateTable",2)){
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Scanline");
        ImGui::TableNextColumn();
        ImGui::Text("%d",y);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Cycle");
        ImGui::TableNextColumn();
        ImGui::Text("%d",x);
        
        ImGui::EndTable();
    }
    
    ImGui::EndTooltip();
}


void event_viewer_t::render_event_hovered_tooltip(){

    if(!event_hovered || !ImGui::BeginTooltip()) return;

    if(ImGui::BeginTable("EventTooltipTable",2)){

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Scanline");
        ImGui::TableNextColumn();
        ImGui::Text("%d",event_hovered->scanline);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Cycle");
        ImGui::TableNextColumn();
        ImGui::Text("%d",event_hovered->cycle);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("PC");
        ImGui::TableNextColumn();
        ImGui::Text("$%04X",event_hovered->pc);
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Event");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(gb_event_type_names[event_hovered->type]);

        if(event_hovered->flag == gb_event_write_flag || event_hovered->flag == gb_event_read_flag){

            ImGui::SameLine(0.0f,0.0f);
            ImGui::Text(" %s",event_hovered->flag == gb_event_write_flag ? "Write" : "Read");
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Address");
            ImGui::TableNextColumn();
            ImGui::Text("$%04X",event_hovered->address);

            if(event_hovered->address >= 0xFF00 && gb_registers_label[event_hovered->address & 0xFF] != nullptr){
                ImGui::SameLine(0.0f,0.0f);
                ImGui::Text(" (%s)",gb_registers_label[event_hovered->address & 0xFF]);
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Value");
            ImGui::TableNextColumn();
            ImGui::Text("$%02X",event_hovered->value);
        }

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}


void event_viewer_t::render_event_markers(){

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    bool image_hovered = ImGui::IsItemHovered();
    
    ImVec2 mouse_pos = ImGui::GetMousePos();

    ImVec2 rect_min = ImGui::GetItemRectMin();

    event_hovered = nullptr;
    
    for(auto& event : event_list){

        event_viewer_t::config_t* config = event_configs + event->type;

        ImVec4 color;

        switch(event->flag){
            case gb_event_write_flag:     color = config->write_color;     break;
            case gb_event_read_flag:      color = config->read_color;      break;
            case gb_event_interrupt_flag: color = config->interrupt_color; break;
        }

        ImVec4 border_color(
            color.x * 0.5f,
            color.y * 0.5f,
            color.z * 0.5f,
            color.w
        );

        ImVec2 p_min(
            rect_min.x + event->cycle * scale,
            rect_min.y + event->scanline * scale
        );

        ImVec2 p_border_min(
            p_min.x - scale,
            p_min.y - scale
        );

        ImVec2 p_border_max(
            p_min.x + scale * 2.0f,
            p_min.y + scale * 2.0f
        );

        draw_list->AddRectFilled(p_border_min,p_border_max,ImGui::ColorConvertFloat4ToU32(border_color));

        if(image_hovered && mouse_in_rect(mouse_pos,p_border_min,p_border_max)){
            
            event_hovered = event;
            
            if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
                
                event_selected = &event;
                
                _update_event_list_scroll = true;

                border_on = true;

                last_border_blink_time = std::chrono::steady_clock::now();
            }
        }
    }

    for(auto& event : event_list){

        event_viewer_t::config_t* config = event_configs + event->type;

        ImVec4 color;

        switch(event->flag){
            case gb_event_write_flag:     color = config->write_color;     break;
            case gb_event_read_flag:      color = config->read_color;      break;
            case gb_event_interrupt_flag: color = config->interrupt_color; break;
        }

        ImVec2 p_min(
            rect_min.x + event->cycle * scale,
            rect_min.y + event->scanline * scale
        );

        ImVec2 p_max(
            p_min.x + scale,
            p_min.y + scale
        );

        draw_list->AddRectFilled(p_min,p_max,ImGui::ColorConvertFloat4ToU32(color));

        if(image_hovered && mouse_in_rect(mouse_pos,p_min,p_max)){
            
            event_hovered = event;
            
            if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
                
                event_selected = &event;
                
                _update_event_list_scroll = true;

                border_on = true;

                last_border_blink_time = std::chrono::steady_clock::now();
            }
        }

    }

    if(event_selected != nullptr){

        auto elapsed = std::chrono::steady_clock::now() - last_border_blink_time;

        if(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= 500){
            
            border_on = !border_on;

            last_border_blink_time = std::chrono::steady_clock::now();
        }

        if(border_on){
            ImVec2 p_min(
                rect_min.x + (*event_selected)->cycle * scale,
                rect_min.y + (*event_selected)->scanline * scale
            );

            ImVec2 p_max(
                p_min.x + scale,
                p_min.y + scale
            );

            ImVec2 p_border_min(
                p_min.x - scale,
                p_min.y - scale
            );

            ImVec2 p_border_max(
                p_min.x + scale * 2.0f,
                p_min.y + scale * 2.0f
            );

            draw_list->AddRect(p_border_min,p_border_max,event_selected_border_color,0.0f,0,2.0f);
            draw_list->AddRect(p_min,p_max,event_selected_border_color,0.0f,0,2.0f);
        }
    }

    if(event_hovered != nullptr){
        render_event_hovered_tooltip();
    }
}


void event_viewer_t::render_event_list(){

    ImGuiListClipper clipper;

    clipper.Begin(event_list.size(),event_list_item_height());

    while(clipper.Step()){
        for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i){

            const gb_event_t*& event = event_list[i];

            event_viewer_t::config_t* config = event_configs + event->type;
            
            ImGui::PushID(i);

            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);

            ImVec4 color;

            switch(event->flag){
                case gb_event_write_flag:     color = config->write_color; break;
                case gb_event_read_flag:      color = config->read_color; break;
                case gb_event_interrupt_flag: color = config->interrupt_color; break;
            }
            
            ImGui::ColorButton("##ColorButton",color,ImGuiColorEditFlags_NoTooltip);

            ImGui::TableSetColumnIndex(1);

            ImGui::Text("%d",event->scanline);

            ImGui::TableSetColumnIndex(2);

            ImGui::Text("%d",event->cycle);

            ImGui::TableSetColumnIndex(3);

            ImGui::Text("$%04X",event->pc);

            ImGui::TableSetColumnIndex(4);

            if(ImGui::Selectable(gb_event_type_names[event->type],&event == event_selected,ImGuiSelectableFlags_SpanAllColumns)){
                event_selected = &event;

                border_on = true;

                last_border_blink_time = std::chrono::steady_clock::now();
            }

            if(event->flag == gb_event_write_flag || event->flag == gb_event_read_flag){
                ImGui::SameLine(0.0f,0.0f);
                ImGui::Text(" %s",event->flag == gb_event_write_flag ? "Write" : "Read");

                ImGui::TableSetColumnIndex(5);

                ImGui::Text("$%04X",event->address);
                if(event->address >= 0xFF00 && gb_registers_label[event->address & 0xFF] != nullptr){
                    ImGui::SameLine(0.0f,0.0f);
                    ImGui::Text(" (%s)",gb_registers_label[event->address & 0xFF]);
                }

                ImGui::TableSetColumnIndex(6);

                ImGui::Text("$%02X",event->value);
            }

            if(show_previous_frame_event && i >= event_list_start_previous_frame){
                ImGui::TableSetColumnIndex(7);

                ImGui::TextUnformatted("Previous Frame");
            }

            ImGui::PopID();
        }
    }
}


void event_viewer_t::render_event_info(int type){
    std::string& info = event_infos[type];
    
    if(!info.length()) return;

    if(!ImGui::BeginTooltip()) return;

    ImGui::SeparatorText(gb_event_type_names[type]);

    ImGui::Text(info.c_str());

    ImGui::EndTooltip();
}


void event_viewer_t::render_interrupt_event_configs(){

    if(!ImGui::CollapsingHeader("Interrupt")) return;

    if(ImGui::BeginTable("InterruptTable",2)){

        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,event_control_label_column_width);

        for(int i = gb_event_halt_type; i <= gb_event_irq_joypad_type; ++i){
            ImGui::PushID(i);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            ImGui::AlignTextToFramePadding();

            ImGui::TextUnformatted(gb_event_type_names[i]);

            ImGui::TableNextColumn();

            if(ImGui::CheckboxFlags("##CheckBoxFlags",&event_configs[i].flags,gb_event_interrupt_flag)){
                if(event_selected != nullptr && (*event_selected)->type == i && !((*event_selected)->flag & event_configs[i].flags)){
                    event_selected = nullptr;
                }
            }

            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            ImGui::ColorEdit3("##ColorEdit",(float*)&event_configs[i].interrupt_color,ImGuiColorEditFlags_NoInputs);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void event_viewer_t::render_io_event_configs(const char* collapsing_header,const char* table,int start,int end){
    if(!ImGui::CollapsingHeader(collapsing_header)) return;

    if(ImGui::BeginTable(table,3)){

        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,event_control_label_column_width);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(0.0f,0.0f,0.0f,0.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImVec4(0.0f,0.0f,0.0f,0.0f));

        while(start <= end){
            ImGui::PushID(start);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            ImGui::AlignTextToFramePadding();

            ImGui::Selectable(gb_event_type_names[start],false,ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

            if(ImGui::IsItemHovered()){
                render_event_info(start);
            }

            ImGui::TableNextColumn();

            if(ImGui::CheckboxFlags("##CheckboxWrite",&event_configs[start].flags,gb_event_write_flag)){
                if(event_selected != nullptr && (*event_selected)->type == start && !((*event_selected)->flag & event_configs[start].flags)){
                    event_selected = nullptr;
                }     
            }

            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            ImGui::ColorEdit3("##ColorPickWrite",(float*)&event_configs[start].write_color,ImGuiColorEditFlags_NoInputs);

            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            ImGui::TextUnformatted("Write");


            ImGui::TableNextColumn();

            if(ImGui::CheckboxFlags("##CheckboxRead",&event_configs[start].flags,gb_event_read_flag)){
                if(event_selected != nullptr && (*event_selected)->type == start && !((*event_selected)->flag & event_configs[start].flags)){
                    event_selected = nullptr;
                }
            }

            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            ImGui::ColorEdit3("##ColorPickRead",(float*)&event_configs[start].read_color,ImGuiColorEditFlags_NoInputs);

            ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

            ImGui::TextUnformatted("Read");


            ImGui::PopID();

            ++start;
        }

        ImGui::PopStyleColor(2);

        ImGui::EndTable();
    }
}


void event_viewer_t::render(){
    if(!open) return;

    if(!gb->cartridge_inserted){
        set_open(false);
        return;
    }

    if(!gb->paused){
        event_selected = nullptr;
    }

    update_texture();
    update_event_list();

    bool _open = open;

    if(ImGui::Begin("Event Viewer",&_open)){

        if(ImGui::BeginTable("EventViewerTable",2,ImGuiTableFlags_PreciseWidths)){
            
            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            float height = (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 0.7f;

            if(ImGui::BeginChild("EventScreenChild",ImVec2(0.0f,height),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
                
                ImGui::Image((ImTextureRef)texture,texture_size);

                ImGui::PushClipRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),true);

                if(gb->paused){
                    render_pc_marker();
                }

                render_event_markers();

                if(ImGui::IsItemHovered()){
                    render_mouse_marker();
                }

                ImGui::PopClipRect();
            }
            ImGui::EndChild();

            
            if(ImGui::BeginChild("EventListChild",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders)){
                
                if(ImGui::BeginTable("EventListTable",8,ImGuiTableFlags_Borders)){
                    
                    ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,event_list_column_width.color);
                    ImGui::TableSetupColumn("Scanline",ImGuiTableColumnFlags_WidthFixed,event_list_column_width.scanline);
                    ImGui::TableSetupColumn("Cycle",ImGuiTableColumnFlags_WidthFixed,event_list_column_width.cycle);
                    ImGui::TableSetupColumn("PC",ImGuiTableColumnFlags_WidthFixed,event_list_column_width.pc);
                    ImGui::TableSetupColumn("Event");
                    ImGui::TableSetupColumn("Address");
                    ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthFixed,event_list_column_width.value);
                    ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,event_list_column_width.previous_frame);

                    ImGui::TableHeadersRow();

                    render_event_list();

                    ImGui::EndTable();
                }

                update_event_list_scroll();
            }
            ImGui::EndChild();


            ImGui::TableNextColumn();

            if(ImGui::BeginChild("ControlChild",event_control_child_size,ImGuiChildFlags_Borders)){
                

                if(ImGui::CollapsingHeader("Screen Palette")){
                    
                    if(ImGui::BeginTable("ScreenPaletteTable",2)){

                        ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,event_control_label_column_width);

                        for(int i = 0; i < gb_event_screen_color_count; ++i){
                            ImGui::PushID(i);
                            
                            ImGui::TableNextRow();
                            
                            ImGui::TableNextColumn();
                            
                            ImGui::TextUnformatted(gb_event_screen_color_names[i]);

                            ImGui::TableNextColumn();

                            gb_rgb_t color = gb_event_screen_palette[i];

                            ImVec4 color_converted(
                                color.r / 255.0f,
                                color.g / 255.0f,
                                color.b / 255.0f,
                                1.0f
                            );

                            ImGui::ColorButton("##ColorButton",color_converted);
                            
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }
                }

                render_interrupt_event_configs();

                render_io_event_configs("Memory","MemoryTable",gb_event_vram_type,gb_event_hram_type);

                render_io_event_configs("Registers","RegistersTable",gb_event_joypad_type,gb_event_others_type);

                ImGui::Checkbox("Show previous frame events",&show_previous_frame_event);

                if(ImGui::Button("Select All")){
                    for(int i = gb_event_halt_type; i <= gb_event_irq_joypad_type; ++i){
                        event_configs[i].flags = gb_event_interrupt_flag;
                    }
                    for(int i = gb_event_vram_type; i <= gb_event_others_type; ++i){
                        event_configs[i].flags = gb_event_write_flag | gb_event_read_flag;
                    }
                }

                ImGui::SameLine();

                if(ImGui::Button("Deselect All")){
                    for(int i = gb_event_none_type; i < gb_event_type_count; ++i){
                        event_configs[i].flags = 0;
                    }
                    event_selected = nullptr;
                }

            }
            ImGui::EndChild();

            ImGui::EndTable();
        }

        event();
    }
    ImGui::End();

    set_open(_open);
}


void event_viewer_t::set_open(bool _open) noexcept {
    if(open == _open) return;

    open = _open;

    if(!open) clear();

    gb_event_manager_enable(gb,open);
}