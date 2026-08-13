#include <gui/tile_viewer/tile_viewer.hpp>

static const char* sources[] = {
    "CPU Bus",
    "Cartridge ROM",
    "Video RAM",
    "Cartridge RAM",
    "Work RAM",
    "High RAM"
};

static const int sources_count = sizeof(sources) / sizeof(sources[0]);


static const char* layouts[] = {
    "8x8",
    "8x16",
    "16x16"
};

static const int layouts_count = sizeof(layouts) / sizeof(layouts[0]);


tile_viewer_t::tile_viewer_t(gb_t* gb,SDL_Renderer* renderer):gb(gb),bg_palette(renderer),obj_palette(renderer){
    texture = SDL_CreateTexture(renderer,tile_viewer_t::texture_format,tile_viewer_t::texture_access,tile_viewer_t::texture_max_size,tile_viewer_t::texture_max_size);

    input_scalar_width = get_input_scalar_width(8);

    ImGuiStyle& style = ImGui::GetStyle();

    grid_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,0.5f));
    border_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);

    update_texture_size();
    update_texture_uv();
    update_data_length();
}

tile_viewer_t::~tile_viewer_t(){
    gb_thread_safe_remove_ppu_handler(gb,&callback_handler);

    SDL_DestroyTexture(texture);
}


void tile_viewer_t::ppu_callback(void* userdata){
    tile_viewer_t* tile_viewer = (tile_viewer_t*)userdata;
    gb_t* gb = tile_viewer->gb;

    tile_viewer->cgb_mode = gb->cgb_mode;

    tile_viewer->bg_palette.update_data(&gb->palette);
    tile_viewer->obj_palette.update_data(&gb->palette);

    uint8_t* dst = tile_viewer->data.data();
    uint32_t address = tile_viewer->address_offset;
    uint32_t len = tile_viewer->data_length;

    switch(tile_viewer->current_source){
        case tile_viewer_t::source_cpu:{
            gb_memory_t* memory = &gb->memory;
            while(len--){
                *dst++ = gb_memory_cpu_read(memory,address++ % gb_bus_length);
            }
            break;
        }
        case tile_viewer_t::source_rom:{
            gb_cartridge_t* cartridge = &gb->cartridge;
            while(len--){
                *dst++ = cartridge->rom[address++ % gb->cartridge.rom_size];
            }
            break;
        }
        case tile_viewer_t::source_vram:{
            gb_ppu_t* ppu = &gb->ppu;
            while(len--){
                *dst++ = ppu->vram[address++ % gb_vram_length];
            }
            break;
        }
        case tile_viewer_t::source_ram:{
            gb_cartridge_t* cartridge = &gb->cartridge;
            while(len--){
                *dst++ = cartridge->ram[address++ % gb->cartridge.ram_size];
            }
            break;
        }
        case tile_viewer_t::source_wram:{
            gb_memory_t* memory = &gb->memory;
            while(len--){
                *dst++ = memory->wram[address++ % gb_wram_length];
            }
            break;
        }
        case tile_viewer_t::source_hram:{
            gb_memory_t* memory = &gb->memory;
            while(len--){
                *dst++ = memory->hram[address++ % gb_hram_length];
            }
            break;
        }
    }
}


static const uint8_t bit_mask[8] = {
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01
};

void tile_viewer_t::render_layout8x8(uint8_t* pixels,int pitch,const gb_rgb_t* colors) noexcept {

    uint32_t address = 0;

    for(int row = 0; row < rows; ++row){
        for(int col = 0; col < columns; ++col){

            for(uint8_t y = 0; y < 8; ++y){

                uint8_t lo = data[address++ % data_length];
                uint8_t hi = data[address++ % data_length];

                for(uint8_t x = 0; x < 8; ++x){

                    uint8_t color_index = ((hi & bit_mask[x]) ? 0x02 : 0x00) | ((lo & bit_mask[x]) ? 0x01 : 0x00);

                    gb_rgb_t color = colors[color_index];

                    uint8_t* pixel = pixels + ((row * 8 + y) * pitch) + ((col * 8 + x) * tile_viewer_t::texture_bytes_per_pixel);
                    pixel[0] = color.r;
                    pixel[1] = color.g;
                    pixel[2] = color.b;
                }
            }
        }
    }
}

void tile_viewer_t::render_layout8x16(uint8_t* pixels,int pitch,const gb_rgb_t* colors) noexcept {

    int _rows = rows / 2;

    uint32_t address = 0;

    for(int row = 0; row < _rows; ++row){
        for(int col = 0; col < columns; ++col){

            for(uint8_t y = 0; y < 16; ++y){

                uint8_t lo = data[address++ % data_length];
                uint8_t hi = data[address++ % data_length];

                for(uint8_t x = 0; x < 8; ++x){

                    uint8_t color_index = ((hi & bit_mask[x]) ? 0x02 : 0x00) | ((lo & bit_mask[x]) ? 0x01 : 0x00);

                    gb_rgb_t color = colors[color_index];

                    uint8_t* pixel = pixels + ((row * 16 + y) * pitch) + ((col * 8 + x) * tile_viewer_t::texture_bytes_per_pixel);
                    pixel[0] = color.r;
                    pixel[1] = color.g;
                    pixel[2] = color.b;
                }
            }
        }
    }
}

void tile_viewer_t::render_layout16x16(uint8_t* pixels,int pitch,const gb_rgb_t* colors) noexcept {

    int _rows = rows / 2;
    int _columns = columns / 2;

    uint32_t address = 0;

    for(int row = 0; row < _rows; ++row){
        for(int col = 0; col < _columns; ++col){

            for(uint8_t tile_y = 0; tile_y < 2; ++tile_y){
                for(uint8_t tile_x = 0; tile_x < 2; ++tile_x){

                    for(uint8_t y = 0; y < 8; ++y){

                        uint8_t lo = data[address++ % data_length];
                        uint8_t hi = data[address++ % data_length];

                        for(uint8_t x = 0; x < 8; ++x){

                            uint8_t color_index = ((hi & bit_mask[x]) ? 0x02 : 0x00) | ((lo & bit_mask[x]) ? 0x01 : 0x00);

                            gb_rgb_t color = colors[color_index];

                            uint8_t* pixel = pixels + ((row * 16 + tile_y * 8 + y) * pitch) + ((col * 16 + tile_x * 8 + x) * tile_viewer_t::texture_bytes_per_pixel);
                            pixel[0] = color.r;
                            pixel[1] = color.g;
                            pixel[2] = color.b;
                        }
                    }
                }
            }
        }
    }
}

void tile_viewer_t::update_texture() noexcept {
    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_Rect rect{0,0,columns * gb_tile_size,rows * gb_tile_size};

    gb_rgb_t colors[4];

    palette_t& palette = obj_palette_selected ? (palette_t&)obj_palette : (palette_t&)bg_palette;

    if(gb->is_cgb){
        if(cgb_mode){
            for(uint8_t i = 0; i < 4; ++i){
                colors[i] = palette.get_cgb_color(palette_index,i);
            }
        }
        else{
            for(uint8_t i = 0; i < 4; ++i){
                colors[i] = palette.get_cgb_dmg_color(palette_index,i);
            }
        }
    }
    else{
        for(uint8_t i = 0; i < 4; ++i){
            colors[i] = palette.get_dmg_color(palette_index,i);
        }
    }

    SDL_LockTexture(texture,&rect,(void**)&pixels,&pitch);

    switch(current_layout){
        case tile_viewer_t::layout_8x8: render_layout8x8(pixels,pitch,colors); break;
        case tile_viewer_t::layout_8x16: render_layout8x16(pixels,pitch,colors); break;
        case tile_viewer_t::layout_16x16: render_layout16x16(pixels,pitch,colors); break;
    }

    SDL_UnlockTexture(texture);
}


void tile_viewer_t::update_texture_size() noexcept {
    texture_size.x = columns * gb_tile_size * scale;
    texture_size.y = rows * gb_tile_size * scale;
}

void tile_viewer_t::update_texture_uv() noexcept {
    texture_uv0.x = 0.0f;
    texture_uv0.y = 0.0f;

    texture_uv1.x = (float)(columns * gb_tile_size) / (float)tile_viewer_t::texture_max_size;
    texture_uv1.y = (float)(rows * gb_tile_size) / (float)tile_viewer_t::texture_max_size;
}

void tile_viewer_t::update_data_length() noexcept {

    uint32_t data_max_length = columns * rows * 0x10;

    switch(current_source){
        case tile_viewer_t::source_cpu:{
            data_length = gb_min(gb_bus_length,data_max_length);
            break;
        }
        case tile_viewer_t::source_rom:{
            data_length = gb_min(gb->cartridge.rom_size,data_max_length);
            break;
        }
        case tile_viewer_t::source_vram:{
            data_length = gb_min(gb_vram_length,data_max_length);
            break;
        }
        case tile_viewer_t::source_ram:{
            data_length = gb_min(gb->cartridge.ram_size,data_max_length);
            break;
        }
        case tile_viewer_t::source_wram:{
            data_length = gb_min(gb_wram_length,data_max_length);
            break;
        }
        case tile_viewer_t::source_hram:{
            data_length = gb_min(gb_hram_length,data_max_length);
            break;
        }
    }
}

void tile_viewer_t::update_address_offset() noexcept {
    switch(current_source){
        case tile_viewer_t::source_cpu:{
            if(address_offset >= gb_bus_length){
                address_offset = gb_bus_length - 1;
            }
            break;
        }
        case tile_viewer_t::source_rom:{
            if(address_offset >= gb->cartridge.rom_size){
                address_offset = gb->cartridge.rom_size - 1;
            }
            break;
        }
        case tile_viewer_t::source_vram:{
            if(address_offset >= gb_vram_length){
                address_offset = gb_vram_length - 1;
            }
            break;
        }
        case tile_viewer_t::source_ram:{
            if(address_offset >= gb->cartridge.ram_size){
                address_offset = gb->cartridge.ram_size - 1;
            }
            break;
        }
        case tile_viewer_t::source_wram:{
            if(address_offset >= gb_wram_length){
                address_offset = gb_wram_length - 1;
            }
            break;
        }
        case tile_viewer_t::source_hram:{
            if(address_offset >= gb_hram_length){
                address_offset = gb_hram_length - 1;
            }
        }
    }
}


void tile_viewer_t::event(){
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


void tile_viewer_t::render_palette(){

    if(!ImGui::BeginTable("PaletteTable",2)) return;

    ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

    bg_palette.update_texture(gb->is_cgb,cgb_mode);
    obj_palette.update_texture(gb->is_cgb,cgb_mode);

    float height = gb_tile_size * tile_viewer_t::palette_scale;

    ImGui::TableNextRow();

    ImGui::TableNextColumn();

    ImVec2 bg_size(
        palette_t::texture_max_width,
        cgb_mode ? palette_t::cgb_texture_height : palette_t::dmg_bg_texture_height
    );

    ImVec2 bg_uv0(
        0.0f,
        0.0f
    );

    ImVec2 bg_uv1(
        1.0f,
        bg_size.y / palette_t::texture_max_height  
    );

    bg_size.x *= tile_viewer_t::palette_scale;
    bg_size.y *= tile_viewer_t::palette_scale;

    ImGui::Image((ImTextureRef)bg_palette.texture,bg_size,bg_uv0,bg_uv1);

    if(ImGui::IsItemClicked(ImGuiMouseButton_Left)){
        obj_palette_selected = false;
        palette_index = (ImGui::GetMousePos().y - ImGui::GetItemRectMin().y) / height;
    }

    if(!obj_palette_selected){
        uint8_t index = palette_index % (cgb_mode ? gb_cgb_palettes : gb_dmg_bg_palettes);

        ImVec2 start = ImGui::GetItemRectMin();

        ImVec2 p_min(
            start.x,
            start.y + index * height
        );

        ImVec2 p_max(
            p_min.x + bg_size.x,
            p_min.y + height
        );

        ImGui::GetWindowDrawList()->AddRect(p_min,p_max,border_hovered_color,0.0f,0,2.0f);
    }


    ImGui::TableNextColumn();

    ImVec2 obj_size(
        palette_t::texture_max_width,
        cgb_mode ? palette_t::cgb_texture_height : palette_t::dmg_obj_texture_height
    );

    ImVec2 obj_uv0(
        0.0f,
        0.0f
    );

    ImVec2 obj_uv1(
        1.0f,
        obj_size.y / palette_t::texture_max_height  
    );

    obj_size.x *= tile_viewer_t::palette_scale;
    obj_size.y *= tile_viewer_t::palette_scale;

    ImGui::Image((ImTextureRef)obj_palette.texture,obj_size,obj_uv0,obj_uv1);

    if(ImGui::IsItemClicked(ImGuiMouseButton_Left)){
        obj_palette_selected = true;
        palette_index = (ImGui::GetMousePos().y - ImGui::GetItemRectMin().y) / height;
    }

    if(obj_palette_selected){
        uint8_t index = palette_index % (cgb_mode ? gb_cgb_palettes : gb_dmg_obj_palettes);

        ImVec2 start = ImGui::GetItemRectMin();

        ImVec2 p_min(
            start.x,
            start.y + index * height
        );

        ImVec2 p_max(
            p_min.x + bg_size.x,
            p_min.y + height
        );

        ImGui::GetWindowDrawList()->AddRect(p_min,p_max,border_hovered_color,0.0f,0,2.0f);
    }

    ImGui::EndTable();
}

void tile_viewer_t::render_grid(ImVec2 start,ImVec2 end){
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for(int row = 0; row < rows + 1; ++row){
        draw_list->AddLineH(
            start.x,
            end.x,
            start.y + row * gb_tile_size * scale,
            grid_color
        );
    }

    for(int col = 0; col < columns + 1; ++col){
        draw_list->AddLineV(
            start.x + col * gb_tile_size * scale,
            start.y,
            end.y,
            grid_color
        );
    }
}

void tile_viewer_t::render_tile_tooltip(int row,int column){
    if(!ImGui::BeginTooltip()) return;

    if(ImGui::BeginTable("TileTooltipTable",2)){

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tile");
        ImGui::TableNextColumn();

        ImVec2 size(
            gb_tile_size * tile_viewer_t::tooltip_tile_scale,
            gb_tile_size * tile_viewer_t::tooltip_tile_scale
        );

        ImVec2 uv0(
            (float)(column * gb_tile_size) / (float)tile_viewer_t::texture_max_size,
            (float)(row * gb_tile_size) / (float)tile_viewer_t::texture_max_size
        );

        ImVec2 uv1(
            uv0.x + ((float)gb_tile_size / (float)tile_viewer_t::texture_max_size),
            uv0.y + ((float)gb_tile_size / (float)tile_viewer_t::texture_max_size)
        );

        ImGui::Image((ImTextureRef)texture,size,uv0,uv1);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Column, Row");
        ImGui::TableNextColumn();
        ImGui::Text("%d, %d",column,row);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tile address");
        ImGui::TableNextColumn();

        uint32_t address = 0;
        switch(current_layout){
            case tile_viewer_t::layout_8x8:{
                /*
                A B C D
                A B C D
                A B C D
                */
                address = (row * columns + column) << 0x04;
                break;
            }
            case tile_viewer_t::layout_8x16:{
                /*
                A C A C
                B D B D
                A C A C
                B D B D
                */
                address = ((row & ~0x01) * columns + ((column << 0x01) | (row & 0x01))) << 0x04;
                break;
            }
            case tile_viewer_t::layout_16x16:{
                /*
                A B A B
                C D C D
                A B A B
                C D C D
                */
                address = (((row & ~0x01) * columns) << 0x04) | ((column >> 0x01) << 0x06) | ((column & 0x01) << 0x04) | ((row & 0x01) << 0x05);
                break;
            }
        }
        ImGui::Text("$%04X",address % data_length);

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}

void tile_viewer_t::render(){
    if(!_open) return;

    if(!gb->cartridge_inserted){
        close();
    }

    if(ImGui::Begin("Tile Viewer",&_open)){

        if(ImGui::BeginTable("TileViewerTable",2)){

            ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            if(ImGui::BeginChild("TileViewerChild",ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
                
                update_texture();

                ImGui::Image((ImTextureRef)texture,texture_size,texture_uv0,texture_uv1);

                if(show_tile_grid) render_grid(ImGui::GetItemRectMin(),ImGui::GetItemRectMax());

                if(ImGui::IsItemHovered()){
                    ImVec2 mouse = ImGui::GetMousePos();
                    ImVec2 start = ImGui::GetItemRectMin();
                    
                    float tile_size = gb_tile_size * scale;

                    int row = (mouse.y - start.y) / tile_size;
                    int column = (mouse.x - start.x) / tile_size;

                    ImVec2 p_min(
                        start.x + (column * tile_size),
                        start.y + (row * tile_size)
                    );

                    ImVec2 p_max(
                        p_min.x + tile_size,
                        p_min.y + tile_size
                    );

                    ImGui::GetWindowDrawList()->AddRect(p_min,p_max,border_hovered_color,0.0f,0,2.0f);

                    render_tile_tooltip(row,column);
                }
            }
            ImGui::EndChild();

            ImGui::TableNextColumn();

            if(ImGui::BeginCombo("Source",sources[current_source])){

                int new_current_source = current_source;

                for(int i = 0; i < sources_count; ++i){
                    
                    if(
                        (i == tile_viewer_t::source_rom && !gb->cartridge.rom_size) ||
                        (i == tile_viewer_t::source_ram && !gb->cartridge.ram_size)
                    ){
                        continue;
                    }

                    if(ImGui::Selectable(sources[i],current_source == i)){
                        new_current_source = i;
                    }
                }

                if(new_current_source != current_source){
                    current_source = new_current_source;
                    update_data_length();
                    update_address_offset();
                }

                ImGui::EndCombo();
            }

            ImGui::Combo("Layout",&current_layout,layouts,layouts_count);

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Address",ImGuiDataType_U32,&address_offset,&input_scalar_step,nullptr,"%X")){
                update_address_offset();
            }

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Columns",ImGuiDataType_S32,&columns,&size_input_scalar_step)){
                if(columns > tile_viewer_t::max_size){
                    columns = tile_viewer_t::max_size;
                }
                else if(columns < tile_viewer_t::min_size){
                    columns = tile_viewer_t::min_size;
                }
                else{
                    columns &= ~1;
                }
                update_texture_size();
                update_texture_uv();
                update_data_length();
            }

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Rows",ImGuiDataType_S32,&rows,&size_input_scalar_step)){
                if(rows > tile_viewer_t::max_size){
                    rows = tile_viewer_t::max_size;
                }
                else if(rows < tile_viewer_t::min_size){
                    rows = tile_viewer_t::min_size;
                }
                else{
                    rows &= ~1;
                }
                update_texture_size();
                update_texture_uv();
                update_data_length();
            }

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh on scanline",ImGuiDataType_U8,&callback_handler.scanline,&input_scalar_step)){
                if(callback_handler.scanline >= gb_scanlines){
                    callback_handler.scanline = gb_scanlines - 1;
                }
            }

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh on cycle",ImGuiDataType_U16,&callback_handler.cycle,&input_scalar_step)){
                if(callback_handler.cycle >= gb_scanline_cycles){
                    callback_handler.cycle = gb_scanline_cycles - 1;
                }
            }

            ImGui::Checkbox("Show tile grid",&show_tile_grid);

            render_palette();

            ImGui::EndTable();
        }

        event();
    }
    ImGui::End();

    if(!_open){
        close();
    }
}