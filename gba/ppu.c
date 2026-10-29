#include "ppu.h"
#include "gba.h"

void gba_ppu_init(gba_ppu_t* ppu,gba_t* gba){
    ppu->gba = gba;
}


void gba_ppu_add_handler(gba_t* gba,gba_ppu_handler_t* handler){
    gba_list_add_element(gba->ppu.handlers,handler,gba_ppu_handler_t);
}

void gba_ppu_remove_handler(gba_t* gba,gba_ppu_handler_t* handler){
    gba_list_remove_element(gba->ppu.handlers,handler,gba_ppu_handler_t);
}


void gba_ppu_schedule_next_event(gba_ppu_t* ppu){
    
    ppu->last_schedule_event = ppu->gba->cycles;

    ppu->next_schedule_event = ppu->gba->cycles;

    if(ppu->scanline < gba_vblank_start_scanline && ppu->cycle < gba_scanline_hblank_start_cycle){
        ppu->next_schedule_event += gba_scanline_hblank_start_cycle - ppu->cycle;
    }
    else{
        ppu->next_schedule_event += gba_scanline_cycles - ppu->cycle;
    }
}

void gba_ppu_update(gba_ppu_t* ppu){
    uint64_t cycles = ppu->gba->cycles - ppu->last_schedule_event;

    while(cycles--){

        ++ppu->cycle;

        if(ppu->cycle == gba_scanline_hblank_start_cycle){

            ppu->lcd_status.hblank_flag = true;

            if(ppu->lcd_status.hblank_irq_enabled){
                ppu->gba->interrupt.flag.v |= gba_interrupt_lcd_hblank_flag;
            }
        }
        if(ppu->cycle >= gba_scanline_cycles){

            ppu->cycle = 0;
            
            ppu->lcd_status.hblank_flag = false;

            ++ppu->scanline;

            if(ppu->scanline == ppu->lcd_status.lyc){

                ppu->lcd_status.lyc_flag = true;

                if(ppu->lcd_status.lyc_irq_enabled){
                    ppu->gba->interrupt.flag.v |= gba_interrupt_lcd_lyc_flag;
                }
            }
            else{
                ppu->lcd_status.lyc_flag = false;
            }

            if(ppu->scanline == gba_vblank_start_scanline){

                ppu->lcd_status.vblank_flag = true;

                if(ppu->lcd_status.vblank_irq_enabled){
                    ppu->gba->interrupt.flag.v |= gba_interrupt_lcd_vblank_flag;
                }

                ++ppu->frame_count;
                
                gba_keypad_update(&ppu->gba->keypad);
            }
            else if(ppu->scanline >= gba_scanlines){

                ppu->scanline = 0;

                ppu->lcd_status.vblank_flag = false;
            }
        }

        if(ppu->handlers != NULL){
            gba_ppu_handler_t* handler = ppu->handlers;
            do{
                if(handler->scanline == ppu->scanline && handler->cycle == ppu->cycle){
                    handler->callback(handler->userdata);
                }
                handler = handler->next;
            }while(handler != NULL);
        }
    }
}



static inline void gba_ppu_write_lcd_control_lsb(gba_ppu_t* ppu,uint8_t value){
    gba_ppu_lcd_control_t* control = &ppu->lcd_control;
    control->bg_mode = value & 0x07;
    control->frame_select = (value & 0x10) != 0x00;
    control->hblank_interval_free = (value & 0x20) != 0x00;
    control->obj_vram_mapping = (value & 0x40) != 0x00;
    control->forced_blank = (value & 0x80) != 0x00;
}

static inline uint8_t gba_ppu_read_lcd_control_lsb(gba_ppu_t* ppu){
    gba_ppu_lcd_control_t* control = &ppu->lcd_control;
    return (
        (control->bg_mode & 0x07) |
        // TODO: CGB mode
        (control->frame_select ? 0x10 : 0x00) |
        (control->hblank_interval_free ? 0x20 : 0x00) |
        (control->obj_vram_mapping ? 0x40 : 0x00) |
        (control->forced_blank ? 0x80 : 0x00)
    );
}

static inline void gba_ppu_write_lcd_control_msb(gba_ppu_t* ppu,uint8_t value){
    gba_ppu_lcd_control_t* control = &ppu->lcd_control;
    control->bg0_enabled = (value & 0x01) != 0x00;
    control->bg1_enabled = (value & 0x02) != 0x00;
    control->bg2_enabled = (value & 0x04) != 0x00;
    control->bg3_enabled = (value & 0x08) != 0x00;
    control->obj_enabled = (value & 0x10) != 0x00;
    control->window0_enabled = (value & 0x20) != 0x00;
    control->window1_enabled = (value & 0x40) != 0x00;
    control->obj_window_enabled = (value & 0x80) != 0x00;
}

static inline uint8_t gba_ppu_read_lcd_control_msb(gba_ppu_t* ppu){
    gba_ppu_lcd_control_t* control = &ppu->lcd_control;
    return (
        (control->bg0_enabled ? 0x01 : 0x00) |
        (control->bg1_enabled ? 0x02 : 0x00) |
        (control->bg2_enabled ? 0x04 : 0x00) |
        (control->bg3_enabled ? 0x08 : 0x00) |
        (control->obj_enabled ? 0x10 : 0x00) |
        (control->window0_enabled ? 0x20 : 0x00) |
        (control->window1_enabled ? 0x40 : 0x00) |
        (control->obj_window_enabled ? 0x80 : 0x00)
    );
}


static inline void gba_ppu_write_lcd_status_lsb(gba_ppu_t* ppu,uint8_t value){
    gba_ppu_lcd_status_t* status = &ppu->lcd_status;
    status->vblank_irq_enabled = (value & 0x08) != 0x00;
    status->hblank_irq_enabled = (value & 0x10) != 0x00;
    status->lyc_irq_enabled = (value & 0x20) != 0x00;
}

static inline uint8_t gba_ppu_read_lcd_status_lsb(gba_ppu_t* ppu){
    gba_ppu_lcd_status_t* status = &ppu->lcd_status;
    return (
        (status->vblank_flag ? 0x01 : 0x00) |
        (status->hblank_flag ? 0x02 : 0x00) |
        (status->lyc_flag ? 0x04 : 0x00) |
        (status->vblank_irq_enabled ? 0x08 : 0x00) |
        (status->hblank_irq_enabled ? 0x10 : 0x00) |
        (status->lyc_irq_enabled ? 0x20 : 0x00)
    );
}

static inline void gba_ppu_write_lcd_status_msb(gba_ppu_t* ppu,uint8_t value){
    gba_ppu_lcd_status_t* status = &ppu->lcd_status;
    status->lyc = value;
}

static inline uint8_t gba_ppu_read_lcd_status_msb(gba_ppu_t* ppu){
    gba_ppu_lcd_status_t* status = &ppu->lcd_status;
    return status->lyc;
}


static inline void gba_ppu_write_bg_control_lsb(gba_ppu_bg_control_t* control,uint8_t value){
    control->priority = value & 0x03;
    control->char_base_block = (value >> 0x02) & 0x03;
    control->mosaic_enabled = (value & 0x40) != 0x00;
    control->color = (value & 0x80) != 0x00;
}

static inline uint8_t gba_ppu_read_bg_control_lsb(gba_ppu_bg_control_t* control){
    return (
        (control->priority & 0x03) |
        ((control->char_base_block & 0x03) << 0x02) |
        (control->mosaic_enabled ? 0x40 : 0x00) |
        (control->color ? 0x80 : 0x00)
    );
}

static inline void gba_ppu_write_bg_control_msb(gba_ppu_bg_control_t* control,uint8_t value){
    control->screen_base_block = value & 0x1F;
    control->wraparound = (value & 0x20) != 0x00;
    control->screen_size = (value >> 0x06) & 0x03;
}

static inline uint8_t gba_ppu_read_bg_control_msb(gba_ppu_bg_control_t* control){
    return (
        (control->screen_base_block & 0x1F) |
        (control->wraparound ? 0x20 : 0x00) |
        ((control->screen_size & 0x03) << 0x06)
    );
}


static inline void gba_ppu_write_win_control(gba_ppu_win_control_t* control,uint8_t value){
    control->bg0_enabled = (value & 0x01) != 0x00;
    control->bg1_enabled = (value & 0x02) != 0x00;
    control->bg2_enabled = (value & 0x04) != 0x00;
    control->bg3_enabled = (value & 0x08) != 0x00;
    control->obj_enabled = (value & 0x10) != 0x00;
    control->special_effect = (value & 0x20) != 0x00;
}

static inline uint8_t gba_ppu_read_win_control(gba_ppu_win_control_t* control){
    return (
        (control->bg0_enabled ? 0x01 : 0x00) |
        (control->bg1_enabled ? 0x02 : 0x00) |
        (control->bg2_enabled ? 0x04 : 0x00) |
        (control->bg3_enabled ? 0x08 : 0x00) |
        (control->obj_enabled ? 0x10 : 0x00) |
        (control->special_effect ? 0x20 : 0x00)
    );
}


static inline void gba_ppu_write_mosaic_size(gba_ppu_mosaic_size_t* mosaic,uint8_t value){
    mosaic->width = (value >> 0x00) & 0x0F;
    mosaic->height = (value >> 0x04) & 0x0F;
}

static inline uint8_t gba_ppu_read_mosaic_size(gba_ppu_mosaic_size_t* mosaic){
    return (
        ((mosaic->width & 0x0F) << 0x00) |
        ((mosaic->height & 0x0F) << 0x04)
    );
}


static inline void gba_ppu_write_special_effect_lsb(gba_ppu_special_effect_t* control,uint8_t value){
    control->bg0_first_target = (value & 0x01) != 0x00;
    control->bg1_first_target = (value & 0x02) != 0x00;
    control->bg2_first_target = (value & 0x04) != 0x00;
    control->bg3_first_target = (value & 0x08) != 0x00;
    control->obj_first_target = (value & 0x10) != 0x00;
    control->bd_first_target = (value & 0x20) != 0x00;
    control->effect = (value >> 0x06) & 0x03;
}

static inline uint8_t gba_ppu_read_special_effect_lsb(gba_ppu_special_effect_t* control){
    return (
        (control->bg0_first_target ? 0x01 : 0x00) |
        (control->bg1_first_target ? 0x02 : 0x00) |
        (control->bg2_first_target ? 0x04 : 0x00) |
        (control->bg3_first_target ? 0x08 : 0x00) |
        (control->obj_first_target ? 0x10 : 0x00) |
        (control->bd_first_target ? 0x20 : 0x00) |
        ((control->effect & 0x03) << 0x06)
    );
}

static inline void gba_ppu_write_special_effect_msb(gba_ppu_special_effect_t* control,uint8_t value){
    control->bg0_second_target = (value & 0x01) != 0x00;
    control->bg1_second_target = (value & 0x02) != 0x00;
    control->bg2_second_target = (value & 0x04) != 0x00;
    control->bg3_second_target = (value & 0x08) != 0x00;
    control->obj_second_target = (value & 0x10) != 0x00;
    control->bd_second_target = (value & 0x20) != 0x00;
}

static inline uint8_t gba_ppu_read_special_effect_msb(gba_ppu_special_effect_t* control){
    return (
        (control->bg0_second_target ? 0x01 : 0x00) |
        (control->bg1_second_target ? 0x02 : 0x00) |
        (control->bg2_second_target ? 0x04 : 0x00) |
        (control->bg3_second_target ? 0x08 : 0x00) |
        (control->obj_second_target ? 0x10 : 0x00) |
        (control->bd_second_target ? 0x20 : 0x00)
    );
}


void gba_ppu_write_register_raw(gba_ppu_t* ppu,uint8_t value,uint32_t address){

    switch(address){
        // LCD Control
        case 0x04000000: gba_ppu_write_lcd_control_lsb(ppu,value); break;
        case 0x04000001: gba_ppu_write_lcd_control_msb(ppu,value); break;
        // Green Swap
        case 0x04000002: ppu->green_swap = (value & 0x01) != 0x00; break;
        case 0x04000003: break;

        // LCD Status
        case 0x04000004: gba_ppu_write_lcd_status_lsb(ppu,value); break;
        case 0x04000005: gba_ppu_write_lcd_status_msb(ppu,value); break;
        // VCounter
        case 0x04000006: break;
        case 0x04000007: break;

        // BG0 Control
        case 0x04000008: gba_ppu_write_bg_control_lsb(ppu->bg_control + 0,value); break;
        case 0x04000009: gba_ppu_write_bg_control_msb(ppu->bg_control + 0,value & ~0x20); break;
        // BG1 Control
        case 0x0400000A: gba_ppu_write_bg_control_lsb(ppu->bg_control + 1,value); break;
        case 0x0400000B: gba_ppu_write_bg_control_msb(ppu->bg_control + 1,value & ~0x20); break;

        // BG2 Control
        case 0x0400000C: gba_ppu_write_bg_control_lsb(ppu->bg_control + 2,value); break;
        case 0x0400000D: gba_ppu_write_bg_control_msb(ppu->bg_control + 2,value); break;
        // BG3 Control
        case 0x0400000E: gba_ppu_write_bg_control_lsb(ppu->bg_control + 3,value); break;
        case 0x0400000F: gba_ppu_write_bg_control_msb(ppu->bg_control + 3,value); break;
        
        // BG0 X Scrolling
        case 0x04000010: ppu->bg_scroll[0].x.b[0] = value; break;
        case 0x04000011: ppu->bg_scroll[0].x.b[1] = value & 0x01; break;
        // BG0 Y Scrolling
        case 0x04000012: ppu->bg_scroll[0].y.b[0] = value; break;
        case 0x04000013: ppu->bg_scroll[0].y.b[1] = value & 0x01; break;

        // BG1 X Scrolling
        case 0x04000014: ppu->bg_scroll[1].x.b[0] = value; break;
        case 0x04000015: ppu->bg_scroll[1].x.b[1] = value & 0x01; break;
        // BG1 Y Scrolling
        case 0x04000016: ppu->bg_scroll[1].y.b[0] = value; break;
        case 0x04000017: ppu->bg_scroll[1].y.b[1] = value & 0x01; break;

        // BG2 X Scrolling
        case 0x04000018: ppu->bg_scroll[2].x.b[0] = value; break;
        case 0x04000019: ppu->bg_scroll[2].x.b[1] = value & 0x01; break;
        // BG2 Y Scrolling
        case 0x0400001A: ppu->bg_scroll[2].y.b[0] = value; break;
        case 0x0400001B: ppu->bg_scroll[2].y.b[1] = value & 0x01; break;

        // BG3 X Scrolling
        case 0x0400001C: ppu->bg_scroll[3].x.b[0] = value; break;
        case 0x0400001D: ppu->bg_scroll[3].x.b[1] = value & 0x01; break;
        // BG3 Y Scrolling
        case 0x0400001E: ppu->bg_scroll[3].y.b[0] = value; break;
        case 0x0400001F: ppu->bg_scroll[3].y.b[1] = value & 0x01; break;

        // BG2 DX
        case 0x04000020: ppu->bg2_affine.dx.b[0] = value; break;
        case 0x04000021: ppu->bg2_affine.dx.b[1] = value; break;
        // BG2 DMX
        case 0x04000022: ppu->bg2_affine.dmx.b[0] = value; break;
        case 0x04000023: ppu->bg2_affine.dmx.b[1] = value; break;

        // BG2 DY
        case 0x04000024: ppu->bg2_affine.dy.b[0] = value; break;
        case 0x04000025: ppu->bg2_affine.dy.b[1] = value; break;
        // BG2 DMY
        case 0x04000026: ppu->bg2_affine.dmy.b[0] = value; break;
        case 0x04000027: ppu->bg2_affine.dmy.b[1] = value; break;

        // BG2 X
        case 0x04000028: ppu->bg2_affine.x.b[0] = value; break;
        case 0x04000029: ppu->bg2_affine.x.b[1] = value; break;
        case 0x0400002A: ppu->bg2_affine.x.b[2] = value; break;
        case 0x0400002B: ppu->bg2_affine.x.b[3] = value & 0x0F; break;

        // BG2 Y
        case 0x0400002C: ppu->bg2_affine.y.b[0] = value; break;
        case 0x0400002D: ppu->bg2_affine.y.b[1] = value; break;
        case 0x0400002E: ppu->bg2_affine.y.b[2] = value; break;
        case 0x0400002F: ppu->bg2_affine.y.b[3] = value & 0x0F; break;

        // BG3 DX
        case 0x04000030: ppu->bg3_affine.dx.b[0] = value; break;
        case 0x04000031: ppu->bg3_affine.dx.b[1] = value; break;
        // BG3 DMX
        case 0x04000032: ppu->bg3_affine.dmx.b[0] = value; break;
        case 0x04000033: ppu->bg3_affine.dmx.b[1] = value; break;
        
        // BG3 DY
        case 0x04000034: ppu->bg3_affine.dy.b[0] = value; break;
        case 0x04000035: ppu->bg3_affine.dy.b[1] = value; break;
        // BG3 DMY
        case 0x04000036: ppu->bg3_affine.dmy.b[0] = value; break;
        case 0x04000037: ppu->bg3_affine.dmy.b[1] = value; break;

        // BG3 X
        case 0x04000038: ppu->bg3_affine.x.b[0] = value; break;
        case 0x04000039: ppu->bg3_affine.x.b[1] = value; break;
        case 0x0400003A: ppu->bg3_affine.x.b[2] = value; break;
        case 0x0400003B: ppu->bg3_affine.x.b[3] = value & 0x0F; break;

        // BG3 Y
        case 0x0400003C: ppu->bg3_affine.y.b[0] = value; break;
        case 0x0400003D: ppu->bg3_affine.y.b[1] = value; break;
        case 0x0400003E: ppu->bg3_affine.y.b[2] = value; break;
        case 0x0400003F: ppu->bg3_affine.y.b[3] = value & 0x0F; break;

        // Window0 Horizontal
        case 0x04000040: ppu->win_size[0].x2 = value; break;
        case 0x04000041: ppu->win_size[0].x1 = value; break;
        // Window1 Horizontal
        case 0x04000042: ppu->win_size[1].x2 = value; break;
        case 0x04000043: ppu->win_size[1].x1 = value; break;

        // Window0 Vertical
        case 0x04000044: ppu->win_size[0].y2 = value; break;
        case 0x04000045: ppu->win_size[0].y1 = value; break;
        // Window1 Vertical
        case 0x04000046: ppu->win_size[1].y2 = value; break;
        case 0x04000047: ppu->win_size[1].y1 = value; break;

        // Window0 Inside Control
        case 0x04000048: gba_ppu_write_win_control(&ppu->win0_control,value); break;
        // Window1 Inside Control
        case 0x04000049: gba_ppu_write_win_control(&ppu->win1_control,value); break;
        // Outside Window Control
        case 0x0400004A: gba_ppu_write_win_control(&ppu->out_win_control,value); break;
        // Object Window Control
        case 0x0400004B: gba_ppu_write_win_control(&ppu->obj_win_control,value); break;

        // Mosaic Size
        case 0x0400004C: gba_ppu_write_mosaic_size(&ppu->bg_mosaic_size,value); break;
        case 0x0400004D: gba_ppu_write_mosaic_size(&ppu->obj_mosaic_size,value); break;
        case 0x0400004E: break;
        case 0x0400004F: break;

        // Special Effect
        case 0x04000050: gba_ppu_write_special_effect_lsb(&ppu->special_effect,value); break;
        case 0x04000051: gba_ppu_write_special_effect_msb(&ppu->special_effect,value); break;
        // Alpha Blending
        case 0x04000052: ppu->ev.a = value & 0x1F; break;
        case 0x04000053: ppu->ev.b = value & 0x1F; break;
        
        // Brightness
        case 0x04000054: ppu->ev.y = value & 0x1F; break;
        case 0x04000055: break;
        case 0x04000056: break;
        case 0x04000057: break;
    }
}

uint8_t gba_ppu_read_register_raw(gba_ppu_t* ppu,uint32_t address){
    
    uint8_t value = ppu->gba->memory.bus.b[address & 0x03];

    switch(address){
        // LCD Control
        case 0x04000000: value = gba_ppu_read_lcd_control_lsb(ppu); break;
        case 0x04000001: value = gba_ppu_read_lcd_control_msb(ppu); break;
        // Green Swap
        case 0x04000002: value = ppu->green_swap ? 0x01 : 0x00; break;
        case 0x04000003: value = 0x00; break;
        // LCD Status
        case 0x04000004: value = gba_ppu_read_lcd_status_lsb(ppu); break;
        case 0x04000005: value = gba_ppu_read_lcd_status_msb(ppu); break;
        // VCounter
        case 0x04000006: value = ppu->scanline; break;
        case 0x04000007: value = 0x00; break;

        // BG0 Control
        case 0x04000008: value = gba_ppu_read_bg_control_lsb(ppu->bg_control + 0); break;
        case 0x04000009: value = gba_ppu_read_bg_control_msb(ppu->bg_control + 0); break;
        // BG1 Control
        case 0x0400000A: value = gba_ppu_read_bg_control_lsb(ppu->bg_control + 1); break;
        case 0x0400000B: value = gba_ppu_read_bg_control_msb(ppu->bg_control + 1); break;
        // BG2 Control
        case 0x0400000C: value = gba_ppu_read_bg_control_lsb(ppu->bg_control + 2); break;
        case 0x0400000D: value = gba_ppu_read_bg_control_msb(ppu->bg_control + 2); break;
        // BG3 Control
        case 0x0400000E: value = gba_ppu_read_bg_control_lsb(ppu->bg_control + 3); break;
        case 0x0400000F: value = gba_ppu_read_bg_control_msb(ppu->bg_control + 3); break;

        // Window0 Inside Control
        case 0x04000048: value = gba_ppu_read_win_control(&ppu->win0_control); break;
        // Window1 Inside Control
        case 0x04000049: value = gba_ppu_read_win_control(&ppu->win1_control); break;
        // Outside Window Control
        case 0x0400004A: value = gba_ppu_read_win_control(&ppu->out_win_control); break;
        // Object Window Control
        case 0x0400004B: value = gba_ppu_read_win_control(&ppu->obj_win_control); break;

        // Mosaic Size
        case 0x0400004C: value = 0x00; break;
        case 0x0400004D: value = 0x00; break;
        case 0x0400004E: value = 0x00; break;
        case 0x0400004F: value = 0x00; break;

        // Special Effect
        case 0x04000050: value = gba_ppu_read_special_effect_lsb(&ppu->special_effect); break;
        case 0x04000051: value = gba_ppu_read_special_effect_msb(&ppu->special_effect); break;
        // Alpha Blending
        case 0x04000052: value = ppu->ev.a & 0x1F; break;
        case 0x04000053: value = ppu->ev.b & 0x1F; break;
        
        // Brightness
        case 0x04000054: value = ppu->ev.y & 0x1F; break;
        case 0x04000055: value = 0x00; break;
        case 0x04000056: value = 0x00; break;
        case 0x04000057: value = 0x00; break;
    }

    return value;
}


void gba_ppu_write_cram(gba_ppu_t* ppu,gba_word_t value,uint32_t address,uint8_t size){
    address &= gba_cram_address_mask;

    // TODO: Plus 1 cycle if GBA accesses video memory at the same time
    
    switch(size){
        case gba_memory_byte_access:{
            address &= ~0x01;

            gba_clock(ppu->gba,1);

            ppu->cram[address | 0x00] = value.b[0];
            ppu->cram[address | 0x01] = value.b[0];
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(ppu->gba,1);

            ppu->cram[address | 0x00] = value.b[0];
            ppu->cram[address | 0x01] = value.b[1];
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(ppu->gba,1);

            ppu->cram[address | 0x00] = value.b[0];
            ppu->cram[address | 0x01] = value.b[1];

            gba_clock(ppu->gba,1);

            ppu->cram[address | 0x02] = value.b[2];
            ppu->cram[address | 0x03] = value.b[3];
            break;
        }
    }
}

gba_word_t gba_ppu_read_cram(gba_ppu_t* ppu,uint32_t address,uint8_t size){
    address &= gba_cram_address_mask;

    gba_word_t value = {0};

    // TODO: Plus 1 cycle if GBA accesses video memory at the same time

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(ppu->gba,1);

            value.b[0] = ppu->cram[address];
            value.b[1] = value.b[0];
            value.b[2] = value.b[0];
            value.b[3] = value.b[0];
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(ppu->gba,1);

            value.b[0] = ppu->cram[address | 0x00];
            value.b[1] = ppu->cram[address | 0x01];
            value.b[2] = value.b[0];
            value.b[3] = value.b[1];
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(ppu->gba,1);

            value.b[0] = ppu->cram[address | 0x00];
            value.b[1] = ppu->cram[address | 0x01];

            gba_clock(ppu->gba,1);

            value.b[2] = ppu->cram[address | 0x02];
            value.b[3] = ppu->cram[address | 0x03];
            break;
        }
    }

    return value;
}


void gba_ppu_write_vram(gba_ppu_t* ppu,gba_word_t value,uint32_t address,uint8_t size){
    address &= gba_vram_address_mask;
    
    if(address >= gba_vram_length){
        address = 0x10000 + (address & 0x7FFF);
    }

    // TODO: Plus 1 cycle if GBA accesses video memory at the same time

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(ppu->gba,1);

            // TODO
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(ppu->gba,1);

            ppu->vram[address | 0x00] = value.b[0];
            ppu->vram[address | 0x01] = value.b[1];
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(ppu->gba,1);

            ppu->vram[address | 0x00] = value.b[0];
            ppu->vram[address | 0x01] = value.b[1];

            gba_clock(ppu->gba,1);

            ppu->vram[address | 0x02] = value.b[2];
            ppu->vram[address | 0x03] = value.b[3];
            break;
        }
    }
}

gba_word_t gba_ppu_read_vram(gba_ppu_t* ppu,uint32_t address,uint8_t size){
    address &= gba_vram_address_mask;

    if(address >= gba_vram_length){
        address = 0x10000 + (address & 0x7FFF);
    }

    // TODO: Plus 1 cycle if GBA accesses video memory at the same time

    gba_word_t value = {0};

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(ppu->gba,1);

            value.b[0] = ppu->vram[address];
            value.b[1] = value.b[0];
            value.b[2] = value.b[0];
            value.b[3] = value.b[0];
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(ppu->gba,1);

            value.b[0] = ppu->vram[address | 0x00];
            value.b[1] = ppu->vram[address | 0x01];
            value.b[2] = value.b[0];
            value.b[3] = value.b[1];
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(ppu->gba,1);

            value.b[0] = ppu->vram[address | 0x00];
            value.b[1] = ppu->vram[address | 0x01];

            gba_clock(ppu->gba,1);

            value.b[2] = ppu->vram[address | 0x02];
            value.b[3] = ppu->vram[address | 0x03];
            break;
        }
    }
    
    return value;
}


void gba_ppu_write_oam(gba_ppu_t* ppu,gba_word_t value,uint32_t address,uint8_t size){
    address &= gba_oam_address_mask;

    // TODO: Plus 1 cycle if GBA accesses video memory at the same time

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(ppu->gba,1);
            // 8-bit writes to OAM are ignored
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(ppu->gba,1);

            ppu->oam[address | 0x00] = value.b[0];
            ppu->oam[address | 0x01] = value.b[1];
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(ppu->gba,1);

            ppu->oam[address | 0x00] = value.b[0];
            ppu->oam[address | 0x01] = value.b[1];
            ppu->oam[address | 0x02] = value.b[2];
            ppu->oam[address | 0x03] = value.b[3];
            break;
        }
    }
}

gba_word_t gba_ppu_read_oam(gba_ppu_t* ppu,uint32_t address,uint8_t size){
    address &= gba_oam_address_mask;

    // TODO: Plus 1 cycle if GBA accesses video memory at the same time

    gba_word_t value = {0};

    switch(size){
        case gba_memory_byte_access:{
            gba_clock(ppu->gba,1);

            value.b[0] = ppu->oam[address];
            value.b[1] = value.b[0];
            value.b[2] = value.b[0];
            value.b[3] = value.b[0];
            break;
        }
        case gba_memory_halfword_access:{
            address &= ~0x01;

            gba_clock(ppu->gba,1);

            value.b[0] = ppu->oam[address | 0x00];
            value.b[1] = ppu->oam[address | 0x01];
            value.b[2] = value.b[0];
            value.b[3] = value.b[1];
            break;
        }
        case gba_memory_word_access:{
            address &= ~0x03;

            gba_clock(ppu->gba,1);

            value.b[0] = ppu->oam[address | 0x00];
            value.b[1] = ppu->oam[address | 0x01];
            value.b[2] = ppu->oam[address | 0x02];
            value.b[3] = ppu->oam[address | 0x03];
            break;
        }
    }

    return value;
}


void gba_ppu_reset(gba_ppu_t* ppu){

    memset(&ppu->lcd_control,0x00,sizeof(ppu->lcd_control));

    ppu->green_swap = false;

    memset(&ppu->lcd_status,0x00,sizeof(ppu->lcd_status));

    ppu->scanline = gba_scanlines - 1;
    ppu->cycle = gba_scanline_cycles - 1;

    memset(&ppu->bg_control,0x00,sizeof(ppu->bg_control));

    memset(&ppu->bg_scroll,0x00,sizeof(ppu->bg_scroll));
    
    memset(&ppu->bg2_affine,0x00,sizeof(ppu->bg2_affine));
    memset(&ppu->bg3_affine,0x00,sizeof(ppu->bg3_affine));
    
    memset(&ppu->win_size,0x00,sizeof(ppu->win_size));

    memset(&ppu->win0_control,0x00,sizeof(ppu->win0_control));
    memset(&ppu->win1_control,0x00,sizeof(ppu->win1_control));
    memset(&ppu->obj_win_control,0x00,sizeof(ppu->obj_win_control));
    memset(&ppu->out_win_control,0x00,sizeof(ppu->out_win_control));

    memset(&ppu->bg_mosaic_size,0x00,sizeof(ppu->bg_mosaic_size));
    memset(&ppu->obj_mosaic_size,0x00,sizeof(ppu->obj_mosaic_size));

    memset(&ppu->special_effect,0x00,sizeof(ppu->special_effect));
    memset(&ppu->ev,0x00,sizeof(ppu->ev));

    memset(ppu->cram,0x00,sizeof(ppu->cram));
    memset(ppu->vram,0x00,sizeof(ppu->vram));
    memset(ppu->oam,0x00,sizeof(ppu->oam));

    ppu->frame_count = 0;

    gba_ppu_schedule_next_event(ppu);
}