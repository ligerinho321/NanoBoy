#include "tilemap_viewer.hpp"

tilemap_viewer_t::tilemap_viewer_t(gb_t* gb,SDL_Renderer* renderer):gb(gb),bg_palette(renderer){
    
    tilemap_texture[0] = SDL_CreateTexture(renderer,texture_format,texture_access,tilemap_texture_width,tilemap_texture_height);
    tilemap_texture[1] = SDL_CreateTexture(renderer,texture_format,texture_access,tilemap_texture_width,tilemap_texture_height);

    ImGuiStyle& style = ImGui::GetStyle();

    grid_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,0.5f));
    scroll_overlay_border_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,1.0f));
    scroll_overlay_background_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f,1.0f,1.0f,0.2f));
    border_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]);
    border_hovered_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_NavCursor]);

    input_scalar_width = get_input_scalar_width();

    update_tilemap_size();
}

tilemap_viewer_t::~tilemap_viewer_t(){
    gb_remove_ppu_callback(gb,&callback_handler);

    SDL_DestroyTexture(tilemap_texture[0]);
    SDL_DestroyTexture(tilemap_texture[1]);
}


void tilemap_viewer_t::callback(void* data){

    tilemap_viewer_t* tmv = (tilemap_viewer_t*)data;

    gb_t* gb = tmv->gb;

    tmv->cgb_mode = gb->cgb_mode;
    
    tmv->tiledata_area = gb->ppu.lcdc.tiledata_area;
    
    tmv->scx = gb->ppu.scx;
    tmv->scy = gb->ppu.scy;

    tmv->bg_palette.bgp = gb->palette.bgp;
    memcpy(tmv->bg_palette.colors,gb->palette.bg_cram_converted,sizeof(tmv->bg_palette.colors));

    memcpy(tmv->vram,gb->ppu.vram,sizeof(tmv->vram));
}


void tilemap_viewer_t::update_tilemap_texture(uint8_t map_index){

    uint16_t base_address = (map_index == 0x01) ? 0x1C00 : 0x1800;
    uint8_t* map = vram + base_address;
    uint8_t* map_attribute = vram + (0x2000 | base_address);

    gb_rgb_t color{0};

    uint8_t* pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(tilemap_texture[map_index],nullptr,(void**)&pixels,&pitch);

    for(int row = 0; row < gb_tilemap_rows; ++row){
        for(int col = 0; col < gb_tilemap_columns; ++col){
            
            uint16_t index = (row << 0x05) | col;
            
            uint8_t tile_index = map[index];

            uint8_t attribute = cgb_mode ? map_attribute[index] : 0x00;

            uint16_t tile_address = tiledata_area ? tile_index << 0x04 : 0x1000 + ((int8_t)tile_index << 0x04);

            uint8_t* tile_data = vram + (((attribute & gb_tilemap_tile_bank_mask) ? 0x2000 : 0x0000) | tile_address);

            for(int y = 0; y < 8; ++y){
                
                uint8_t tile_byte_address = ((attribute & gb_tilemap_vertical_flip_mask) ? 0x07 ^ y : y) << 0x01;

                uint8_t lo = tile_data[tile_byte_address + 0x00];
                uint8_t hi = tile_data[tile_byte_address + 0x01];
                
                for(int x = 0; x < 8; ++x){
                    
                    uint8_t bit = 0x01 << ((attribute & gb_tilemap_horizontal_flip_mask) ? x : 0x07 ^ x);

                    uint8_t color_index = ((hi & bit) ? 0x02 : 0x00) | ((lo & bit) ? 0x01 : 0x00);

                    if(gb->type == gb_cgb){
                        if(cgb_mode){
                            color = bg_palette.get_cgb_color(attribute & gb_tilemap_palette_mask,color_index);
                        }
                        else{
                            color = bg_palette.get_cgb_dmg_color(0,color_index);
                        }
                    }
                    else{
                        color = bg_palette.get_dmg_color(0,color_index);
                    }

                    uint8_t* pixel = pixels + (((row << 0x03) | y) * pitch) + (((col << 0x03) | x) * texture_bytes_per_pixel);
                    pixel[0] = color.r;
                    pixel[1] = color.g;
                    pixel[2] = color.b;
                }
            }
        }
    }

    SDL_UnlockTexture(tilemap_texture[map_index]);
}


void tilemap_viewer_t::render_grid(ImVec2 tilemap_start){

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for(int i = 0; i < 33; ++i){
        draw_list->AddLineH(
            tilemap_start.x,
            tilemap_start.x + tilemap_texture_width * tilemap_scale,
            tilemap_start.y + i * gb_tile_size * tilemap_scale,
            grid_color
        );
        draw_list->AddLineV(
            tilemap_start.x + i * gb_tile_size * tilemap_scale,
            tilemap_start.y,
            tilemap_start.y + tilemap_texture_height * tilemap_scale,
            grid_color
        );
    }
}

void tilemap_viewer_t::render_scroll_overlay(ImVec2 tilemap_start,ImVec2 tilemap_end){

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    int scx_right = scx + 159;
    int scy_bottom = scy + 143;
    ImVec2 min;
    ImVec2 max;

    if(scx_right > 255 && scy_bottom > 255){
        
        min.x = tilemap_start.x;
        min.y = tilemap_start.y;
        max.x = min.x + (scx_right - 255) * tilemap_scale;
        max.y = min.y + (scy_bottom - 255) * tilemap_scale;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);

        min.x = tilemap_start.x + scx * tilemap_scale;
        min.y = tilemap_start.y;
        max.x = tilemap_end.x;
        max.y = min.y + (scy_bottom - 255) * tilemap_scale;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);

        min.x = tilemap_start.x;
        min.y = tilemap_start.y + scy * tilemap_scale;
        max.x = min.x + (scx_right - 255) * tilemap_scale;
        max.y = tilemap_end.y;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);

        min.x = tilemap_start.x + scx * tilemap_scale;
        min.y = tilemap_start.y + scy * tilemap_scale;
        max = tilemap_end;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);
    }
    else if(scx_right > 255){

        min.x = tilemap_start.x;
        min.y = tilemap_start.y + scy * tilemap_scale;
        max.x = tilemap_start.x + (scx_right - 255) * tilemap_scale;
        max.y = min.y + 144 * tilemap_scale;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);

        min.x = tilemap_start.x + scx * tilemap_scale;
        min.y = tilemap_start.y + scy * tilemap_scale;
        max.x = tilemap_end.x;
        max.y = min.y + 144 * tilemap_scale;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);
    }
    else if(scy_bottom > 255){

        min.x = tilemap_start.x + scx * tilemap_scale;
        min.y = tilemap_start.y;
        max.x = min.x + 160 * tilemap_scale;
        max.y = tilemap_start.y + (scy_bottom - 255) * tilemap_scale;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);

        min.x = tilemap_start.x + scx * tilemap_scale;
        min.y = tilemap_start.y + scy * tilemap_scale;
        max.x = min.x + 160 * tilemap_scale;
        max.y = tilemap_end.y;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);
    }
    else{

        min.x = tilemap_start.x + scx * tilemap_scale;
        min.y = tilemap_start.y + scy * tilemap_scale;
        max.x = min.x + 160 * tilemap_scale;
        max.y = min.y + 144 * tilemap_scale;
        draw_list->AddRectFilled(min,max,scroll_overlay_background_color);
        draw_list->AddRect(min,max,scroll_overlay_border_color);
    }

}

void tilemap_viewer_t::render_tile_tooltip(bool tilemap,uint8_t col,uint8_t row){

    ImVec2 start = ImGui::GetItemRectMin();

    if(!ImGui::BeginTooltip()) return;

    if(ImGui::BeginTable("TileTooltipTable",2)){

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        uint8_t x = col * gb_tile_size;
        uint8_t y = row * gb_tile_size;
        
        uint16_t base_address = tilemap ? 0x1C00 : 0x1800;
        uint16_t tilemap_address = base_address + row * gb_tilemap_columns + col;
        uint8_t tile_index = vram[tilemap_address];

        uint16_t attribute_address = 0x0000;
        uint8_t attribute = 0x00;
        uint8_t palette_index = 0x00;
        if(cgb_mode){
            attribute_address = 0x2000 | tilemap_address;
            attribute = vram[attribute_address];
            palette_index = attribute & gb_tilemap_palette_mask;
        }

        uint16_t tile_address = tiledata_area ? tile_index << 0x04 : 0x1000 + ((int8_t)tile_index << 0x04);
        if(cgb_mode && (attribute & gb_tilemap_tile_bank_mask)){
            tile_address |= 0x2000;
        }

        //Tile
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tile");
        ImGui::TableNextColumn();

        ImVec2 tile_texture_size(
            gb_tile_size * tooltip_tile_scale,
            gb_tile_size * tooltip_tile_scale
        );
        ImVec2 tile_texture_uv0(
            (float)x / (float)tilemap_texture_width,
            (float)y / (float)tilemap_texture_height
        );
        ImVec2 tile_texture_uv1(
            tile_texture_uv0.x + ((float)gb_tile_size / (float)tilemap_texture_width),
            tile_texture_uv0.y + ((float)gb_tile_size / (float)tilemap_texture_height)
        );

        ImGui::Image((ImTextureRef)tilemap_texture[tilemap],tile_texture_size,tile_texture_uv0,tile_texture_uv1);

        draw_list->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),border_color);

        //Palette
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Palette");
        ImGui::TableNextColumn();

        ImVec2 palette_texture_size(
            (float)palette_t::texture_max_width * (float)tooltip_palette_scale,
            (float)gb_tile_size * (float)tooltip_palette_scale
        );
        ImVec2 palette_texture_uv0(
            0.0f,
            (cgb_mode ? palette_index * gb_tile_size : 0) / (float)palette_t::texture_max_height
        );
        ImVec2 palette_texture_uv1(
            1.0f,
            palette_texture_uv0.y + ((float)gb_tile_size / (float)palette_t::texture_max_height)
        );

        ImGui::Image((ImTextureRef)bg_palette.texture,palette_texture_size,palette_texture_uv0,palette_texture_uv1);

        draw_list->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),border_color);


        //Column, Row
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Column, Row");
        ImGui::TableNextColumn();
        ImGui::Text("%d, %d",col,row);

        //X, Y
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("X, Y");
        ImGui::TableNextColumn();
        ImGui::Text("%d, %d",x,y);

        //Size
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Size");
        ImGui::TableNextColumn();
        ImGui::Text("%dx%d",gb_tile_size,gb_tile_size);

        //Tilemap address
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tilemap address");
        ImGui::TableNextColumn();
        ImGui::Text("$%04X",tilemap_address);

        //Tile index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tile index");
        ImGui::TableNextColumn();
        ImGui::Text("$%02X",tile_index);

        //Tile address
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Tile address");
        ImGui::TableNextColumn();
        ImGui::Text("$%04X",tile_address);

        if(cgb_mode){
            //Attribute address
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Attribute address");
            ImGui::TableNextColumn();
            ImGui::Text("$%04X",attribute_address);

            //Attribute data
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Attribute data");
            ImGui::TableNextColumn();
            ImGui::Text("$%02X",attribute);

            //Palette index
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Palette index");
            ImGui::TableNextColumn();
            ImGui::Text("$%02X",palette_index);

            //Horizontal flip
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Horizontal flip");
            ImGui::TableNextColumn();
            if(attribute & gb_tilemap_horizontal_flip_mask){
                ImGui::TextUnformatted("True");
            }
            else{
                ImGui::TextUnformatted("False");
            }

            //Vertical flip
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Vertical flip");
            ImGui::TableNextColumn();
            if(attribute & gb_tilemap_vertical_flip_mask){
                ImGui::TextUnformatted("True");
            }
            else{
                ImGui::TextUnformatted("False");
            }

            //Priority
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Priority");
            ImGui::TableNextColumn();
            if(attribute & gb_tilemap_priority_mask){
                ImGui::TextUnformatted("True");
            }
            else{
                ImGui::TextUnformatted("False");
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndTooltip();
}

void tilemap_viewer_t::render_tilemap(const char* str_id,bool tilemap){

    if(ImGui::BeginChild(str_id,ImVec2(0.0f,0.0f),ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar)){
        
        bg_palette.update_texture(gb->type,cgb_mode);
        update_tilemap_texture(tilemap);

        ImGui::Image((ImTextureRef)tilemap_texture[tilemap],tilemap_size);
        
        ImVec2 tilemap_start = ImGui::GetItemRectMin();
        ImVec2 tilemap_end = ImGui::GetItemRectMax();

        if(show_tile_grid) render_grid(tilemap_start);
        
        if(show_scroll_overlay) render_scroll_overlay(tilemap_start,tilemap_end);

        if(ImGui::IsItemHovered()){

            ImVec2 mouse = ImGui::GetMousePos();

            float tile_size = gb_tile_size * tilemap_scale;

            uint8_t col = (uint8_t)((mouse.x - tilemap_start.x) / tile_size);
            uint8_t row = (uint8_t)((mouse.y - tilemap_start.y) / tile_size);

            ImVec2 tile_start(
                tilemap_start.x + (col * tile_size),
                tilemap_start.y + (row * tile_size)
            );
            ImVec2 tile_end(
                tile_start.x + tile_size,
                tile_start.y + tile_size
            );

            ImGui::GetWindowDrawList()->AddRect(tile_start,tile_end,border_hovered_color,0.0f,0,2.0f);

            render_tile_tooltip(tilemap,col,row);
        }
    }
    ImGui::EndChild();
}

void tilemap_viewer_t::render(){
    
    if(!open) return;

    bool _open = open;

    if(ImGui::Begin("Tilemap Viewer",&_open)){

        if(ImGui::BeginTable("LayoutTable",2)){

            ImGui::TableSetupColumn("Left",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Right",ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();

            if(ImGui::BeginTabBar("AddressPointer")){
                
                if(ImGui::BeginTabItem("9800")){
                    render_tilemap("Tilemap0",0);
                    ImGui::EndTabItem();
                }
                
                if(ImGui::BeginTabItem("9C00")){
                    render_tilemap("Tilemap1",1);
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::TableNextColumn();

            ImGui::Checkbox("Show Tile Grid",&show_tile_grid);

            ImGui::Checkbox("Shwo Scroll Overlay",&show_scroll_overlay);

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

        if(ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){

            if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && tilemap_scale < tilemap_max_scale){
                ++tilemap_scale;
                update_tilemap_size();
            }
            else if(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && tilemap_scale > tilemap_min_scale){
                --tilemap_scale;
                update_tilemap_size();
            }

            ImGuiIO& io = ImGui::GetIO();
            if(io.KeyCtrl){
                if(io.MouseWheel > 0.0f && tilemap_scale < tilemap_max_scale){
                    ++tilemap_scale;
                    update_tilemap_size();
                }
                else if(io.MouseWheel < 0.0f && tilemap_scale > tilemap_min_scale){
                    --tilemap_scale;
                    update_tilemap_size();
                }
            }
        }
    }
    ImGui::End();

    set_open(_open);
}