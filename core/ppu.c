#include "ppu.h"
#include "gb.h"

void gb_ppu_init(gb_ppu_t* ppu,gb_t* gb){
    ppu->gb = gb;

    gb_ppu_map_vram(ppu);

    gb_ppu_map_registers(ppu);

    gb_ppu_map_oam(ppu);
    
    ppu->vbk_register_handler = (gb_memory_handler_t){
        gb_ppu_write_vbk_register,
        gb_ppu_read_vbk_register,
        ppu
    };
}


void gb_ppu_clear_screen(gb_ppu_t* ppu){
    memset(ppu->screen,0xFF,sizeof(ppu->screen));
}

void gb_ppu_init_line_renderer(gb_ppu_t* ppu){
    ppu->wx_enabled = false;

    ppu->tile_fetcher.step = 0x00;
    ppu->sprite_fetcher.step = 0x00;

    ppu->sprite_found_index = 0xFF;
    ppu->fetch_window = false;
    ppu->fetch_column = 0x00;
    ppu->drawn_pixels = -0x08 - (ppu->scx & 0x07);
    ppu->fictitious_fetch = true;

    ppu->tile_fifo.length = 0x08;

    ppu->sprite_fifo.length = 0x00;
    memset(ppu->sprite_fifo.data,0x00,sizeof(ppu->sprite_fifo.data));
}

void gb_ppu_visible_scanline(gb_ppu_t* ppu){
    switch(ppu->cycle){
        case 4:{
            ppu->_lyc = ppu->lyc;

            if(ppu->_ly != 0 || !ppu->first_frame){
                ppu->status.mode = gb_ppu_oam_mode;

                ppu->sprite_buffer_length = 0;
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

void gb_ppu_vblank_scanline(gb_ppu_t* ppu){
    switch(ppu->cycle){
        case 4:{
            ppu->_lyc = ppu->lyc;

            if(ppu->_ly == 144){
                ppu->frame_count++;
                ppu->first_frame = false;

                gb_joypad_update(&ppu->gb->joypad);

                ppu->status.mode = gb_ppu_vblank_mode;

                ppu->gb->interrupt.flag |= gb_interrupt_vblank_flag;

                ppu->wy_enabled = false;
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
            ppu->ly = ppu->_ly;

            ppu->_lyc = 0xFFFF;

            if(ppu->_ly >= 154){
                ppu->_ly = 0;
                ppu->ly = ppu->_ly;

                ppu->wy_enabled = ppu->ly == ppu->wy;
                ppu->window_ly = -1;
            }
        }
        break;
    }
}

void gb_ppu_update_irq_line(gb_ppu_t* ppu){
    
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

void gb_ppu_oam_evaluation(gb_ppu_t* ppu){
    if(!(ppu->cycle & 0x01) || ppu->sprite_buffer_length >= 0x0A) return;

    uint8_t ly = ppu->ly + 0x10;
    uint8_t sprite_height = ppu->lcdc.sprite_size ? 0x10 : 0x08;
    gb_sprite_t* sprite = (gb_sprite_t*)(ppu->oam + ppu->oam_address);

    if(ly >= sprite->y && ly < (sprite->y + sprite_height)){
        memcpy(ppu->sprite_buffer + ppu->sprite_buffer_length++,sprite,0x04);
    }

    ppu->oam_address += 0x04;
}

void gb_ppu_drawing(gb_ppu_t* ppu){
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

    if(ppu->sprite_found_index == 0xFF){
        for(uint8_t i = 0x00; i < ppu->sprite_buffer_length; ++i){
            if((int)ppu->sprite_buffer[i].x - 0x08 == ppu->drawn_pixels){
                ppu->sprite_found_index = i;
                break;
            }
        }
    }

    if(ppu->sprite_found_index != 0xFF && ppu->tile_fetcher.step >= 0x05 && ppu->tile_fifo.length > 0x00){
        gb_ppu_sprite_fetcher_step(ppu);
    }
    else{
        gb_ppu_tile_fetcher_step(ppu);

        gb_ppu_render_pixel(ppu);
    }
}


void gb_ppu_off_clock(gb_ppu_t* ppu,int cycles){
    ppu->off_cycle += cycles;
    while(ppu->off_cycle >= gb_frame_cycles){
        ppu->off_cycle -= gb_frame_cycles;
        ppu->frame_count++;
    }
}

void gb_ppu_on_clock(gb_ppu_t* ppu,int cycles){
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
            }
        }

        ppu->status.lcy_equals_ly = ppu->ly == ppu->_lyc;

        gb_ppu_update_irq_line(ppu);

        if(ppu->gb->callback_handles != NULL){
            gb_ppu_callback_handler_t* handler = ppu->gb->callback_handles;
            do{
                if(handler->scanline == ppu->ly && handler->cycle == ppu->cycle){
                    handler->callback(handler->data);
                }
                handler = handler->next;
            }while(handler != NULL);
        }
    }
}


void gb_ppu_tile_fetcher_step(gb_ppu_t* ppu){

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

            ppu->tile_fetcher.attributes = ppu->gb->cgb_mode ? ppu->vram[0x2000 | map_address] : 0x00;
            
            ppu->tile_fetcher.tile_address = (ppu->tile_fetcher.attributes & 0x08) ? 0x2000 : 0x0000;
            ppu->tile_fetcher.tile_address |= ppu->lcdc.tiledata_area ? tile_index << 0x04 : 0x1000 + ((int8_t)tile_index << 0x04);
            ppu->tile_fetcher.tile_address |= ((ppu->tile_fetcher.attributes & 0x40) ? 0x07 ^ y_fine : y_fine) << 0x01;
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

        if(ppu->lcdc.tile_enabled){

            for(uint8_t i = 0x00; i < 0x08; ++i){

                gb_pixel_fifo_entry_t* entry = ppu->tile_fifo.data + ((ppu->tile_fifo.front + ppu->tile_fifo.length) & 0x07);

                uint8_t bit = 0x01 << ((ppu->tile_fetcher.attributes & 0x20) ? i : 0x07 ^ i);

                entry->palette_index = ppu->tile_fetcher.attributes & 0x07;
                entry->color_index = ((ppu->tile_fetcher.hi & bit) ? 0x02 : 0x00) | ((ppu->tile_fetcher.lo & bit) ? 0x01 : 0x00);
                entry->priority = ppu->tile_fetcher.attributes & 0x80;
                
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

void gb_ppu_sprite_fetcher_step(gb_ppu_t* ppu){
    switch(ppu->sprite_fetcher.step++){
        case 0x01:{
            gb_sprite_t* sprite = ppu->sprite_buffer + ppu->sprite_found_index;
            
            uint8_t tile_index = sprite->tile_index;
            if(ppu->lcdc.sprite_size) tile_index &= 0xFE;

            uint8_t y = (ppu->ly + 0x10) - sprite->y;

            ppu->sprite_fetcher.tile_address = ((sprite->attributes & 0x08) ? 0x2000 : 0x0000) | (tile_index << 0x04);
            ppu->sprite_fetcher.tile_address += ((sprite->attributes & 0x40) ? ((ppu->lcdc.sprite_size ? 0x0F : 0x07) ^ y) : y) << 0x01;
            break;
        }
        case 0x03:{
            ppu->sprite_fetcher.lo = ppu->vram[ppu->sprite_fetcher.tile_address + 0x00];
            break;
        }
        case 0x05:{
            ppu->sprite_fetcher.hi = ppu->vram[ppu->sprite_fetcher.tile_address + 0x01];

            gb_sprite_t* sprite = ppu->sprite_buffer + ppu->sprite_found_index;

            for(uint8_t i = 0x00; i < 0x08; ++i){
                uint8_t bit = 0x01 << ((sprite->attributes & 0x20) ? i : 0x07 ^ i);
                uint8_t color_index = ((ppu->sprite_fetcher.hi & bit) ? 0x02 : 0x00) | ((ppu->sprite_fetcher.lo & bit) ? 0x01 : 0x00);

                gb_pixel_fifo_entry_t* entry = ppu->sprite_fifo.data + ((ppu->sprite_fifo.front + i) & 0x07);

                if(color_index && (!entry->color_index || (ppu->gb->cgb_mode && ppu->sprite_found_index < entry->index))){
                    if(ppu->gb->cgb_mode){
                        entry->palette_index = sprite->attributes & 0x07;
                    }
                    else{
                        entry->palette_index = (sprite->attributes & 0x10) ? 0x01 : 0x00;
                    }
                    entry->color_index = color_index;
                    entry->priority = sprite->attributes & 0x80;
                    entry->index = ppu->sprite_found_index;
                }
            }

            ppu->sprite_fetcher.step = 0x00;
            ppu->sprite_fifo.length = 0x08;
            ppu->sprite_found_index = 0xFF;
            sprite->x = 0xFF;
            break;
        }
    }
}


void gb_ppu_render_pixel(gb_ppu_t* ppu){

    if(!ppu->tile_fifo.length || ppu->sprite_found_index != 0xFF) return;

    //No primeiro frame apos a PPU ser ligada os pixels não são enviados para a tela segundo o teste firstwhite.gb

    if(ppu->drawn_pixels >= 0 && !ppu->first_frame){

        gb_pixel_fifo_entry_t tile = ppu->tile_fifo.data[ppu->tile_fifo.front];
        gb_pixel_fifo_entry_t sprite = ppu->sprite_fifo.data[ppu->sprite_fifo.front];

        gb_rgb_t color = {0};

        if(ppu->lcdc.sprite_enabled && sprite.color_index && (!tile.color_index || !ppu->lcdc.tile_enabled || (!tile.priority && !sprite.priority))){
            if(ppu->gb->type == gb_cgb){
                if(ppu->gb->cgb_mode){
                    color = gb_palette_get_cgb_obp_color(&ppu->gb->palette,sprite.palette_index,sprite.color_index);
                }
                else{
                    color = gb_palette_get_cgb_dmg_obp_color(&ppu->gb->palette,sprite.palette_index,sprite.color_index);
                }
            }
            else{
                color = gb_palette_get_dmg_obp_color(&ppu->gb->palette,sprite.palette_index,sprite.color_index);
            }
        }
        else{
            if(ppu->gb->type == gb_cgb){
                if(ppu->gb->cgb_mode){
                    color = gb_palette_get_cgb_bgp_color(&ppu->gb->palette,tile.palette_index,tile.color_index);
                }
                else{
                    color = gb_palette_get_cgb_dmg_bgp_color(&ppu->gb->palette,tile.color_index);
                }
            }
            else{
                color = gb_palette_get_dmg_bgp_color(&ppu->gb->palette,tile.color_index);
            }
        }

        uint8_t* pixel = ppu->screen + ((ppu->ly * gb_screen_pitch) + (ppu->drawn_pixels * gb_screen_bytes_per_pixel));
        pixel[0] = color.r;
        pixel[1] = color.g;
        pixel[2] = color.b;
    }

    ppu->drawn_pixels++;

    gb_pixel_fifo_pop(&ppu->tile_fifo);
    gb_pixel_fifo_pop(&ppu->sprite_fifo);
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


void gb_ppu_write_register(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;

    switch(address){
        //LCDC
        case 0xFF40:{
            ppu->lcdc.tile_enabled = value & 0x01;
            ppu->lcdc.sprite_enabled = value & 0x02;
            ppu->lcdc.sprite_size = value & 0x04;
            ppu->lcdc.bg_tilemap_area = value & 0x08;
            ppu->lcdc.tiledata_area = value & 0x10;
            ppu->lcdc.window_enabled = value & 0x20;
            ppu->lcdc.window_tilemap_area = value & 0x40;
            
            bool lcd_enabled = value & 0x80;

            if(ppu->lcdc.lcd_enabled && !lcd_enabled){

                ppu->lcdc.lcd_enabled = false;

                ppu->status.mode = gb_ppu_hblank_mode;

                gb_ppu_clear_screen(ppu);
                ppu->off_cycle = ppu->ly * gb_scanline_cycles + ppu->cycle;
                
                ppu->ly = 0x00;
                ppu->_ly = 0x00;
                ppu->cycle = 0x00;

                ppu->vram_blocked = false;
                ppu->oam_blocked = false;

                ppu->clock = gb_ppu_off_clock;
            }
            else if(!ppu->lcdc.lcd_enabled && lcd_enabled){

                ppu->lcdc.lcd_enabled = true;

                //Quando a PPU é ligado a linha 0 é mais curta em 5 T-cycles
                ppu->cycle = 0x04;

                ppu->first_frame = true;

                ppu->clock = gb_ppu_on_clock;
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
                (ppu->lcdc.sprite_enabled ? 0x02 : 0x00) |
                (ppu->lcdc.sprite_size ? 0x04 : 0x00) |
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
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    ppu->vram_bank = value & 0x01;
    ppu->vram_bank_ptr = ppu->vram + (ppu->vram_bank ? 0x2000 : 0x0000);
}

uint8_t gb_ppu_read_vbk_register(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    return 0xFE | (ppu->vram_bank & 0x01);
}


void gb_ppu_write_oam(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    if(!ppu->gb->dma.oam_running && !ppu->oam_blocked){
        ppu->oam[address & 0xFF] = value;
    }
}

uint8_t gb_ppu_read_oam(void* data,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;
    if(!ppu->gb->dma.oam_running && !ppu->oam_blocked){
        return ppu->oam[address & 0xFF];
    }
    return 0xFF;
}


void gb_ppu_map_vram(gb_ppu_t* ppu){

    ppu->vram_handler = (gb_memory_handler_t){
        gb_ppu_write_vram,
        gb_ppu_read_vram,
        ppu
    };

    gb_memory_map(&ppu->gb->memory,&ppu->vram_handler,0x8000,0x9FFF);
}

void gb_ppu_map_registers(gb_ppu_t* ppu){

    ppu->register_handler = (gb_memory_handler_t){
        gb_ppu_write_register,
        gb_ppu_read_register,
        ppu
    };

    gb_memory_handler_t** bus = ppu->gb->memory.bus;

    bus[0xFF40] = &ppu->register_handler;
    bus[0xFF41] = &ppu->register_handler;
    bus[0xFF42] = &ppu->register_handler;
    bus[0xFF43] = &ppu->register_handler;
    bus[0xFF44] = &ppu->register_handler;
    bus[0xFF45] = &ppu->register_handler;
    bus[0xFF4A] = &ppu->register_handler;
    bus[0xFF4B] = &ppu->register_handler;
}

void gb_ppu_map_oam(gb_ppu_t* ppu){

    ppu->oam_handler = (gb_memory_handler_t){
        gb_ppu_write_oam,
        gb_ppu_read_oam,
        ppu
    };

    gb_memory_map(&ppu->gb->memory,&ppu->oam_handler,0xFE00,0xFE9F);
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
    ppu->window_ly = 0x00;

    memset(&ppu->tile_fetcher,0x00,sizeof(ppu->tile_fetcher));
    memset(&ppu->sprite_fetcher,0x00,sizeof(ppu->sprite_fetcher));

    ppu->sprite_found_index = 0x00;
    ppu->fetch_window = false;
    ppu->fetch_column = 0x00;
    ppu->drawn_pixels = 0x00;
    ppu->fictitious_fetch = false;

    memset(ppu->sprite_buffer,0x00,sizeof(ppu->sprite_buffer));
    ppu->sprite_buffer_length = 0x00;

    memset(&ppu->tile_fifo,0x00,sizeof(ppu->tile_fifo));
    memset(&ppu->sprite_fifo,0x00,sizeof(ppu->sprite_fifo));

    memset(ppu->vram,0x00,sizeof(ppu->vram));
    ppu->vram_bank_ptr = ppu->vram;
    ppu->vram_bank = 0x00;
    ppu->vram_blocked = false;

    memset(ppu->oam,0x00,sizeof(ppu->oam));
    ppu->oam_address = 0x00;
    ppu->oam_blocked = false;

    ppu->status_irq_line = false;
    
    ppu->off_cycle = 0;

    ppu->frame_count = 0;
    ppu->first_frame = false;

    gb_ppu_clear_screen(ppu);

    ppu->clock = gb_ppu_off_clock;
}