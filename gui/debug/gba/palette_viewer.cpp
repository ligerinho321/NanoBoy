#include <gui/debug/gba/palette_viewer.hpp>

void gba_palette_viewer_t::init(gba_t* _gba,SDL_Renderer* renderer){
    gba = _gba;

    file_save.init();
    file_save.set_extensions(image_extensions,image_extensions_count);
    file_save.set_callback(file_save_callback,this);

    bg_palette.init(renderer);
    obj_palette.init(renderer);

    update_palette_size();

    input_scalar_width = get_input_scalar_width(8);

    ImGuiStyle& style = ImGui::GetStyle();

    grid_color = 0xFFFFFFFF;
    border_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]);
    border_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);
}

void gba_palette_viewer_t::uninit(){
    bg_palette.uninit();
    obj_palette.uninit();
}


void gba_palette_viewer_t::ppu_callback(void* userdata){
    gba_palette_viewer_t* palette_viewer = (gba_palette_viewer_t*)userdata;
    
    gba_ppu_t* ppu = &palette_viewer->gba->ppu;

    palette_viewer->bg_palette.update_data(ppu->cram + 0x000);
    palette_viewer->bg_palette.update_texture();

    palette_viewer->obj_palette.update_data(ppu->cram + 0x200);
    palette_viewer->obj_palette.update_texture();
}


void gba_palette_viewer_t::file_save_callback(void* userdata,std::filesystem::path path){
    gba_palette_viewer_t* palette_viewer = (gba_palette_viewer_t*)userdata;
    
    gba_palette_panel_t& palette = palette_viewer->current_palette_index ? palette_viewer->obj_palette : palette_viewer->bg_palette;

    uint8_t* pixels = nullptr;
    int pitch = 0;

    if(!SDL_LockTexture(palette.texture,nullptr,(void**)&pixels,&pitch)){

        save_image(path,pixels,gba_palette_panel_t::texture_width,gba_palette_panel_t::texture_height,gba_palette_panel_t::texture_bytes_per_pixel);

        SDL_UnlockTexture(palette.texture);
    }
    else{
        printf("SDL_LockTexture: %s\n",SDL_GetError());
    }
}


void gba_palette_viewer_t::update_palette_size(){
    palette_size.x = gba_palette_panel_t::texture_width * palette_scale;
    palette_size.y = gba_palette_panel_t::texture_height * palette_scale;
}


void gba_palette_viewer_t::event(){
    if(!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) return;

    if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && palette_scale < image_max_scale){
        ++palette_scale;
        update_palette_size();
    }
    else if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && palette_scale > image_min_scale){
        --palette_scale;
        update_palette_size();
    }

    ImGuiIO& io = ImGui::GetIO();
    if(io.KeyCtrl){
        if(io.MouseWheel > 0.0f && palette_scale < image_max_scale){
            ++palette_scale;
            update_palette_size();
        }
        else if(io.MouseWheel < 0.0f && palette_scale > image_min_scale){
            --palette_scale;
            update_palette_size();
        }
    }
}


void gba_palette_viewer_t::render_tooltip_color(gba_palette_panel_t& palette,int col,int row){
    if(!ImGui::BeginTooltip()) return;

    if(ImGui::BeginTable("ColorTooltip",2)){

        // Color
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Color");
        ImGui::TableNextColumn();

        ImVec2 color_texture_size(
            64,
            64
        );

        ImVec2 color_texture_uv0(
            (col * 8.0f) / (float)gba_palette_panel_t::texture_width,
            (row * 8.0f) / (float)gba_palette_panel_t::texture_height
        );

        ImVec2 color_texture_uv1(
            color_texture_uv0.x + (8.0f / (float)gba_palette_panel_t::texture_width),
            color_texture_uv0.y + (8.0f / (float)gba_palette_panel_t::texture_height)
        );

        ImGui::Image((ImTextureRef)palette.texture,color_texture_size,color_texture_uv0,color_texture_uv1);

        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),border_color);


        int index = row * 16 + col;

        uint16_t color = palette.colors[index] & 0x7FFF;

        SDL_Color rgb555 = unpack_rgb555(color);

        SDL_Color rgb888 = rgb555_to_rgb888(rgb555);

        // Index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Index");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",index);

        // Value
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Value");
        ImGui::TableNextColumn();
        ImGui::Text("$%04X",color);

        // Red
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Red");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",rgb555.r);

        // Green
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Green");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",rgb555.g);

        // Blue
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Blue");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",rgb555.b);

        // Hex code
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Hex Code");
        ImGui::TableNextColumn();
        ImGui::Text("#%02X%02X%02X",rgb888.r,rgb888.g,rgb888.b);

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}

void gba_palette_viewer_t::render_palette(const char* id,gba_palette_panel_t& palette){
    if(ImGui::BeginChild(id,ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
        
        ImGui::Image((ImTextureRef)palette.texture,palette_size);
        
        if(show_grid){
            ImVec2 start = ImGui::GetItemRectMin();
            ImVec2 end = ImGui::GetItemRectMax();

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            float tile_size = 8 * palette_scale;

            for(int i = 0; i < 17; ++i){
                draw_list->AddLineH(
                    start.x,
                    end.x,
                    start.y + i * tile_size,
                    grid_color
                );

                draw_list->AddLineV(
                    start.x + i * tile_size,
                    start.y,
                    end.y,
                    grid_color
                );
            }
        }

        if(ImGui::IsItemHovered()){
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 start = ImGui::GetItemRectMin();

            float tile_size = 8 * palette_scale;

            int row = (mouse.y - start.y) / tile_size;
            int col = (mouse.x - start.x) / tile_size;

            ImVec2 p_min(
                start.x + (col * tile_size),
                start.y + (row * tile_size)
            );

            ImVec2 p_max(
                p_min.x + tile_size,
                p_min.y + tile_size
            );

            ImGui::GetWindowDrawList()->AddRect(
                p_min,
                p_max,
                border_hovered_color,
                0.0f,
                0,
                2.0f
            );

            render_tooltip_color(palette,col,row);
        }
    }
    ImGui::EndChild();
}

void gba_palette_viewer_t::render(){
    if(!open) return;

    if(!gba->cartridge_inserted){
        set_open(false);
    }

    bool _open = open;

    if(ImGui::Begin("Palette Viewer",&_open)){

        if(ImGui::BeginTable("PaletteTable",2)){

            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(nullptr,ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            if(ImGui::BeginTabBar("PaletteTabBar")){

                if(ImGui::BeginTabItem("Background")){
                    current_palette_index = 0;

                    render_palette("BackgroundPalette",bg_palette);

                    ImGui::EndTabItem();
                }

                if(ImGui::BeginTabItem("Object")){
                    current_palette_index = 1;
                    
                    render_palette("ObjectPalette",obj_palette);

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::TableNextColumn();

            if(ImGui::Button("Export")){
                file_save.set_open(true);
            }

            ImGui::SeparatorText("View Options");
            
            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh On Scanline",ImGuiDataType_U8,&ppu_handler.scanline,&input_scalar_step,&input_scalar_step_fast)){
                if(ppu_handler.scanline >= gba_scanlines){
                    ppu_handler.scanline = gba_scanlines - 1;
                }
            }

            ImGui::SetNextItemWidth(input_scalar_width);
            if(ImGui::InputScalar("Refresh On Cycle",ImGuiDataType_U8,&ppu_handler.cycle,&input_scalar_step,&input_scalar_step_fast)){
                if(ppu_handler.cycle >= gba_scanline_cycles){
                    ppu_handler.cycle = gba_scanline_cycles - 1;
                }
            }

            ImGui::Checkbox("Show Grid",&show_grid);

            ImGui::EndTable();
        }

        event();

        file_save.render();
    }
    ImGui::End();

    set_open(_open);
}


void gba_palette_viewer_t::clear(){
    bg_palette.clear();
    obj_palette.clear();
}

void gba_palette_viewer_t::set_open(bool _open) noexcept {
    if(open == _open) return;

    open = _open;

    if(open){
        gba_ppu_add_handler(gba,&ppu_handler);
    }
    else{
        gba_ppu_remove_handler(gba,&ppu_handler);
    }
}