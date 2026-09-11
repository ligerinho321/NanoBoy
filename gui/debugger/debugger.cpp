#include <gui/debugger/debugger.hpp>

static const char* breakpoint_popup_names[] = {
    "Add Breakpoint",
    "Edit Breakpoint"
};

debugger_t::debugger_t(gb_t* gb):gb(gb){
    update_breakpoint_popup_size_constraints();
}

debugger_t::~debugger_t(){
    gb_breakpoint_manager_clear(gb);

    for(auto breakpoint : breakpoints){
        delete breakpoint;
    }

    breakpoint_selected = nullptr;
    
    breakpoints.clear();
}


void debugger_t::add_breakpoint(){
    gb_breakpoint_t* breakpoint = new gb_breakpoint_t(temp_breakpoint);

    gb_breakpoint_manager_add(gb,breakpoint);

    breakpoints.push_back(breakpoint);

    breakpoint_selected = breakpoint;
}

void debugger_t::edit_breakpoint(){
    breakpoint_selected->enabled = temp_breakpoint.enabled;
    breakpoint_selected->memory_type = temp_breakpoint.memory_type;
    breakpoint_selected->address = temp_breakpoint.address;
}

void debugger_t::delete_breakpoint(){

    gb_breakpoint_manager_remove(gb,breakpoint_selected);

    breakpoints.remove(breakpoint_selected);
    
    delete breakpoint_selected;

    breakpoint_selected = nullptr;
}


void debugger_t::update_breakpoint_popup_size_constraints(){
    ImGuiStyle& style = ImGui::GetStyle();

    float title_bar_height = ImGui::GetFrameHeight();

    //MemoryTypeCombo + AddressScalarInput + (EnabledCheckbox + OkButton + CancelButton);
    float content_height = ImGui::GetFrameHeightWithSpacing() * 3.0f;

    float breakpoint_popup_height =  title_bar_height + content_height + style.WindowPadding.y * 2.0f;

    breakpoint_popup_min_size.x = style.WindowMinSize.x;
    breakpoint_popup_min_size.y = breakpoint_popup_height;

    breakpoint_popup_max_size.x = FLT_MAX;
    breakpoint_popup_max_size.y = breakpoint_popup_height;
}

void debugger_t::update_breakpoint_max_address(){
    breakpoint_max_address = gb_memory_type_length(gb,temp_breakpoint.memory_type);

    if(breakpoint_max_address > 0){
        --breakpoint_max_address;
    }

    snprintf(breakpoint_max_address_text,sizeof(breakpoint_max_address_text),"(Max: $%lX)",breakpoint_max_address);

    breakpoint_max_address_text_width = ImGui::CalcTextSize(breakpoint_max_address_text).x;
}

void debugger_t::open_breakpoint_popup(uint8_t type){
    breakpoint_popup_type = type;
    request_open_breakpoint_popup = true;

    if(type == breakpoint_popup_type_t::breakpoint_popup_add_type){
        temp_breakpoint.enabled = false;
        temp_breakpoint.memory_type = gb_memory_cpu_type;
        temp_breakpoint.address = 0;
    }
    else{
        temp_breakpoint.enabled = breakpoint_selected->enabled;
        temp_breakpoint.memory_type = breakpoint_selected->memory_type;
        temp_breakpoint.address = breakpoint_selected->address;
    }
}

void debugger_t::render_breakpoint_popup(){
    if(request_open_breakpoint_popup){
        request_open_breakpoint_popup = false;

        ImGui::OpenPopup(breakpoint_popup_names[breakpoint_popup_type]);
        
        breakpoint_popup_open = true;

        update_breakpoint_max_address();

        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();

        breakpoint_popup_start_pos.x = window_pos.x + window_size.x * 0.5f;
        breakpoint_popup_start_pos.y = window_pos.y + window_size.y * 0.5f;
    }

    if(!breakpoint_popup_open) return;


    ImGui::SetNextWindowSizeConstraints(breakpoint_popup_min_size,breakpoint_popup_max_size);

    ImGui::SetNextWindowPos(breakpoint_popup_start_pos,ImGuiCond_Appearing,ImVec2(0.5f,0.5f));


    if(!ImGui::BeginPopupModal(breakpoint_popup_names[breakpoint_popup_type],&breakpoint_popup_open)) return;


    ImGuiStyle& style = ImGui::GetStyle();


    ImGui::AlignTextToFramePadding();

    ImGui::TextUnformatted("Memory Type:");

    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if(ImGui::BeginCombo("##MemoryTypeCombo",gb_memory_type_names[temp_breakpoint.memory_type])){
        for(int i = 0; i < gb_memory_type_count; ++i){
            if(
                (i == gb_memory_rom_type && !gb_memory_type_length(gb,gb_memory_rom_type)) ||
                (i == gb_memory_ram_type && !gb_memory_type_length(gb,gb_memory_ram_type))
            ){
                continue;
            }

            if(ImGui::Selectable(gb_memory_type_names[i],temp_breakpoint.memory_type == i) && temp_breakpoint.memory_type != i){
                temp_breakpoint.memory_type = i;
                
                update_breakpoint_max_address();

                if(temp_breakpoint.address > breakpoint_max_address){
                    temp_breakpoint.address = breakpoint_max_address;
                }
            }
        }
        ImGui::EndCombo();
    }


    ImGui::AlignTextToFramePadding();
    
    ImGui::TextUnformatted("Address:");
    
    ImGui::SameLine(0.0f,style.ItemInnerSpacing.x);

    float input_scalar_width = ImGui::GetContentRegionAvail().x - breakpoint_max_address_text_width - style.ItemSpacing.x;

    ImGui::SetNextItemWidth(input_scalar_width);

    if(ImGui::InputScalar("##AddressInputScalar",ImGuiDataType_U64,&temp_breakpoint.address,nullptr,nullptr,"%lX",ImGuiInputTextFlags_CharsHexadecimal)){
        if(temp_breakpoint.address > breakpoint_max_address){
            temp_breakpoint.address = breakpoint_max_address;
        }
    }
    
    ImGui::SameLine();

    ImGui::TextUnformatted(breakpoint_max_address_text);


    ImGui::Checkbox("Enabled",&temp_breakpoint.enabled);

    ImGui::SameLine();

    const char* ok = "Ok";
    const char* cancel = "Cancel";

    float ok_button_width = ImGui::CalcTextSize(ok).x + style.FramePadding.x * 2.0f;
    float cancel_button_width = ImGui::CalcTextSize(cancel).x + style.FramePadding.x * 2.0f;

    ImGui::SetCursorPosX((ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x) - (ok_button_width + cancel_button_width + style.ItemSpacing.x));

    if(ImGui::Button(ok)){
        if(breakpoint_popup_type == breakpoint_popup_type_t::breakpoint_popup_add_type){
            add_breakpoint();
        }
        else{
            edit_breakpoint();
        }
        breakpoint_popup_open = false;
    }

    ImGui::SameLine();

    if(ImGui::Button(cancel)){
        breakpoint_popup_open = false;
    }

    ImGui::EndPopup();
}

void debugger_t::render_breakpoints(){
    ImGui::SeparatorText("Breakpoints");

    if(ImGui::Button("Add")) open_breakpoint_popup(breakpoint_popup_type_t::breakpoint_popup_add_type);

    ImGui::SameLine();

    ImGui::BeginDisabled(breakpoint_selected == nullptr);

    if(ImGui::Button("Edit")) open_breakpoint_popup(breakpoint_popup_type_t::breakpoint_popup_edit_type);

    ImGui::SameLine();

    if(ImGui::Button("Delete")){
        delete_breakpoint();
    }

    ImGui::EndDisabled();

    if(ImGui::BeginTable("BreakpointTable",3,ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders,ImGui::GetContentRegionAvail())){
        
        ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("Memory Type",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Address",ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        int id = 0;
        for(auto breakpoint : breakpoints){
            ImGui::PushID(id++);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            ImGui::Checkbox("##CheckBox",&breakpoint->enabled);

            ImGui::TableNextColumn();

            if(ImGui::Selectable(gb_memory_type_names[breakpoint->memory_type],breakpoint_selected == breakpoint,ImGuiSelectableFlags_SpanAllColumns)){
                breakpoint_selected = breakpoint;
            }

            ImGui::TableNextColumn();

            ImGui::Text("$%lX",breakpoint->address);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    render_breakpoint_popup();
}


void debugger_t::render_registers(){

    ImGui::SeparatorText("Registers");

    ImGuiStyle& style = ImGui::GetStyle();

    const char* input_format_u8 = "%02" PRIX8;
    const char* input_format_u16 = "%04" PRIX16;

    float input_u8_width = ImGui::CalcTextSize("00").x + style.FramePadding.x * 2.0f;
    float input_u16_width = ImGui::CalcTextSize("0000").x + style.FramePadding.x * 2.0f;

    ImGuiInputFlags input_flags = ImGuiInputTextFlags_CharsHexadecimal;

    ImGui::BeginDisabled(!gb->paused);

    ImGui::PushID("Registers");

    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("A",ImGuiDataType_U8,&gb->cpu.state.a,nullptr,nullptr,input_format_u8,input_flags);

    ImGui::SameLine();

    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("F",ImGuiDataType_U8,&gb->cpu.state.f,nullptr,nullptr,input_format_u8,input_flags);

    ImGui::SameLine();

    ImGui::Text("($%04" PRIX16 ")",gb->cpu.state.af);


    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("B",ImGuiDataType_U8,&gb->cpu.state.b,nullptr,nullptr,input_format_u8,input_flags);

    ImGui::SameLine();

    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("C",ImGuiDataType_U8,&gb->cpu.state.c,nullptr,nullptr,input_format_u8,input_flags);

    ImGui::SameLine();

    ImGui::Text("($%04" PRIX16 ")",gb->cpu.state.bc);


    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("D",ImGuiDataType_U8,&gb->cpu.state.d,nullptr,nullptr,input_format_u8,input_flags);

    ImGui::SameLine();

    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("E",ImGuiDataType_U8,&gb->cpu.state.e,nullptr,nullptr,input_format_u8,input_flags);

    ImGui::SameLine();

    ImGui::Text("($%04" PRIX16 ")",gb->cpu.state.de);


    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("H",ImGuiDataType_U8,&gb->cpu.state.h,nullptr,nullptr,input_format_u8,input_flags);
    
    ImGui::SameLine();

    ImGui::SetNextItemWidth(input_u8_width);
    ImGui::InputScalar("L",ImGuiDataType_U8,&gb->cpu.state.l,nullptr,nullptr,input_format_u8,input_flags);

    ImGui::SameLine();

    ImGui::Text("($%04" PRIX16 ")",gb->cpu.state.hl);


    ImGui::SetNextItemWidth(input_u16_width);
    ImGui::InputScalar("SP",ImGuiDataType_U16,&gb->cpu.state.sp,nullptr,nullptr,input_format_u16,input_flags);

    ImGui::SetNextItemWidth(input_u16_width);
    ImGui::InputScalar("PC",ImGuiDataType_U16,&gb->cpu.state.pc,nullptr,nullptr,input_format_u16,input_flags);

    ImGui::PopID();

    ImGui::EndDisabled();
}

void debugger_t::render_flags(){
    ImGui::SeparatorText("Flags");

    bool z = gb->cpu.state.f & gb_cpu_zero_flag;
    bool n = gb->cpu.state.f & gb_cpu_subtraction_flag;
    bool h = gb->cpu.state.f & gb_cpu_half_carry_flag;
    bool c = gb->cpu.state.f & gb_cpu_carry_flag;

    ImGui::BeginDisabled(!gb->paused);

    ImGui::PushID("Flags");

    if(ImGui::Checkbox("Z",&z)){
        z ? (gb->cpu.state.f |= gb_cpu_zero_flag) : (gb->cpu.state.f &= ~gb_cpu_zero_flag);
    }
    
    ImGui::SameLine();

    if(ImGui::Checkbox("N",&n)){
        n ? (gb->cpu.state.f |= gb_cpu_subtraction_flag) : (gb->cpu.state.f &= ~gb_cpu_subtraction_flag);
    }

    ImGui::SameLine();

    if(ImGui::Checkbox("H",&h)){
        h ? (gb->cpu.state.f |= gb_cpu_half_carry_flag) : (gb->cpu.state.f &= ~gb_cpu_half_carry_flag);
    }

    ImGui::SameLine();

    if(ImGui::Checkbox("C",&c)){
        c ? (gb->cpu.state.f |= gb_cpu_carry_flag) : (gb->cpu.state.f &= ~gb_cpu_carry_flag);
    }

    ImGui::PopID();

    ImGui::EndDisabled();
}

void debugger_t::render_others(){
    ImGui::SeparatorText("Others");

    ImGui::BeginDisabled(!gb->paused);

    if(ImGui::BeginCombo("State",gb_cpu_mode_names[gb->cpu.state.mode])){
        
        for(int i = 0; i < gb_cpu_mode_count; ++i){

            if(ImGui::Selectable(gb_cpu_mode_names[i],gb->cpu.state.mode == i)){
                gb_cpu_set_mode(&gb->cpu,i);
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Checkbox("IME",&gb->cpu.state.ime);

    ImGui::EndDisabled();
}

void debugger_t::render_disassembly(){

    ImGui::SeparatorText("Disassembly");

    char disassembler[256] = {0};

    uint16_t pc = gb->cpu.state.pc;
    
    for(int i = 0; i < 15; ++i){
        pc += gb_disassembler_disassemble(gb,pc,disassembler,sizeof(disassembler));

        ImGui::TextUnformatted(disassembler);
    }
}

void debugger_t::render(){

    if(!open) return;

    if(!gb->cartridge_inserted){
        set_open(false);
        return;
    }

    bool _open = open;

    if(ImGui::Begin("Debugger",&_open)){

        ImGui::BeginDisabled(!gb->paused);
        
        if(ImGui::Button("Run")) gb_pause(gb,false);

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(gb->paused);

        if(ImGui::Button("Pause")) gb_pause(gb,true);

        ImGui::EndDisabled();

        ImGui::SameLine();

        if(ImGui::Button("Step")) gb_execute_step(gb);

        if(ImGui::BeginTable("DebuggerTable",2)){

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            render_registers();

            render_flags();

            render_others();

            ImGui::TableNextColumn();

            render_disassembly();
            
            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Game Boy Timing");
            
            ImGui::Text("T-Cycle: %" PRIu64,gb->state.cycle);
            ImGui::Text("M-Cycle: %" PRIu64,gb->state.cycle / 4);

            ImGui::TableNextColumn();

            ImGui::SeparatorText("PPU Timing");

            ImGui::Text("Scanline: %" PRIu8,gb->ppu.state.scanline);
            ImGui::Text("Cycle: %" PRIu16,gb->ppu.state.cycle);

            ImGui::EndTable();
        }

        render_breakpoints();
    }
    ImGui::End();

    set_open(_open);
}


void debugger_t::set_open(bool _open) noexcept {
    if(_open == open) return;

    open = _open;

    if(open){
        gb_breakpoint_manager_enable(gb,true);

        gb_pause(gb,true);
    }
    else{
        gb_breakpoint_manager_enable(gb,false);
    }
}