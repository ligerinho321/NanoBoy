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

    uint8_t* last = ppu->screen[!ppu->screen_index];
    
    uint8_t* current = ppu->screen[ppu->screen_index];
    
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

    gb_atomic_store_explicit(&ppu->screen_index,!ppu->screen_index,gb_memory_order_release);

    ppu->current_screen = ppu->screen[ppu->screen_index];
}


static inline void gb_ppu_init_line_renderer(gb_ppu_t* ppu){
    ppu->wx_enabled = false;

    ppu->tile_fetcher.step = 0x00;
    ppu->object_fetcher.step = 0x00;

    ppu->object_found_index = 0xFF;
    ppu->fetch_window = false;
    ppu->fetch_column = 0x00;
    ppu->drawn_pixels = -0x08 - (ppu->scx & 0x07);
    ppu->fictitious_fetch = true;

    ppu->tile_fifo.length = 0x08;

    ppu->object_fifo.length = 0x00;
    memset(ppu->object_fifo.data,0x00,sizeof(ppu->object_fifo.data));
}

static inline void gb_ppu_visible_scanline(gb_ppu_t* ppu){
    switch(ppu->cycle){
        case 4:{
            ppu->_lyc = ppu->lyc;

            if(ppu->_ly != 0 || !ppu->first_frame){
                ppu->status.mode = gb_ppu_oam_mode;

                ppu->object_buffer_length = 0;
                ppu->oam_address = 0;

                ppu->oam_blocked = true;
            }
            break;
        }
        case 84:{
            ppu->status.mode = gb_ppu_drawing_mode;

            ppu->vram_blocked = true;
            ppu->oam_blocked = true;

            gb_ppu_init_line_renderer(ppu);
            break;
        }
        case 89:{
            ppu->fictitious_fetch = false;
            break;
        }
        case 456:{
            ppu->cycle = 0;
            ppu->_ly++;
            ppu->ly = ppu->_ly;

            ppu->_lyc = 0xFFFF;

            if(!ppu->wy_enabled){
                ppu->wy_enabled = ppu->ly == ppu->wy;
            }

            break;
        }
    }
}

static inline void gb_ppu_vblank_scanline(gb_ppu_t* ppu){
    switch(ppu->cycle){
        case 4:{
            ppu->_lyc = ppu->lyc;

            if(ppu->_ly == 144){
                ppu->first_frame = false;

                gb_ppu_swap_frame_buffer(ppu);

                ++ppu->frame_count;

                gb_frame_timer_clock(&ppu->gb->frame_timer);

                gb_joypad_update(&ppu->gb->joypad);

                ppu->wy_enabled = false;
                
                ppu->status.mode = gb_ppu_vblank_mode;

                ppu->gb->interrupt.flag |= gb_interrupt_vblank_flag;
            }
            break;
        }
        case 5:{
            if(ppu->_ly == 153){
                ppu->ly = 0;
                ppu->_lyc = 0xFFFF;
            }
            break;
        }
        case 12:{
            if(ppu->_ly == 153){
                ppu->_lyc = ppu->lyc;
            }
            break;
        }
        case 456:{
            ppu->cycle = 0;
            ppu->_ly++;

            if(ppu->_ly >= 154){
                ppu->_ly = 0;

                ppu->wy_enabled = ppu->ly == ppu->wy;
                ppu->window_ly = -1;
            }
            else{
                ppu->_lyc = 0xFFFF;
            }

            ppu->ly = ppu->_ly;
        }
        break;
    }
}

static inline void gb_ppu_update_irq_line(gb_ppu_t* ppu){
    
    //No primeiro frame quando a PPU é ligada a primeira scanline começa no modo 0 inves do 2 e vai diretamente para o 3
    //e durante o modo 0 mesmo que STAT bit3 esteja definido STAT IF não é ativado

    bool new_irq_line = (
        (ppu->status.lcy_equals_ly && ppu->status.lyc_enabled) ||
        (ppu->status.mode == gb_ppu_hblank_mode && (ppu->status.hblank_enabled && !(ppu->first_frame && ppu->ly == 0 && ppu->cycle < 84))) ||
        (ppu->status.mode == gb_ppu_vblank_mode && (ppu->status.vblank_enabled || (ppu->ly == 144 && ppu->cycle == 4 && ppu->status.oam_enabled))) || 
        (ppu->status.mode == gb_ppu_oam_mode && ppu->status.oam_enabled)
    );

    if(!ppu->status_irq_line && new_irq_line){
        ppu->gb->interrupt.flag |= gb_interrupt_lcd_flag;
    }

    ppu->status_irq_line = new_irq_line;
}

static inline void gb_ppu_oam_evaluation(gb_ppu_t* ppu){
    if(!(ppu->cycle & 0x01) || ppu->object_buffer_length >= 0x0A) return;

    uint8_t ly = ppu->ly + 0x10;
    uint8_t object_height = ppu->lcdc.object_size ? gb_object_max_height : gb_object_min_height;
    gb_object_t* object = (gb_object_t*)(ppu->oam + ppu->oam_address);

    if(ly >= object->y && ly < (object->y + object_height)){
        memcpy(ppu->object_buffer + ppu->object_buffer_length++,object,0x04);
    }

    ppu->oam_address += 0x04;
}


static inline void gb_ppu_tile_fetcher_step(gb_ppu_t* ppu){

    switch(ppu->tile_fetcher.step++){
        case 0x01:{
            uint8_t y_fine = 0x00;
            uint16_t map_address = 0x0000;

            if(ppu->fetch_window){
                uint8_t x = ppu->fetch_column & 0x1F;
                uint8_t y = (ppu->window_ly >> 0x03) & 0x1F;

                y_fine = ppu->window_ly & 0x07;
                map_address = (ppu->lcdc.window_tilemap_area ? 0x1C00 : 0x1800) | (y << 0x05) | x;
            }
            else{
                uint8_t x = (ppu->fetch_column + (ppu->scx >> 0x03)) & 0x1F;
                uint8_t y = ((ppu->ly + ppu->scy) >> 0x03) & 0x1F;

                y_fine = (ppu->ly + ppu->scy) & 0x07;
                map_address = (ppu->lcdc.bg_tilemap_area ? 0x1C00 : 0x1800) | (y << 0x05) | x;
            }
            
            uint8_t tile_index = ppu->vram[map_address];

            ppu->tile_fetcher.attribute = ppu->gb->cgb_mode ? ppu->vram[0x2000 | map_address] : 0x00;
            
            ppu->tile_fetcher.tile_address = (ppu->tile_fetcher.attribute & gb_tilemap_tile_bank_mask) ? 0x2000 : 0x0000;
            ppu->tile_fetcher.tile_address |= ppu->lcdc.tiledata_area ? tile_index << 0x04 : 0x1000 + ((int8_t)tile_index << 0x04);
            ppu->tile_fetcher.tile_address |= ((ppu->tile_fetcher.attribute & gb_tilemap_vertical_flip_mask) ? 0x07 ^ y_fine : y_fine) << 0x01;
            break;
        }
        case 0x03:{
            ppu->tile_fetcher.lo = ppu->vram[ppu->tile_fetcher.tile_address + 0x00];
            break;
        }
        case 0x05:{
            ppu->tile_fetcher.hi = ppu->vram[ppu->tile_fetcher.tile_address + 0x01];
            break;
        }
    }
    
    if(ppu->tile_fifo.length == 0x00 && ppu->tile_fetcher.step > 0x05){

        if(ppu->lcdc.tile_enabled || ppu->gb->cgb_mode){

            for(uint8_t i = 0x00; i < 0x08; ++i){

                gb_pixel_fifo_entry_t* entry = ppu->tile_fifo.data + ((ppu->tile_fifo.front + ppu->tile_fifo.length) & 0x07);

                uint8_t bit = 0x01 << ((ppu->tile_fetcher.attribute & gb_tilemap_horizontal_flip_mask) ? i : 0x07 ^ i);

                entry->palette_index = ppu->tile_fetcher.attribute & gb_tilemap_palette_mask;
                entry->color_index = ((ppu->tile_fetcher.hi & bit) ? 0x02 : 0x00) | ((ppu->tile_fetcher.lo & bit) ? 0x01 : 0x00);
                entry->priority = ppu->tile_fetcher.attribute & gb_tilemap_priority_mask;
                
                ppu->tile_fifo.length++;
            }
        }
        else{
            ppu->tile_fifo.length = 0x08;
        }
        
        ppu->fetch_column++;
        ppu->tile_fetcher.step = 0x00;
    }
}

static inline void gb_ppu_object_fetcher_step(gb_ppu_t* ppu){
    switch(ppu->object_fetcher.step++){
        case 0x01:{
            gb_object_t* sprite = ppu->object_buffer + ppu->object_found_index;
            
            uint8_t y = (ppu->ly + 0x10) - sprite->y;

            ppu->object_fetcher.tile_address = (ppu->gb->cgb_mode && (sprite->attribute & gb_object_tile_bank_mask)) ? 0x2000 : 0x0000;
            ppu->object_fetcher.tile_address |= (sprite->tile_index & (ppu->lcdc.object_size ? 0xFE : 0xFF)) << 0x04;
            ppu->object_fetcher.tile_address |= ((sprite->attribute & gb_object_vertical_flip_mask) ? ((ppu->lcdc.object_size ? 0x0F : 0x07) ^ y) : y) << 0x01;
            break;
        }
        case 0x03:{
            ppu->object_fetcher.lo = ppu->vram[ppu->object_fetcher.tile_address + 0x00];
            break;
        }
        case 0x05:{
            ppu->object_fetcher.hi = ppu->vram[ppu->object_fetcher.tile_address + 0x01];

            gb_object_t* sprite = ppu->object_buffer + ppu->object_found_index;

            for(uint8_t i = 0x00; i < 0x08; ++i){
                uint8_t bit = 0x01 << ((sprite->attribute & gb_object_horizontal_flip_mask) ? i : 0x07 ^ i);
                uint8_t color_index = ((ppu->object_fetcher.hi & bit) ? 0x02 : 0x00) | ((ppu->object_fetcher.lo & bit) ? 0x01 : 0x00);

                gb_pixel_fifo_entry_t* entry = ppu->object_fifo.data + ((ppu->object_fifo.front + i) & 0x07);

                if(color_index && (!entry->color_index || (!ppu->gb->obj_priority_mode && ppu->object_found_index < entry->index))){
                    if(ppu->gb->cgb_mode){
                        entry->palette_index = sprite->attribute & gb_object_cgb_palette_mask;
                    }
                    else{
                        entry->palette_index = (sprite->attribute & gb_object_dmg_palette_mask) ? 0x01 : 0x00;
                    }
                    entry->color_index = color_index;
                    entry->priority = sprite->attribute & gb_object_priority_mask;
                    entry->index = ppu->object_found_index;
                }
            }

            ppu->object_fetcher.step = 0x00;
            ppu->object_fifo.length = 0x08;
            ppu->object_found_index = 0xFF;
            sprite->x = 0xFF;
            break;
        }
    }
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

    if(!ppu->tile_fifo.length || ppu->object_found_index != 0xFF) return;

    //No primeiro frame apos a PPU ser ligada os pixels não são enviados para a tela segundo o teste firstwhite.gb

    if(ppu->drawn_pixels >= 0 && !ppu->first_frame){

        gb_pixel_fifo_entry_t* tile = ppu->tile_fifo.data + ppu->tile_fifo.front;
        gb_pixel_fifo_entry_t* object = ppu->object_fifo.data + ppu->object_fifo.front;

        gb_rgb_t color = {0};

        gb_palette_t* palette = &ppu->gb->palette;

        if(ppu->lcdc.object_enabled && object->color_index && (!tile->color_index || !ppu->lcdc.tile_enabled || (!tile->priority && !object->priority))){
            if(ppu->gb->is_cgb){
                if(ppu->gb->cgb_mode){
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
            if(ppu->gb->is_cgb){
                if(ppu->gb->cgb_mode){
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

        uint8_t* pixel = ppu->current_screen + ppu->_ly * gb_screen_pitch + ppu->drawn_pixels * gb_screen_bytes_per_pixel;
        
        pixel[0] = color.r;
        pixel[1] = color.g;
        pixel[2] = color.b;
    }

    ppu->drawn_pixels++;

    gb_pixel_fifo_pop(&ppu->tile_fifo);
    gb_pixel_fifo_pop(&ppu->object_fifo);
}


static inline void gb_ppu_drawing(gb_ppu_t* ppu){
    if(ppu->fictitious_fetch) return;

    if(!ppu->wx_enabled){
        ppu->wx_enabled = ppu->drawn_pixels == (ppu->wx - 0x07);

        bool fetch_window = ppu->lcdc.window_enabled && ppu->wx_enabled && ppu->wy_enabled;

        if(ppu->fetch_window != fetch_window){
            ppu->window_ly++;
            ppu->tile_fetcher.step = 0;
            ppu->fetch_window = fetch_window;
            ppu->fetch_column = 0;
            ppu->tile_fifo.length = 0;
        }
    }

    //O buscador de sprite espera que o fifo de tiles não esteja vazio
    //O buscador de sprite espera até que o buscador de tile termine para começar a sua busca
    //O primero ciclo do buscador de sprite se sobrepoem ao ultimo ciclo do buscador de tile

    if(ppu->object_found_index == 0xFF){
        for(uint8_t i = 0x00; i < ppu->object_buffer_length; ++i){
            if((int)ppu->object_buffer[i].x - 0x08 == ppu->drawn_pixels){
                ppu->object_found_index = i;
                break;
            }
        }
    }

    if(ppu->object_found_index != 0xFF && ppu->tile_fetcher.step >= 0x05 && ppu->tile_fifo.length > 0x00){
        gb_ppu_object_fetcher_step(ppu);
    }
    else{
        gb_ppu_tile_fetcher_step(ppu);

        gb_ppu_render_pixel(ppu);
    }
}


void gb_ppu_clock(gb_ppu_t* ppu,int cycles){
    if(ppu->lcdc.lcd_enabled){
        while(cycles--){
            ppu->cycle++;

            if(ppu->_ly < gb_vblank_scanline){
                gb_ppu_visible_scanline(ppu);
            }
            else{
                gb_ppu_vblank_scanline(ppu);
            }

            if(ppu->status.mode == gb_ppu_oam_mode){
                gb_ppu_oam_evaluation(ppu);
            }
            else if(ppu->status.mode == gb_ppu_drawing_mode){

                gb_ppu_drawing(ppu);

                if(ppu->drawn_pixels >= gb_screen_width){
                    ppu->status.mode = gb_ppu_hblank_mode;

                    ppu->vram_blocked = false;
                    ppu->oam_blocked = false;

                    if(ppu->gb->dma.vram_hblank_running){
                        ppu->gb->dma.vram_hblank_pending = true;
                    }
                }
            }

            ppu->status.lcy_equals_ly = ppu->ly == ppu->_lyc;

            gb_ppu_update_irq_line(ppu);

            if(ppu->handlers != NULL){
                gb_ppu_handler_t* handler = ppu->handlers;
                do{
                    if(handler->scanline == ppu->ly && handler->cycle == ppu->cycle){
                        handler->callback(handler->userdata);
                    }
                    handler = handler->next;
                }while(handler != NULL);
            }
        }
    }
    else{
        ppu->off_cycle += cycles;
        
        while(ppu->off_cycle >= gb_frame_cycles){
            
            ppu->off_cycle -= gb_frame_cycles;

            memset(ppu->current_screen,0xFF,gb_screen_length);

            gb_ppu_swap_frame_buffer(ppu);

            ++ppu->frame_count;

            gb_frame_timer_clock(&ppu->gb->frame_timer);

            gb_joypad_update(&ppu->gb->joypad);
        }
    }
}


const uint8_t* gb_ppu_get_render_buffer(gb_ppu_t* ppu){
    bool screen_index = gb_atomic_load_explicit(&ppu->screen_index,gb_memory_order_acquire);
    return ppu->screen[!screen_index];
}


void gb_ppu_write_vram(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    if(!ppu->vram_blocked){
        ppu->vram_bank_ptr[address & 0x1FFF] = value;
    }
}

uint8_t gb_ppu_read_vram(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    if(!ppu->vram_blocked){
        return ppu->vram_bank_ptr[address & 0x1FFF];
    }
    return 0xFF;
}

size_t gb_ppu_vram_absolute_address(gb_ppu_t* ppu,uint16_t relative_address){
    return (ppu->vram_bank ? 0x2000 : 0x0000) | (relative_address & 0x1FFF);
}


void gb_ppu_write_register(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;

    switch(address){
        //LCDC
        case 0xFF40:{
            ppu->lcdc.tile_enabled = value & 0x01;
            ppu->lcdc.object_enabled = value & 0x02;
            ppu->lcdc.object_size = value & 0x04;
            ppu->lcdc.bg_tilemap_area = value & 0x08;
            ppu->lcdc.tiledata_area = value & 0x10;
            ppu->lcdc.window_enabled = value & 0x20;
            ppu->lcdc.window_tilemap_area = value & 0x40;
            
            bool lcd_enabled = value & 0x80;

            if(ppu->lcdc.lcd_enabled && !lcd_enabled){

                ppu->lcdc.lcd_enabled = false;

                ppu->status.mode = gb_ppu_hblank_mode;

                ppu->off_cycle = ppu->ly * gb_scanline_cycles + ppu->cycle;
                
                ppu->ly = 0x00;
                ppu->_ly = 0x00;
                ppu->cycle = 0x00;

                ppu->vram_blocked = false;
                ppu->oam_blocked = false;
            }
            else if(!ppu->lcdc.lcd_enabled && lcd_enabled){

                ppu->lcdc.lcd_enabled = true;

                //Quando a PPU é ligado a linha 0 é mais curta em 5 T-cycles
                ppu->cycle = 0x04;

                ppu->first_frame = true;
            }

            break;
        }
        //STAT
        case 0xFF41:{
            ppu->status.hblank_enabled = value & 0x08;
            ppu->status.vblank_enabled = value & 0x10;
            ppu->status.oam_enabled = value & 0x20;
            ppu->status.lyc_enabled = value & 0x40;
            break;
        }
        //SCY
        case 0xFF42:{
            ppu->scy = value;
            break;
        }
        //SCX
        case 0xFF43:{
            ppu->scx = value;
            break;
        }
        //LYC
        case 0xFF45:{
            ppu->lyc = value;
            if(ppu->_lyc != 0xFFFF){
                ppu->_lyc = ppu->lyc;
            }
            break;
        }
        //WY
        case 0xFF4A:{
            ppu->wy = value;
            break;
        }
        //WX
        case 0xFF4B:{
            ppu->wx = value;
            break;
        }
    }
}

uint8_t gb_ppu_read_register(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        //LCDC
        case 0xFF40:{
            value = (
                (ppu->lcdc.tile_enabled ? 0x01 : 0x00) |
                (ppu->lcdc.object_enabled ? 0x02 : 0x00) |
                (ppu->lcdc.object_size ? 0x04 : 0x00) |
                (ppu->lcdc.bg_tilemap_area ? 0x08 : 0x00) |
                (ppu->lcdc.tiledata_area ? 0x10 : 0x00) |
                (ppu->lcdc.window_enabled ? 0x20 : 0x00) |
                (ppu->lcdc.window_tilemap_area ? 0x40 : 0x00) |
                (ppu->lcdc.lcd_enabled ? 0x80 : 0x00)
            );
            break;
        }
        //STAT
        case 0xFF41:{
            value = (
                (ppu->status.mode & 0x03) |
                (ppu->status.lcy_equals_ly ? 0x04 : 0x00) |
                (ppu->status.hblank_enabled ? 0x08 : 0x00) |
                (ppu->status.vblank_enabled ? 0x10 : 0x00) |
                (ppu->status.oam_enabled ? 0x20 : 0x00) |
                (ppu->status.lyc_enabled ? 0x40 : 0x00) |
                0x80
            );
            break;
        }
        //SCY
        case 0xFF42:{
            value = ppu->scy;
            break;
        }
        //SCX
        case 0xFF43:{
            value = ppu->scx;
            break;
        }
        //LY
        case 0xFF44:{
            value = ppu->ly;
            break;
        }
        //LYC
        case 0xFF45:{
            value = ppu->lyc;
            break;
        }
        //WY
        case 0xFF4A:{
            value = ppu->wy;
            break;
        }
        //WX
        case 0xFF4B:{
            value = ppu->wx;
            break;
        }
    }

    return value;
}


void gb_ppu_write_vbk_register(void* data,uint8_t value,uint16_t address){
    gb_unused(address);

    gb_ppu_t* ppu = (gb_ppu_t*)data;
    gb_t* gb = ppu->gb;

    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return;

    ppu->vram_bank = value & 0x01;
    ppu->vram_bank_ptr = ppu->vram + (ppu->vram_bank ? 0x2000 : 0x0000);
}

uint8_t gb_ppu_read_vbk_register(void* data,uint16_t address){
    gb_unused(address);

    gb_ppu_t* ppu = (gb_ppu_t*)data;
    gb_t* gb = ppu->gb;
    
    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return 0xFF;
    
    return 0xFE | (ppu->vram_bank & 0x01);
}


void gb_ppu_write_oam(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    if(ppu->gb->dma.oam_state != gb_oam_dma_state_transfer && !ppu->oam_blocked){
        ppu->oam[address & 0xFF] = value;
    }
}

uint8_t gb_ppu_read_oam(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    if(ppu->gb->dma.oam_state != gb_oam_dma_state_transfer && !ppu->oam_blocked){
        return ppu->oam[address & 0xFF];
    }
    return 0xFF;
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

    gb_memory_map(memory,&ppu->vbk_register_descriptor,0xFF4F);
}


void gb_ppu_init_vram_after_skip_boot_dmg(gb_ppu_t* ppu){

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

        ppu->vram[0x0010 + i * 0x08] = byte1;
        ppu->vram[0x0012 + i * 0x08] = byte1;
        ppu->vram[0x0014 + i * 0x08] = byte2;
        ppu->vram[0x0016 + i * 0x08] = byte2;
    }

    // ®

    static const uint8_t more_vram[8] = {
        0x3C,0x42,0xB9,0xA5,0xB9,0xA5,0x42,0x3C
    };

    for(int i = 0; i < 8; ++i){
        ppu->vram[0x0190 + i * 0x02] = more_vram[i];
    }

    // Tilemap

    ppu->vram[0x1910] = 0x19;

    for(int i = 0; i < 12; ++i){
        ppu->vram[0x1904 + i] = i + 0x01;
        ppu->vram[0x1924 + i] = i + 0x0D;
    }
}


void gb_ppu_reset(gb_ppu_t* ppu){

    memset(&ppu->lcdc,0x00,sizeof(ppu->lcdc));
    memset(&ppu->status,0x00,sizeof(ppu->status));

    ppu->scy = 0x00;
    ppu->scx = 0x00;
    
    ppu->ly = 0x00;
    ppu->_ly = 0x00;
    ppu->cycle = 0x00;

    ppu->lyc = 0x00;
    ppu->_lyc = 0x00;

    ppu->wy = 0x00;
    ppu->wx = 0x00;
    ppu->wy_enabled = false;
    ppu->wx_enabled = false;
    ppu->window_ly = -1;

    memset(&ppu->tile_fetcher,0x00,sizeof(ppu->tile_fetcher));
    memset(&ppu->object_fetcher,0x00,sizeof(ppu->object_fetcher));

    ppu->object_found_index = 0x00;
    ppu->fetch_window = false;
    ppu->fetch_column = 0x00;
    ppu->drawn_pixels = 0x00;
    ppu->fictitious_fetch = false;

    memset(ppu->object_buffer,0x00,sizeof(ppu->object_buffer));
    ppu->object_buffer_length = 0x00;

    memset(&ppu->tile_fifo,0x00,sizeof(ppu->tile_fifo));
    memset(&ppu->object_fifo,0x00,sizeof(ppu->object_fifo));

    ppu->status_irq_line = false;
    
    ppu->off_cycle = 0;

    ppu->frame_count = 0;
    ppu->first_frame = false;

    memset(ppu->screen[ppu->screen_index],0xFF,gb_screen_length);
    ppu->screen_index = !ppu->screen_index;
    ppu->current_screen = ppu->screen[ppu->screen_index];

    memset(ppu->vram,0x00,sizeof(ppu->vram));
    ppu->vram_bank_ptr = ppu->vram;
    ppu->vram_bank = 0x00;
    ppu->vram_blocked = false;

    memset(ppu->oam,0x00,sizeof(ppu->oam));
    ppu->oam_address = 0x00;
    ppu->oam_blocked = false;

}

void gb_ppu_skip_boot(gb_ppu_t* ppu){
    
    if(ppu->gb->is_cgb){
        
        if(ppu->gb->cgb_mode){
            ppu->ly = 144;
            ppu->_ly = 144;
            ppu->cycle = 156;
        }
        else{
            //Value based on the Pokemon Red ROM
            ppu->ly = 147;
            ppu->_ly = 147;
            ppu->cycle = 348;
        }
    }
    else{
        ppu->status.lcy_equals_ly = true;

        ppu->ly = 0;
        ppu->_ly = 153;
        ppu->cycle = 396;

        gb_ppu_init_vram_after_skip_boot_dmg(ppu);
    }

    ppu->lcdc.tile_enabled = true;
    ppu->lcdc.tiledata_area = true;
    ppu->lcdc.lcd_enabled = true;

    ppu->status.mode = 0x01;
}


void gb_ppu_save_state(gb_ppu_t* ppu,gb_state_t* state){
    gb_state_write(state,ppu->lcdc);
    gb_state_write(state,ppu->status);

    gb_state_write(state,ppu->scy);
    gb_state_write(state,ppu->scx);

    gb_state_write(state,ppu->ly);
    gb_state_write(state,ppu->_ly);
    gb_state_write(state,ppu->cycle);

    gb_state_write(state,ppu->lyc);
    gb_state_write(state,ppu->_lyc);

    gb_state_write(state,ppu->wy);
    gb_state_write(state,ppu->wx);
    gb_state_write(state,ppu->wy_enabled);
    gb_state_write(state,ppu->wx_enabled);
    gb_state_write(state,ppu->window_ly);

    gb_state_write(state,ppu->tile_fetcher);
    gb_state_write(state,ppu->object_fetcher);

    gb_state_write(state,ppu->object_found_index);
    gb_state_write(state,ppu->fetch_window);
    gb_state_write(state,ppu->fetch_column);
    gb_state_write(state,ppu->drawn_pixels);
    gb_state_write(state,ppu->fictitious_fetch);

    gb_state_write_ex(state,ppu->object_buffer,sizeof(ppu->object_buffer));
    gb_state_write(state,ppu->object_buffer_length);
    
    gb_state_write(state,ppu->tile_fifo);
    gb_state_write(state,ppu->object_fifo);

    gb_state_write(state,ppu->status_irq_line);

    gb_state_write(state,ppu->off_cycle);
    
    gb_state_write(state,ppu->frame_count);
    gb_state_write(state,ppu->first_frame);

    gb_state_write(state,ppu->screen_index);
    gb_state_write_ex(state,ppu->screen,sizeof(ppu->screen));

    gb_state_write_ex(state,ppu->vram,sizeof(ppu->vram));
    gb_state_write(state,ppu->vram_bank);
    gb_state_write(state,ppu->vram_blocked);

    gb_state_write_ex(state,ppu->oam,sizeof(ppu->oam));
    gb_state_write(state,ppu->oam_address);
    gb_state_write(state,ppu->oam_blocked);
}

void gb_ppu_load_state(gb_ppu_t* ppu,gb_state_t* state){
    gb_state_read(state,ppu->lcdc);
    gb_state_read(state,ppu->status);

    gb_state_read(state,ppu->scy);
    gb_state_read(state,ppu->scx);
    
    gb_state_read(state,ppu->ly);
    gb_state_read(state,ppu->_ly);
    gb_state_read(state,ppu->cycle);
    
    gb_state_read(state,ppu->lyc);
    gb_state_read(state,ppu->_lyc);
    
    gb_state_read(state,ppu->wy);
    gb_state_read(state,ppu->wx);
    gb_state_read(state,ppu->wy_enabled);
    gb_state_read(state,ppu->wx_enabled);
    gb_state_read(state,ppu->window_ly);

    gb_state_read(state,ppu->tile_fetcher);
    gb_state_read(state,ppu->object_fetcher);
    
    gb_state_read(state,ppu->object_found_index);
    gb_state_read(state,ppu->fetch_window);
    gb_state_read(state,ppu->fetch_column);
    gb_state_read(state,ppu->drawn_pixels);
    gb_state_read(state,ppu->fictitious_fetch);

    gb_state_read_ex(state,ppu->object_buffer,sizeof(ppu->object_buffer));
    gb_state_read(state,ppu->object_buffer_length);
    
    gb_state_read(state,ppu->tile_fifo);
    gb_state_read(state,ppu->object_fifo);

    gb_state_read(state,ppu->status_irq_line);

    gb_state_read(state,ppu->off_cycle);
    
    gb_state_read(state,ppu->frame_count);
    gb_state_read(state,ppu->first_frame);
    
    gb_state_read(state,ppu->screen_index);
    gb_state_read_ex(state,ppu->screen,sizeof(ppu->screen));
    ppu->current_screen = ppu->screen[ppu->screen_index];

    gb_state_read_ex(state,ppu->vram,sizeof(ppu->vram));
    gb_state_read(state,ppu->vram_bank);
    gb_state_read(state,ppu->vram_blocked);
    ppu->vram_bank_ptr = ppu->vram + (ppu->vram_bank ? 0x2000 : 0x0000);

    gb_state_read_ex(state,ppu->oam,sizeof(ppu->oam));
    gb_state_read(state,ppu->oam_address);
    gb_state_read(state,ppu->oam_blocked);
}