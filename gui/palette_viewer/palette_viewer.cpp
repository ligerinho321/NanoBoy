#include <gui/palette_viewer/palette_viewer.hpp>

palette_viewer_t::palette_viewer_t(gb_t* gb,SDL_Renderer* renderer):gb(gb),bg_palette(renderer),obj_palette(renderer){
    input_scalar_width = get_input_scalar_width();

    ImGuiStyle& style = ImGui::GetStyle();

    border_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]);
    border_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);
}

palette_viewer_t::~palette_viewer_t(){
    gb_thread_safe_remove_ppu_handler(gb,&callback_handler);
}


void palette_viewer_t::callback(void* data){
    palette_viewer_t* pv = (palette_viewer_t*)data;
    gb_t* gb = pv->gb;

    bg_palette_t& bg_palette = pv->bg_palette;
    obj_palette_t& obj_palette = pv->obj_palette;

    pv->cgb_mode = pv->gb->cgb_mode;
    
    bg_palette.bgp = gb->palette.bgp;
    
    obj_palette.obp[0] = gb->palette.obp[0];
    obj_palette.obp[1] = gb->palette.obp[1];

    memcpy(bg_palette.colors,gb->palette.bg_cram_converted,sizeof(bg_palette.colors));
    memcpy(obj_palette.colors,gb->palette.obj_cram_converted,sizeof(obj_palette.colors));

    memcpy(pv->bg_cram,gb->palette.bg_cram,sizeof(pv->bg_cram));
    memcpy(pv->obj_cram,gb->palette.obj_cram,sizeof(pv->obj_cram));
}


void palette_viewer_t::render_tooltip_color(palette_t& palette,uint8_t* cram,uint8_t col,uint8_t row){

    if(!ImGui::BeginTooltip()) return;

    if(ImGui::BeginTable("ColorTooltipTable",2)){

        //Color
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Color");
        ImGui::TableNextColumn();

        ImVec2 color_texture_size(
            (float)gb_tile_size * (float)tooltip_color_scale,
            (float)gb_tile_size * (float)tooltip_color_scale
        );

        ImVec2 color_texture_uv0(
            (col * gb_tile_size) / (float)palette_t::texture_max_width,
            (row * gb_tile_size) / (float)palette_t::texture_max_height
        );

        ImVec2 color_texture_uv1(
            color_texture_uv0.x + (gb_tile_size / (float)palette_t::texture_max_width),
            color_texture_uv0.y + (gb_tile_size / (float)palette_t::texture_max_height)
        );

        ImGui::Image((ImTextureRef)palette.texture,color_texture_size,color_texture_uv0,color_texture_uv1);

        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),border_color);

        //Palette index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Palette index");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",row);

        //Color index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Color index");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",col);

        gb_rgb_t color{0};

        if(gb->type == gb_cgb){

            uint16_t address = 0x00;

            if(cgb_mode){
                color = palette.get_cgb_color(row,col);
                address = palette.get_cgb_address_color(row,col) * gb_cgb_bytes_per_color;
            }
            else{
                color = palette.get_cgb_dmg_color(row,col);
                address = palette.get_cgb_dmg_address_color(row,col) * gb_cgb_bytes_per_color;
            }

            uint16_t value = (cram[address + 0x01] << 0x08) | (cram[address + 0x00] << 0x00);
            uint8_t r = (value >> 0x00) & 0x1F;
            uint8_t g = (value >> 0x05) & 0x1F;
            uint8_t b = (value >> 0x0A) & 0x1F;

            //Value
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Value");
            ImGui::TableNextColumn();
            ImGui::Text("$%04X",value);

            //RGB555
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("RGB555");
            ImGui::TableNextColumn();
            ImGui::Text("%d, %d, %d",r,g,b);
        }
        else{
            color = palette.get_dmg_color(row,col);

            //Value
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Value");
            ImGui::TableNextColumn();
            ImGui::Text("$%02X\n",palette.get_dmg_address_color(row,col));
        }

        //RGB888
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("RGB888");
        ImGui::TableNextColumn();
        ImGui::Text("%d, %d, %d",color.r,color.g,color.b);

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}

void palette_viewer_t::render_palette(palette_t& palette,uint8_t* cram){

    ImVec2 texture_size(
        (float)palette_t::texture_max_width,
        (float)(cgb_mode ? palette_t::cgb_texture_height : (palette.is_obj ? palette_t::dmg_obj_texture_height : palette_t::dmg_bg_texture_height))
    );
    
    ImVec2 texture_uv0(
        0.0f,
        0.0f
    );
    
    ImVec2 texture_uv1(
        1.0f,
        texture_size.y / (float)palette_t::texture_max_height
    );

    texture_size.x *= palette_scale;
    texture_size.y *= palette_scale;

    palette.update_texture(gb->type,cgb_mode);

    ImGui::Image((ImTextureRef)palette.texture,texture_size,texture_uv0,texture_uv1);

    if(ImGui::IsItemHovered()){
        ImVec2 start = ImGui::GetItemRectMin();

        ImVec2 mouse = ImGui::GetMousePos();

        float tile_size = gb_tile_size * palette_scale;

        uint8_t col = (uint8_t)((mouse.x - start.x) / tile_size);
        uint8_t row = (uint8_t)((mouse.y - start.y) / tile_size);

        ImVec2 p0(
            start.x + col * tile_size,
            start.y + row * tile_size
        );
        ImVec2 p1(
            p0.x + tile_size,
            p0.y + tile_size
        );

        ImGui::GetWindowDrawList()->AddRect(p0,p1,border_hovered_color,0.0f,0,2.0f);

        render_tooltip_color(palette,cram,col,row);
    }
}

void palette_viewer_t::render(){
    if(!open) return;

    if(!gb->cartridge_inserted){
        set_open(false);
    }

    bool _open = open;

    if(ImGui::Begin("Palette Viewer",&_open)){

        if(ImGui::BeginTable("PaletteTable",3)){

            ImGui::TableSetupColumn("BackgroundColumn",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("ObjectColumn",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("ControlColumn",ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Background");
            render_palette(bg_palette,bg_cram);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Object");
            render_palette(obj_palette,obj_cram);

            ImGui::TableNextColumn();

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh on scanline",ImGuiDataType_U8,&callback_handler.scanline,&input_scalar_step,&input_scalar_step_fast)){
                if(callback_handler.scanline >= gb_scanlines){
                    callback_handler.scanline = gb_scanlines - 1;
                }
            }

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh on cycle",ImGuiDataType_U16,&callback_handler.cycle,&input_scalar_step,&input_scalar_step_fast)){
                if(callback_handler.cycle >= gb_scanline_cycles){
                    callback_handler.cycle = gb_scanline_cycles - 1;
                }
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();

    set_open(_open);
}


void palette_viewer_t::clear(){
    cgb_mode = false;
    
    bg_palette.clear();
    obj_palette.clear();

    memset(bg_cram,0,sizeof(bg_cram));
    memset(obj_cram,0,sizeof(obj_cram));
}


void palette_viewer_t::set_open(bool _open) noexcept {
    if(open == _open) return;

    open = _open;
    
    if(open){
        gb_thread_safe_add_ppu_handler(gb,&callback_handler);
    }
    else{
        gb_thread_safe_remove_ppu_handler(gb,&callback_handler);
    }
}