#include "palette.h"
#include "gb.h"

const gb_rgb_t dmg_palette[4] = {
    {0xE0,0xF8,0xD0},
    {0x88,0xC0,0x70},
    {0x34,0x68,0x56},
    {0x08,0x18,0x20}
};


void gb_palette_init(gb_palette_t* palette,gb_t* gb){
    palette->gb = gb;

    gb_palette_map_dmg_registers(palette);
    
    palette->cgb_register_handler = (gb_memory_handler_t){
        gb_palette_write_cgb_register,
        gb_palette_read_cgb_register,
        palette
    };
}


gb_rgb_t gb_palette_rgb555_to_rgb888(uint16_t color){
    uint8_t r5 = color & 0x1F;
    uint8_t g5 = (color >> 0x05) & 0x1F;
    uint8_t b5 = (color >> 0x0A) & 0x1F;
    return (gb_rgb_t){
        (uint8_t)((r5 << 0x03) | (r5 >> 0x02)),
        (uint8_t)((g5 << 0x03) | (g5 >> 0x02)),
        (uint8_t)((b5 << 0x03) | (b5 >> 0x02))
    };
}


void gb_palette_write_dmg_register(void* data,uint8_t value,uint16_t address){
    gb_palette_t* palette = (gb_palette_t*)data;

    switch(address){
        //BGP
        case 0xFF47: palette->bgp = value; break;
        //OBP0
        case 0xFF48: palette->obp[0] = value; break;
        //OBP1
        case 0xFF49: palette->obp[1] = value; break;
    }
}

uint8_t gb_palette_read_dmg_register(void* data,uint16_t address){
    gb_palette_t* palette = (gb_palette_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        //BGP
        case 0xFF47: value = palette->bgp; break;
        //OBP0
        case 0xFF48: value = palette->obp[0]; break;
        //OBP1
        case 0xFF49: value = palette->obp[1]; break;
    }

    return value;
}


void gb_palette_write_cgb_register(void* data,uint8_t value,uint16_t address){
    gb_palette_t* palette = (gb_palette_t*)data;

    switch(address){
        //BCPS
        case 0xFF68:{
            palette->bcps = value;
            break;
        }
        //BCPD
        case 0xFF69:{
            uint8_t address = palette->bcps & 0x3F;
            
            palette->cgb_bg_cram[address] = value;

            uint16_t color = (palette->cgb_bg_cram[address | 0x01] << 0x08) | palette->cgb_bg_cram[address & 0x3E];

            palette->cgb_bg_cram_converted[address >> 0x01] = gb_palette_rgb555_to_rgb888(color);

            if(palette->bcps & 0x80){
                palette->bcps = (palette->bcps & 0xC0) | ((address + 0x01) & 0x3F);
            }
            break;
        }
        //OCPS
        case 0xFF6A:{
            palette->ocps = value;
            break;
        }
        //OCPD
        case 0xFF6B:{
            uint8_t address = palette->ocps & 0x3F;
            
            palette->cgb_obj_cram[address] = value;

            uint16_t color = (palette->cgb_obj_cram[address | 0x01] << 0x08) | palette->cgb_obj_cram[address & 0x3E];

            palette->cgb_obj_cram_converted[address >> 0x01] = gb_palette_rgb555_to_rgb888(color);

            if(palette->ocps & 0x80){
                palette->ocps = (palette->ocps & 0xC0) | ((address + 0x01) & 0x3F);
            }
            break;
        }
    }
}

uint8_t gb_palette_read_cgb_register(void* data,uint16_t address){
    gb_palette_t* palette = (gb_palette_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        //BCPS
        case 0xFF68: value = palette->bcps; break;
        //BCPD
        case 0xFF69: value = palette->cgb_bg_cram[palette->bcps & 0x3F]; break;
        //OCPS
        case 0xFF6A: value = palette->ocps; break;
        //OCPD
        case 0xFF6B: value = palette->cgb_obj_cram[palette->ocps & 0x3F]; break;
    }

    return value;
}


void gb_palette_map_dmg_registers(gb_palette_t* palette){

    palette->dmg_register_handler = (gb_memory_handler_t){
        gb_palette_write_dmg_register,
        gb_palette_read_dmg_register,
        palette
    };

    gb_memory_handler_t** bus = palette->gb->memory.bus;

    bus[0xFF47] = &palette->dmg_register_handler;
    bus[0xFF48] = &palette->dmg_register_handler;
    bus[0xFF49] = &palette->dmg_register_handler;
}


void gb_palette_map_cgb_registers(gb_palette_t* palette){
    gb_memory_handler_t** bus = palette->gb->memory.bus;
    bus[0xFF68] = &palette->cgb_register_handler;
    bus[0xFF69] = &palette->cgb_register_handler;
    bus[0xFF6A] = &palette->cgb_register_handler;
    bus[0xFF6B] = &palette->cgb_register_handler;
}

void gb_palette_unmap_cgb_registers(gb_palette_t* palette){
    gb_memory_handler_t** bus = palette->gb->memory.bus;
    bus[0xFF68] = NULL;
    bus[0xFF69] = NULL;
    bus[0xFF6A] = NULL;
    bus[0xFF6B] = NULL; 
}


void gb_palette_reset(gb_palette_t* palette){
    palette->bgp = 0x00;
    palette->obp[0] = 0x00;
    palette->obp[1] = 0x00;

    if(palette->gb->type == gb_cgb){
        palette->bcps = 0x00;
        palette->ocps = 0x00;

        memset(palette->cgb_bg_cram,0x00,sizeof(palette->cgb_bg_cram));
        memset(palette->cgb_obj_cram,0x00,sizeof(palette->cgb_obj_cram));

        memset(palette->cgb_bg_cram_converted,0x00,sizeof(palette->cgb_bg_cram_converted));
        memset(palette->cgb_obj_cram_converted,0x00,sizeof(palette->cgb_obj_cram_converted));
    }
}