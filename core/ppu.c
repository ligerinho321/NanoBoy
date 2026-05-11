#include "./ppu.h"
#include "./gb.h"

void gb_ppu_init(gb_ppu_t* ppu,gb_t* gb){
    ppu->gb = gb;

    ppu->register_handler = (gb_memory_handler_t){
        gb_ppu_write_register,
        gb_ppu_read_register,
        ppu
    };
}

void gb_ppu_clock(gb_ppu_t* ppu){

}

void gb_ppu_write_register(void* data,uint8_t value,uint16_t address){
    gb_ppu_t* ppu = (gb_ppu_t*)data;

    switch(address){
        //LCDC
        case 0xFF40:{
            ppu->lcdc.bg_and_window_enabled = value & 0x01;
            ppu->lcdc.obj_enabled = value & 0x02;
            ppu->lcdc.obj_size = value & 0x04;
            ppu->lcdc.bg_tilemap_area = value & 0x08;
            ppu->lcdc.bg_and_window_tiledata_area = value & 0x10;
            ppu->lcdc.window_enabled = value & 0x20;
            ppu->lcdc.window_tilemap_area = value & 0x40;
            ppu->lcdc.lcd_enabled = value & 0x80;
            break;
        }
        //STAT
        case 0xFF41:{
            ppu->status.mode0_select = value & 0x08;
            ppu->status.mode1_select = value & 0x10;
            ppu->status.mode2_select = value & 0x20;
            ppu->status.lyc_select = value & 0x40;
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
                (ppu->lcdc.bg_and_window_enabled ? 0x01 : 0x00) |
                (ppu->lcdc.obj_enabled ? 0x02 : 0x00) |
                (ppu->lcdc.obj_size ? 0x04 : 0x00) |
                (ppu->lcdc.bg_tilemap_area ? 0x08 : 0x00) |
                (ppu->lcdc.bg_and_window_tiledata_area ? 0x10 : 0x00) |
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
                (ppu->status.mode0_select ? 0x08 : 0x00) |
                (ppu->status.mode1_select ? 0x10 : 0x00) |
                (ppu->status.mode2_select ? 0x20 : 0x00) |
                (ppu->status.lyc_select ? 0x40 : 0x00) |
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

void gb_ppu_map(gb_ppu_t* ppu){
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

void gb_ppu_reset(gb_ppu_t* ppu){
    
}