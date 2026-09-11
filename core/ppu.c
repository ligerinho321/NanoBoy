#include "ppu.h"
#include "gb.h"


void gb_ppu_init(gb_ppu_t* ppu,gb_t* gb){
    ppu->gb = gb;

    ppu->vram_descriptor = (gb_memory_descriptor_t){
        gb_ppu_write_vram,
        gb_ppu_read_vram,
        ppu
    };

    ppu->register_descriptor = (gb_memory_descriptor_t){
        gb_ppu_write_register,
        gb_ppu_read_register,
        ppu
    };

    ppu->vbk_register_descriptor = (gb_memory_descriptor_t){
        gb_ppu_write_vbk_register,
        gb_ppu_read_vbk_register,
        ppu
    };
    
    ppu->oam_descriptor = (gb_memory_descriptor_t){
        gb_ppu_write_oam,
        gb_ppu_read_oam,
        ppu
    };
}


void gb_ppu_add_handler(gb_t* gb,gb_ppu_handler_t* handler){
    gb_list_add_element(gb->ppu.handlers,handler,gb_ppu_handler_t);
}

void gb_ppu_remove_handler(gb_t* gb,gb_ppu_handler_t* handler){
    gb_list_remove_element(gb->ppu.handlers,handler,gb_ppu_handler_t);
}


void gb_ppu_interframe_blending(gb_ppu_t* ppu){

    uint8_t* last = ppu->state.screen[!ppu->state.screen_index];
    
    uint8_t* current = ppu->state.screen[ppu->state.screen_index];
    
    uint8_t* end = current + gb_screen_length;

    while(current < end){
        *current = *last * 0.4f + *current * 0.6f;
        ++current;
        ++last;
    }
}


static inline void gb_ppu_swap_frame_buffer(gb_ppu_t* ppu){

    if(ppu->interframe_blending){
        gb_ppu_interframe_blending(ppu);
    }

    ppu->state.screen_index = !ppu->state.screen_index;

    ppu->current_screen = ppu->state.screen[ppu->state.screen_index];
}


static inline void gb_ppu_visible_scanline(gb_ppu_t* ppu){
    
    gb_ppu_state_t* state = &ppu->state;

    switch(state->cycle){
        case 4:{
            state->_lyc = state->lyc;

            if(state->scanline != 0 || !state->first_frame){

                state->status.mode = gb_ppu_oam_mode;

                state->event_screen_color = gb_event_screen_palette[gb_event_screen_oam_scan_color];

                state->object_buffer_length = 0;
                state->oam_address = 0;

                state->oam_write_blocked = true;
            }
            break;
        }
        case 80:{
            if(state->scanline != 0 || !state->first_frame){
                state->vram_read_blocked = true;   
            }
            break;
        }
        case 83:{
            state->oam_write_blocked = false;
            break;
        }
        case 84:{
            state->status.mode = gb_ppu_drawing_mode;

            state->vram_write_blocked = true;
            state->vram_read_blocked = true;

            state->oam_write_blocked = true;
            state->oam_read_blocked = true;

            state->wx_enabled = false;

            state->tile_fetcher.step = 0x00;
            state->object_fetcher.step = 0x00;

            state->object_found_index = 0xFF;
            state->fetch_window = false;
            state->fetch_column = 0x00;
            state->drawn_pixels = -0x08 - (state->scx & 0x07);

            state->fictitious_fetch = true;

            state->tile_fifo.length = 0x08;

            state->object_fifo.length = 0x00;
            memset(state->object_fifo.data,0x00,sizeof(state->object_fifo.data));

            state->event_screen_color = gb_event_screen_palette[gb_event_screen_fictitious_fetch_color];
            break;
        }
        case 89:{
            state->fictitious_fetch = false;
            break;
        }
        case gb_scanline_cycles:{
            state->cycle = 0;
            state->scanline++;
            state->ly = state->scanline;

            state->_lyc = 0xFFFF;

            if(state->scanline < gb_vblank_scanline){
                state->oam_read_blocked = true;
            }

            if(!state->wy_enabled){
                state->wy_enabled = state->ly == state->wy;
            }

            break;
        }
    }
}

static inline void gb_ppu_vblank_scanline(gb_ppu_t* ppu){
    
    gb_ppu_state_t* state = &ppu->state;

    switch(state->cycle){
        case 4:{
            state->_lyc = state->lyc;

            if(state->scanline == gb_vblank_scanline){

                state->wy_enabled = false;
                
                state->status.mode = gb_ppu_vblank_mode;

                state->event_screen_color = gb_event_screen_palette[gb_event_screen_vblank_color];

                ppu->gb->interrupt.state.flag |= gb_interrupt_vblank_flag;

            }
            break;
        }
        case 5:{
            if(state->scanline == 153){
                state->ly = 0;
                state->_lyc = 0xFFFF;
            }
            break;
        }
        case 12:{
            if(state->scanline == 153){
                state->_lyc = state->lyc;
            }
            break;
        }
        case gb_scanline_cycles:{
            state->cycle = 0;
            state->scanline++;

            if(state->scanline >= gb_scanlines){
                state->scanline = 0;

                state->wy_enabled = state->ly == state->wy;
                state->window_ly = -1;

                if(!state->first_frame){
                    gb_ppu_swap_frame_buffer(ppu);
                }
                else{
                    state->first_frame = false;
                }

                ++state->frame_count;

                gb_frame_timer_clock(&ppu->gb->frame_timer);

                gb_joypad_update(&ppu->gb->joypad);

                gb_event_manager_swap_frame(ppu->gb);
            }
            else{
                state->_lyc = 0xFFFF;
            }

            state->ly = state->scanline;
        }
        break;
    }
}

static inline void gb_ppu_update_irq_line(gb_ppu_t* ppu){
    
    //No primeiro frame quando a PPU é ligada a primeira scanline começa no modo 0 inves do 2 e vai diretamente para o 3
    //e durante o modo 0 mesmo que STAT bit3 esteja definido STAT IF não é ativado

    gb_ppu_state_t* state = &ppu->state;

    bool new_irq_line = (
        (state->status.lcy_equals_ly && state->status.lyc_enabled) ||
        (state->status.mode == gb_ppu_hblank_mode && (state->status.hblank_enabled && !(state->first_frame && state->ly == 0 && state->cycle < 84))) ||
        (state->status.mode == gb_ppu_vblank_mode && (state->status.vblank_enabled || (state->ly == 144 && state->cycle == 4 && state->status.oam_enabled))) || 
        (state->status.mode == gb_ppu_oam_mode && state->status.oam_enabled)
    );

    if(!state->status_irq_line && new_irq_line){
        ppu->gb->interrupt.state.flag |= gb_interrupt_lcd_flag;
    }

    state->status_irq_line = new_irq_line;
}

static inline void gb_ppu_oam_evaluation(gb_ppu_t* ppu){

    gb_ppu_state_t* state = &ppu->state;

    if(!(state->cycle & 0x01) || state->object_buffer_length >= 0x0A) return;

    uint8_t ly = state->ly + 0x10;
    uint8_t object_height = state->lcdc.object_size ? gb_object_max_height : gb_object_min_height;
    gb_object_t* object = (gb_object_t*)(state->oam + state->oam_address);

    if(ly >= object->y && ly < (object->y + object_height)){
        memcpy(state->object_buffer + state->object_buffer_length++,object,0x04);
    }

    state->oam_address += 0x04;
}


void gb_pixel_fifo_pop(gb_pixel_fifo_t* fifo){
    if(fifo->length == 0x00) return;

    gb_pixel_fifo_entry_t* entry = fifo->data + fifo->front;
    entry->palette_index = 0;
    entry->color_index = 0;
    entry->priority = false;
    entry->index = 0;
    
    fifo->front = (fifo->front + 0x01) & 0x07;

    fifo->length--;
}


static inline void gb_ppu_render_pixel(gb_ppu_t* ppu){

    gb_ppu_state_t* state = &ppu->state;

    //No primeiro frame apos a PPU ser ligada os pixels não são enviados para a tela segundo o teste firstwhite.gb

    if(state->drawn_pixels >= 0){

        gb_pixel_fifo_entry_t* tile = state->tile_fifo.data + state->tile_fifo.front;
        gb_pixel_fifo_entry_t* object = state->object_fifo.data + state->object_fifo.front;

        gb_rgb_t color = {0};

        gb_palette_t* palette = &ppu->gb->palette;

        if(state->lcdc.object_enabled && object->color_index && (!tile->color_index || !state->lcdc.tile_enabled || (!tile->priority && !object->priority))){
            if(ppu->gb->state.is_cgb){
                if(ppu->gb->state.cgb_mode){
                    color = gb_palette_get_cgb_obp_color(palette,object->palette_index,object->color_index);
                }
                else{
                    color = gb_palette_get_cgb_dmg_obp_color(palette,object->palette_index,object->color_index);
                }
            }
            else{
                color = gb_palette_get_dmg_obp_color(palette,object->palette_index,object->color_index);
            }
        }
        else{
            if(ppu->gb->state.is_cgb){
                if(ppu->gb->state.cgb_mode){
                    color = gb_palette_get_cgb_bgp_color(palette,tile->palette_index,tile->color_index);
                }
                else{
                    color = gb_palette_get_cgb_dmg_bgp_color(palette,tile->color_index);
                }
            }
            else{
                color = gb_palette_get_dmg_bgp_color(palette,tile->color_index);
            }
        }

        uint8_t* pixel = ppu->current_screen + state->scanline * gb_screen_pitch + state->drawn_pixels * gb_screen_bytes_per_pixel;
        
        pixel[0] = color.r;
        pixel[1] = color.g;
        pixel[2] = color.b;

        state->event_screen_color = color;
    }

    state->drawn_pixels++;

    gb_pixel_fifo_pop(&state->tile_fifo);
    gb_pixel_fifo_pop(&state->object_fifo);
}

static inline void gb_ppu_drawing(gb_ppu_t* ppu){
    gb_ppu_state_t* state = &ppu->state;
    gb_tile_fetcher_t* tile_fetcher = &ppu->state.tile_fetcher;

    if(!state->wx_enabled){
        state->wx_enabled = state->drawn_pixels == (state->wx - 0x07);

        bool fetch_window = state->lcdc.window_enabled && state->wx_enabled && state->wy_enabled;

        if(state->fetch_window != fetch_window){
            state->window_ly++;
            tile_fetcher->step = 0;
            state->fetch_window = fetch_window;
            state->fetch_column = 0;
            state->tile_fifo.length = 0;
        }
    }

    //O buscador de sprite espera que o fifo de tiles não esteja vazio
    //O buscador de sprite espera até que o buscador de tile termine para começar a sua busca
    //O primero ciclo do buscador de sprite se sobrepoem ao ultimo ciclo do buscador de tile

    if(state->object_found_index == 0xFF){
        for(uint8_t i = 0x00; i < state->object_buffer_length; ++i){
            if((int)state->object_buffer[i].x - 0x08 == state->drawn_pixels){
                state->object_found_index = i;
                break;
            }
        }
    }

    if(state->object_found_index != 0xFF && tile_fetcher->step >= 0x05 && state->tile_fifo.length > 0x00){
        
        state->event_screen_color = gb_event_screen_palette[gb_event_screen_object_fetch_color];

        gb_object_fetcher_t* object_fetcher = &state->object_fetcher;

        switch(object_fetcher->step++){
            case 0x01:{
                gb_object_t* sprite = state->object_buffer + state->object_found_index;
                
                uint8_t y = (state->ly + 0x10) - sprite->y;

                object_fetcher->tile_address = (ppu->gb->state.cgb_mode && (sprite->attribute & gb_object_tile_bank_mask)) ? 0x2000 : 0x0000;
                object_fetcher->tile_address |= (sprite->tile_index & (state->lcdc.object_size ? 0xFE : 0xFF)) << 0x04;
                object_fetcher->tile_address |= ((sprite->attribute & gb_object_vertical_flip_mask) ? ((state->lcdc.object_size ? 0x0F : 0x07) ^ y) : y) << 0x01;
                break;
            }
            case 0x03:{
                object_fetcher->lo = state->vram[object_fetcher->tile_address + 0x00];
                break;
            }
            case 0x05:{
                object_fetcher->hi = state->vram[object_fetcher->tile_address + 0x01];

                gb_object_t* sprite = state->object_buffer + state->object_found_index;

                for(uint8_t i = 0x00; i < 0x08; ++i){
                    uint8_t bit = 0x01 << ((sprite->attribute & gb_object_horizontal_flip_mask) ? i : 0x07 ^ i);
                    uint8_t color_index = ((object_fetcher->hi & bit) ? 0x02 : 0x00) | ((object_fetcher->lo & bit) ? 0x01 : 0x00);

                    gb_pixel_fifo_entry_t* entry = state->object_fifo.data + ((state->object_fifo.front + i) & 0x07);

                    if(color_index && (!entry->color_index || (!ppu->gb->state.obj_priority_mode && state->object_found_index < entry->index))){
                        if(ppu->gb->state.cgb_mode){
                            entry->palette_index = sprite->attribute & gb_object_cgb_palette_mask;
                        }
                        else{
                            entry->palette_index = (sprite->attribute & gb_object_dmg_palette_mask) ? 0x01 : 0x00;
                        }
                        entry->color_index = color_index;
                        entry->priority = sprite->attribute & gb_object_priority_mask;
                        entry->index = state->object_found_index;
                    }
                }

                object_fetcher->step = 0x00;
                state->object_fifo.length = 0x08;
                state->object_found_index = 0xFF;
                sprite->x = 0xFF;
                break;
            }
        }
    }
    else{
        if(state->fetch_window){
            state->event_screen_color = gb_event_screen_palette[gb_event_screen_window_fetch_color];
        }
        else{
            state->event_screen_color = gb_event_screen_palette[gb_event_screen_background_fetch_color];
        }

        switch(tile_fetcher->step++){
            case 0x01:{
                uint8_t y_fine = 0x00;
                uint16_t map_address = 0x0000;

                if(state->fetch_window){
                    uint8_t x = state->fetch_column & 0x1F;
                    uint8_t y = (state->window_ly >> 0x03) & 0x1F;

                    y_fine = state->window_ly & 0x07;
                    map_address = (state->lcdc.window_tilemap_area ? 0x1C00 : 0x1800) | (y << 0x05) | x;
                }
                else{
                    uint8_t x = (state->fetch_column + (state->scx >> 0x03)) & 0x1F;
                    uint8_t y = ((state->ly + state->scy) >> 0x03) & 0x1F;

                    y_fine = (state->ly + state->scy) & 0x07;
                    map_address = (state->lcdc.bg_tilemap_area ? 0x1C00 : 0x1800) | (y << 0x05) | x;
                }
                
                uint8_t tile_index = state->vram[map_address];

                tile_fetcher->attribute = ppu->gb->state.cgb_mode ? state->vram[0x2000 | map_address] : 0x00;
                
                tile_fetcher->tile_address = (tile_fetcher->attribute & gb_tilemap_tile_bank_mask) ? 0x2000 : 0x0000;
                tile_fetcher->tile_address |= state->lcdc.tiledata_area ? tile_index << 0x04 : 0x1000 + ((int8_t)tile_index << 0x04);
                tile_fetcher->tile_address |= ((tile_fetcher->attribute & gb_tilemap_vertical_flip_mask) ? 0x07 ^ y_fine : y_fine) << 0x01;
                break;
            }
            case 0x03:{
                tile_fetcher->lo = state->vram[tile_fetcher->tile_address + 0x00];
                break;
            }
            case 0x05:{
                tile_fetcher->hi = state->vram[tile_fetcher->tile_address + 0x01];
                break;
            }
        }
        
        if(state->tile_fifo.length && state->object_found_index == 0xFF){
            gb_ppu_render_pixel(ppu);
        }

        if(state->tile_fifo.length == 0x00 && tile_fetcher->step > 0x05){

            if(state->lcdc.tile_enabled || ppu->gb->state.cgb_mode){

                for(uint8_t i = 0x00; i < 0x08; ++i){

                    gb_pixel_fifo_entry_t* entry = state->tile_fifo.data + ((state->tile_fifo.front + state->tile_fifo.length) & 0x07);

                    uint8_t bit = 0x01 << ((tile_fetcher->attribute & gb_tilemap_horizontal_flip_mask) ? i : 0x07 ^ i);

                    entry->palette_index = tile_fetcher->attribute & gb_tilemap_palette_mask;
                    entry->color_index = ((tile_fetcher->hi & bit) ? 0x02 : 0x00) | ((tile_fetcher->lo & bit) ? 0x01 : 0x00);
                    entry->priority = tile_fetcher->attribute & gb_tilemap_priority_mask;
                    
                    state->tile_fifo.length++;
                }
            }
            else{
                state->tile_fifo.length = 0x08;
            }
            
            state->fetch_column++;
            tile_fetcher->step = 0x00;
        }
    }
}


void gb_ppu_clock(gb_ppu_t* ppu,int cycles){
    
    gb_ppu_state_t* state = &ppu->state;

    if(state->lcdc.lcd_enabled){

        gb_event_manager_t* event_manager = &ppu->gb->event_manager;

        while(cycles--){
            state->cycle++;

            if(state->scanline < gb_vblank_scanline){
                gb_ppu_visible_scanline(ppu);

                if(state->status.mode == gb_ppu_oam_mode){
                    gb_ppu_oam_evaluation(ppu);
                }
                else if(state->status.mode == gb_ppu_drawing_mode && !state->fictitious_fetch){

                    gb_ppu_drawing(ppu);

                    if(state->drawn_pixels >= gb_screen_width){

                        state->status.mode = gb_ppu_hblank_mode;

                        state->event_screen_color = gb_event_screen_palette[gb_event_screen_hblank_color];

                        state->vram_write_blocked = false;
                        state->vram_read_blocked = false;

                        state->oam_write_blocked = false;
                        state->oam_read_blocked = false;

                        if(ppu->gb->dma.state.vram_hblank_running){
                            ppu->gb->dma.state.vram_hblank_pending = true;
                        }
                    }
                }
            }
            else{
                gb_ppu_vblank_scanline(ppu);
            }

            state->status.lcy_equals_ly = state->ly == state->_lyc;

            
            gb_ppu_update_irq_line(ppu);


            if(ppu->handlers != NULL){
                gb_ppu_handler_t* handler = ppu->handlers;
                do{
                    if(handler->scanline == state->ly && handler->cycle == state->cycle){
                        handler->callback(handler->userdata);
                    }
                    handler = handler->next;
                }while(handler != NULL);
            }

            
            if(event_manager->enabled){
                uint8_t* pixel = event_manager->screen + (state->scanline * gb_event_screen_pitch) + (state->cycle * gb_event_screen_bytes_per_pixel);

                pixel[0] = state->event_screen_color.r;
                pixel[1] = state->event_screen_color.g;
                pixel[2] = state->event_screen_color.b;
            }
        }
    }
    else{
        state->off_cycle += cycles;
        
        while(state->off_cycle >= gb_frame_cycles){
            
            state->off_cycle -= gb_frame_cycles;

            memset(ppu->current_screen,0xFF,gb_screen_length);

            gb_ppu_swap_frame_buffer(ppu);

            ++state->frame_count;

            gb_frame_timer_clock(&ppu->gb->frame_timer);

            gb_joypad_update(&ppu->gb->joypad);
        }
    }
}


const uint8_t* gb_ppu_get_render_buffer(gb_t* gb){
    gb_ppu_t* ppu = &gb->ppu;
    return ppu->state.screen[!ppu->state.screen_index];
}


void gb_ppu_write_vram(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    
    if(!ppu->state.vram_write_blocked){
        ppu->vram_bank_ptr[address & 0x1FFF] = value;
    }

}

uint8_t gb_ppu_read_vram(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    
    uint8_t value = 0xFF;

    if(!ppu->state.vram_read_blocked){
        value = ppu->vram_bank_ptr[address & 0x1FFF];
    }

    return value;
}

size_t gb_ppu_vram_absolute_address(gb_ppu_t* ppu,uint16_t relative_address){
    return (ppu->state.vram_bank ? 0x2000 : 0x0000) | (relative_address & 0x1FFF);
}


void gb_ppu_write_register(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    gb_ppu_state_t* state = &ppu->state;

    switch(address){
        //LCDC
        case 0xFF40:{
            state->lcdc.tile_enabled = value & 0x01;
            state->lcdc.object_enabled = value & 0x02;
            state->lcdc.object_size = value & 0x04;
            state->lcdc.bg_tilemap_area = value & 0x08;
            state->lcdc.tiledata_area = value & 0x10;
            state->lcdc.window_enabled = value & 0x20;
            state->lcdc.window_tilemap_area = value & 0x40;
            
            bool lcd_enabled = value & 0x80;

            if(state->lcdc.lcd_enabled && !lcd_enabled){

                state->lcdc.lcd_enabled = false;

                state->status.mode = gb_ppu_hblank_mode;

                state->off_cycle = state->ly * gb_scanline_cycles + state->cycle;
                
                state->ly = 0x00;
                state->scanline = 0x00;
                state->cycle = 0x00;

                state->vram_write_blocked = false;
                state->vram_read_blocked = false;

                state->oam_write_blocked = false;
                state->oam_read_blocked = false;
            }
            else if(!state->lcdc.lcd_enabled && lcd_enabled){

                state->lcdc.lcd_enabled = true;

                state->cycle = 0x07;

                state->window_ly = -1;
                
                state->event_screen_color = gb_event_screen_palette[gb_event_screen_hblank_color];

                gb_event_manager_screen_blank(ppu->gb);

                gb_event_manager_swap_frame(ppu->gb);

                state->first_frame = true;
            }

            break;
        }
        //STAT
        case 0xFF41:{
            state->status.hblank_enabled = value & 0x08;
            state->status.vblank_enabled = value & 0x10;
            state->status.oam_enabled = value & 0x20;
            state->status.lyc_enabled = value & 0x40;
            break;
        }
        //SCY
        case 0xFF42:{
            state->scy = value;
            break;
        }
        //SCX
        case 0xFF43:{
            state->scx = value;
            break;
        }
        //LYC
        case 0xFF45:{
            state->lyc = value;
            if(state->_lyc != 0xFFFF){
                state->_lyc = state->lyc;
            }
            break;
        }
        //WY
        case 0xFF4A:{
            state->wy = value;
            break;
        }
        //WX
        case 0xFF4B:{
            state->wx = value;
            break;
        }
    }
}

uint8_t gb_ppu_read_register(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    gb_ppu_state_t *state = &ppu->state;

    uint8_t value = 0xFF;

    switch(address){
        //LCDC
        case 0xFF40:{
            value = (
                (state->lcdc.tile_enabled ? 0x01 : 0x00) |
                (state->lcdc.object_enabled ? 0x02 : 0x00) |
                (state->lcdc.object_size ? 0x04 : 0x00) |
                (state->lcdc.bg_tilemap_area ? 0x08 : 0x00) |
                (state->lcdc.tiledata_area ? 0x10 : 0x00) |
                (state->lcdc.window_enabled ? 0x20 : 0x00) |
                (state->lcdc.window_tilemap_area ? 0x40 : 0x00) |
                (state->lcdc.lcd_enabled ? 0x80 : 0x00)
            );
            break;
        }
        //STAT
        case 0xFF41:{
            value = (
                (state->status.mode & 0x03) |
                (state->status.lcy_equals_ly ? 0x04 : 0x00) |
                (state->status.hblank_enabled ? 0x08 : 0x00) |
                (state->status.vblank_enabled ? 0x10 : 0x00) |
                (state->status.oam_enabled ? 0x20 : 0x00) |
                (state->status.lyc_enabled ? 0x40 : 0x00) |
                0x80
            );
            break;
        }
        //SCY
        case 0xFF42:{
            value = state->scy;
            break;
        }
        //SCX
        case 0xFF43:{
            value = state->scx;
            break;
        }
        //LY
        case 0xFF44:{
            value = state->ly;
            break;
        }
        //LYC
        case 0xFF45:{
            value = state->lyc;
            break;
        }
        //WY
        case 0xFF4A:{
            value = state->wy;
            break;
        }
        //WX
        case 0xFF4B:{
            value = state->wx;
            break;
        }
    }

    return value;
}


void gb_ppu_write_vbk_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_ppu_t* ppu = (gb_ppu_t*)data;

    ppu->state.vram_bank = value & 0x01;
    ppu->vram_bank_ptr = ppu->state.vram + (ppu->state.vram_bank ? 0x2000 : 0x0000);
}

uint8_t gb_ppu_read_vbk_register(void* data,uint16_t address){
    gb_unused(address);

    gb_ppu_t* ppu = (gb_ppu_t*)data;
    
    return 0xFE | ppu->state.vram_bank;
}


void gb_ppu_write_oam(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    
    if(ppu->gb->dma.state.oam_state != gb_oam_dma_state_transfer && !ppu->state.oam_write_blocked){
        ppu->state.oam[address & 0xFF] = value;
    }
}

uint8_t gb_ppu_read_oam(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;

    uint8_t value = 0xFF;

    if(ppu->gb->dma.state.oam_state != gb_oam_dma_state_transfer && !ppu->state.oam_read_blocked){
        value = ppu->state.oam[address & 0xFF];
    }

    return value;
}

size_t gb_ppu_oam_absolute_address(gb_ppu_t* ppu,uint16_t relative_address){
    gb_unused(ppu);
    return relative_address & 0xFF;
}


void gb_ppu_map(gb_ppu_t* ppu){
    gb_memory_t* memory = &ppu->gb->memory;

    gb_memory_map_in_range(memory,&ppu->vram_descriptor,0x8000,0x9FFF);

    gb_memory_map_in_range(memory,&ppu->register_descriptor,0xFF40,0xFF45);
    gb_memory_map_in_range(memory,&ppu->register_descriptor,0xFF4A,0xFF4B);

    gb_memory_map_in_range(memory,&ppu->oam_descriptor,0xFE00,0xFE9F);
}


void gb_ppu_init_vram_after_skip_boot_dmg(gb_ppu_t* ppu){

    uint8_t* vram = ppu->state.vram;

    // Nintendo

    const uint8_t* logo = gb_cartridge_nintendo_logo(&ppu->gb->cartridge);

    for(int i = 0; i < 0x30; ++i){

        uint8_t value = logo[i];

        uint8_t byte1 = 0x00;
        byte1 |= (value & 0x10) >> 0x03;
        byte1 |= (value & 0x20) >> 0x02;
        byte1 |= (value & 0x40) >> 0x01;
        byte1 |= (value & 0x80) >> 0x00;
        byte1 |= byte1 >> 0x01;

        uint8_t byte2 = 0x00;
        byte2 |= (value & 0x01) << 0x00;
        byte2 |= (value & 0x02) << 0x01;
        byte2 |= (value & 0x04) << 0x02;
        byte2 |= (value & 0x08) << 0x03;
        byte2 |= byte2 << 0x01;

        vram[0x0010 + i * 0x08] = byte1;
        vram[0x0012 + i * 0x08] = byte1;
        vram[0x0014 + i * 0x08] = byte2;
        vram[0x0016 + i * 0x08] = byte2;
    }

    // ®

    static const uint8_t more_vram[8] = {
        0x3C,0x42,0xB9,0xA5,0xB9,0xA5,0x42,0x3C
    };

    for(int i = 0; i < 8; ++i){
        vram[0x0190 + i * 0x02] = more_vram[i];
    }

    // Tilemap

    vram[0x1910] = 0x19;

    for(int i = 0; i < 12; ++i){
        vram[0x1904 + i] = i + 0x01;
        vram[0x1924 + i] = i + 0x0D;
    }
}


void gb_ppu_reset(gb_ppu_t* ppu){

    memset(&ppu->state,0x00,sizeof(ppu->state));

    ppu->current_screen = ppu->state.screen[ppu->state.screen_index];

    ppu->vram_bank_ptr = ppu->state.vram;

}

void gb_ppu_skip_boot(gb_ppu_t* ppu){
    
    gb_ppu_state_t* state = &ppu->state;

    if(ppu->gb->state.is_cgb){
        
        if(ppu->gb->state.cgb_mode){
            state->ly = 144;
            state->scanline = 144;
            state->cycle = 156;
        }
        else{
            //Value based on the Pokemon Red ROM
            state->ly = 147;
            state->scanline = 147;
            state->cycle = 348;
        }
    }
    else{
        state->status.lcy_equals_ly = true;

        state->ly = 0;
        state->scanline = 153;
        state->cycle = 396;

        gb_ppu_init_vram_after_skip_boot_dmg(ppu);
    }

    state->lcdc.tile_enabled = true;
    state->lcdc.tiledata_area = true;
    state->lcdc.lcd_enabled = true;

    state->status.mode = gb_ppu_vblank_mode;

    state->event_screen_color = gb_event_screen_palette[gb_event_screen_vblank_color];
}


void gb_ppu_save_state(gb_ppu_t* ppu,gb_snapshot_t* snapshot){
    snapshot->ppu = ppu->state;
}

void gb_ppu_load_state(gb_ppu_t* ppu,gb_snapshot_t* snapshot){

    ppu->state = snapshot->ppu;

    ppu->current_screen = ppu->state.screen[ppu->state.screen_index];

    ppu->vram_bank_ptr = ppu->state.vram + (ppu->state.vram_bank ? 0x2000 : 0x0000);
}