#include "palette.h"
#include "gb.h"

const gb_rgb_t dmg_colors[gb_dmg_colors] = {
    {0xE0,0xF8,0xD0},
    {0x88,0xC0,0x70},
    {0x34,0x68,0x56},
    {0x08,0x18,0x20}
};


void gb_palette_init(gb_palette_t* palette,gb_t* gb){
    palette->gb = gb;

    palette->dmg_register_handler = (gb_memory_handler_t){
        gb_palette_write_dmg_register,
        gb_palette_read_dmg_register,
        palette
    };
    
    palette->cgb_register_handler = (gb_memory_handler_t){
        gb_palette_write_cgb_register,
        gb_palette_read_cgb_register,
        palette
    };
}


void gb_palette_cgb_dmg_colorization(gb_palette_t* palette){

    static const uint8_t title_checksums[79] = {
        0x00, 0x88, 0x16, 0x36, 0xD1, 0xDB, 0xF2, 0x3C, 0x8C, 0x92, 0x3D, 0x5C, 0x58, 0xC9, 0x3E, 0x70,
        0x1D, 0x59, 0x69, 0x19, 0x35, 0xA8, 0x14, 0xAA, 0x75, 0x95, 0x99, 0x34, 0x6F, 0x15, 0xFF, 0x97,
        0x4B, 0x90, 0x17, 0x10, 0x39, 0xF7, 0xF6, 0xA2, 0x49, 0x4E, 0x43, 0x68, 0xE0, 0x8B, 0xF0, 0xCE,
        0x0C, 0x29, 0xE8, 0xB7, 0x86, 0x9A, 0x52, 0x01, 0x9D, 0x71, 0x9C, 0xBD, 0x5D, 0x6D, 0x67, 0x3F,
        0x6B,
        //These checksums are also discriminated based on the 4th title letter
        0xB3, 0x46, 0x28, 0xA5, 0xC6, 0xD3, 0x27, 0x61, 0x18, 0x66, 0x6A, 0xBF, 0x0D, 0xF4
    };

    static const char title_fourth_letters[30] = "BEFAARBEKEK R-" "URAR INAILICE " "R";

    static const uint8_t palette_indexes_and_flags[94] = {
        0x7C, 0x08, 0x12, 0xA3, 0xA2, 0x07, 0x87, 0x4B,
        0x20, 0x12, 0x65, 0xA8, 0x16, 0xA9, 0x86, 0xB1,
        0x68, 0xA0, 0x87, 0x66, 0x12, 0xA1, 0x30, 0x3C,
        0x12, 0x85, 0x12, 0x64, 0x1B, 0x07, 0x06, 0x6F,
        0x6E, 0x6E, 0xAE, 0xAF, 0x6F, 0xB2, 0xAF, 0xB2,
        0xA8, 0xAB, 0x6F, 0xAF, 0x86, 0xAE, 0xA2, 0xA2,
        0x12, 0xAF, 0x13, 0x12, 0xA1, 0x6E, 0xAF, 0xAF,
        0xAD, 0x06, 0x4C, 0x6E, 0xAF, 0xAF, 0x12, 0x7C,
        0xAC, 0xA8, 0x6A, 0x6E, 0x13, 0xA0, 0x2D, 0xA8,
        0x2B, 0xAC, 0x64, 0xAC, 0x6D, 0x87, 0xBC, 0x60,
        0xB4, 0x13, 0x72, 0x7C, 0xB5, 0xAE, 0xAE, 0x7C,
        0x7C, 0x65, 0xA2, 0x6C, 0x64, 0x85
    };

    static const uint8_t palette_indexes[87] = {
        0x80, 0xB0, 0x40, 
        0x88, 0x20, 0x68, 
        0xDE, 0x00, 0x70, 
        0xDE, 0x20, 0x78, 
        0x20, 0x20, 0x38, 
        0x20, 0xB0, 0x90, 
        0x20, 0xB0, 0xA0, 
        0xE0, 0xB0, 0xC0, 
        0x98, 0xB6, 0x48, 
        0x80, 0xE0, 0x50, 
        0x1E, 0x1E, 0x58, 
        0x20, 0xB8, 0xE0, 
        0x88, 0xB0, 0x10, 
        0x20, 0x00, 0x10, 
        0x20, 0xE0, 0x18, 
        0xE0, 0x18, 0x00, 
        0x18, 0xE0, 0x20, 
        0xA8, 0xE0, 0x20, 
        0x18, 0xE0, 0x00, 
        0x20, 0x18, 0xD8, 
        0xC8, 0x18, 0xE0, 
        0x00, 0xE0, 0x40, 
        0x28, 0x28, 0x28, 
        0x18, 0xE0, 0x60, 
        0x20, 0x18, 0xE0, 
        0x00, 0x00, 0x08, 
        0xE0, 0x18, 0x30, 
        0xD0, 0xD0, 0xD0, 
        0x20, 0xE0, 0xE8
    };

    static const uint8_t palettes[240] = {
        0xFF,0x7F, 0xBF,0x32, 0xD0,0x00, 0x00,0x00, 
        0x9F,0x63, 0x79,0x42, 0xB0,0x15, 0xCB,0x04, 
        0xFF,0x7F, 0x31,0x6E, 0x4A,0x45, 0x00,0x00, 
        0xFF,0x7F, 0xEF,0x1B, 0x00,0x02, 0x00,0x00, 
        0xFF,0x7F, 0x1F,0x42, 0xF2,0x1C, 0x00,0x00, 
        0xFF,0x7F, 0x94,0x52, 0x4A,0x29, 0x00,0x00, 
        0xFF,0x7F, 0xFF,0x03, 0x2F,0x01, 0x00,0x00, 
        0xFF,0x7F, 0xEF,0x03, 0xD6,0x01, 0x00,0x00, 
        0xFF,0x7F, 0xB5,0x42, 0xC8,0x3D, 0x00,0x00, 
        0x74,0x7E, 0xFF,0x03, 0x80,0x01, 0x00,0x00, 
        0xFF,0x67, 0xAC,0x77, 0x13,0x1A, 0x6B,0x2D, 
        0xD6,0x7E, 0xFF,0x4B, 0x75,0x21, 0x00,0x00, 
        0xFF,0x53, 0x5F,0x4A, 0x52,0x7E, 0x00,0x00, 
        0xFF,0x4F, 0xD2,0x7E, 0x4C,0x3A, 0xE0,0x1C, 
        0xED,0x03, 0xFF,0x7F, 0x5F,0x25, 0x00,0x00, 
        0x6A,0x03, 0x1F,0x02, 0xFF,0x03, 0xFF,0x7F, 
        0xFF,0x7F, 0xDF,0x01, 0x12,0x01, 0x00,0x00, 
        0x1F,0x23, 0x5F,0x03, 0xF2,0x00, 0x09,0x00, 
        0xFF,0x7F, 0xEA,0x03, 0x1F,0x01, 0x00,0x00, 
        0x9F,0x29, 0x1A,0x00, 0x0C,0x00, 0x00,0x00, 
        0xFF,0x7F, 0x7F,0x02, 0x1F,0x00, 0x00,0x00, 
        0xFF,0x7F, 0xE0,0x03, 0x06,0x02, 0x20,0x01, 
        0xFF,0x7F, 0xEB,0x7E, 0x1F,0x00, 0x00,0x7C, 
        0xFF,0x7F, 0xFF,0x3F, 0x00,0x7E, 0x1F,0x00, 
        0xFF,0x7F, 0xFF,0x03, 0x1F,0x00, 0x00,0x00, 
        0xFF,0x03, 0x1F,0x00, 0x0C,0x00, 0x00,0x00, 
        0xFF,0x7F, 0x3F,0x03, 0x93,0x01, 0x00,0x00, 
        0x00,0x00, 0x00,0x42, 0x7F,0x03, 0xFF,0x7F, 
        0xFF,0x7F, 0x8C,0x7E, 0x00,0x7C, 0x00,0x00, 
        0xFF,0x7F, 0xEF,0x1B, 0x80,0x61, 0x00,0x00
    };

    gb_cartridge_t* cartridge = &palette->gb->cartridge;

    uint8_t palette_id = 0x00;

    uint8_t old_licensee_code = gb_cartridge_old_licensee_code(cartridge);
    const char* new_licensee_code = gb_cartridge_new_licensee_code(cartridge);

    if(old_licensee_code == 0x01 || (old_licensee_code == 0x33 && !memcmp(new_licensee_code,"01",0x02))){
        
        const char* title = gb_cartridge_title(cartridge);

        uint8_t title_checksum = 0x00;
        
        for(int i = 0; i < 16; ++i){
            title_checksum += title[i];
        }

        int index = -1;

        for(int i = 0; i < 79; ++i){
            if(title_checksums[i] == title_checksum){
                index = i;
                break;
            }
        }

        if(index >= 0){
            if(index < 65){
                palette_id = index;
            }
            else{
                uint8_t col = index - 65;

                for(int row = 0; row < 3; ++row){
                    
                    uint8_t pos = row * 14 + col;

                    if(pos >= 30) break;

                    if(title_fourth_letters[pos] == title[0x03]){
                        palette_id = index + row * 14;
                        break;
                    }
                }
            }
        }
    }


    uint8_t flags = palette_indexes_and_flags[palette_id] >> 0x05;

    const uint8_t* palette_index = palette_indexes + (palette_indexes_and_flags[palette_id] & 0x1F) * 3;


    const uint8_t* bgp_src = palettes + palette_index[2];

    
    const uint8_t* obp0_src = NULL;

    if(flags & 0x01){
        obp0_src = palettes + palette_index[0];
    }
    else{
        obp0_src = palettes + palette_index[2];
    }


    const uint8_t* obp1_src = NULL;

    if(flags & 0x04){
        obp1_src = palettes + palette_index[1];
    }
    else if(flags & 0x02){
        obp1_src = palettes + palette_index[0];
    }
    else{
        obp1_src = palettes + palette_index[2];
    }


    uint8_t* bgp_dst = palette->bg_cram;
    uint8_t* obp0_dst = palette->obj_cram + 0;
    uint8_t* obp1_dst = palette->obj_cram + 8;

    for(int i = 0; i < 4; ++i){

        bgp_dst[0] = bgp_src[0];
        bgp_dst[1] = bgp_src[1];
        
        palette->bg_cram_converted[i] = gb_palette_rgb555_to_rgb888((bgp_src[1] << 0x08) | bgp_src[0]);
        
        bgp_dst += 2;
        bgp_src += 2;

        obp0_dst[0] = obp0_src[0];
        obp0_dst[1] = obp0_src[1];
        
        palette->obj_cram_converted[0 + i] = gb_palette_rgb555_to_rgb888((obp0_src[1] << 0x08) | obp0_src[0]);
        
        obp0_dst += 2;
        obp0_src += 2;

        obp1_dst[0] = obp1_src[0];
        obp1_dst[1] = obp1_src[1];
        
        palette->obj_cram_converted[4 + i] = gb_palette_rgb555_to_rgb888((obp1_src[1] << 0x08) | obp1_src[0]);
        
        obp1_dst += 2;
        obp1_src += 2;
    }
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
    gb_t* gb = palette->gb;

    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return;

    switch(address){
        //BCPS
        case 0xFF68:{
            palette->bcps = value & 0xBF;
            break;
        }
        //BCPD
        case 0xFF69:{
            uint8_t address = palette->bcps & 0x3F;
            
            palette->bg_cram[address] = value;

            uint16_t color = (palette->bg_cram[(address & 0x3E) | 0x01] << 0x08) | palette->bg_cram[address & 0x3E];

            palette->bg_cram_converted[address >> 0x01] = gb_palette_rgb555_to_rgb888(color);

            if(palette->bcps & 0x80){
                palette->bcps = (palette->bcps & 0x80) | ((address + 0x01) & 0x3F);
            }
            break;
        }
        //OCPS
        case 0xFF6A:{
            palette->ocps = value & 0xBF;
            break;
        }
        //OCPD
        case 0xFF6B:{
            uint8_t address = palette->ocps & 0x3F;
            
            palette->obj_cram[address] = value;

            uint16_t color = (palette->obj_cram[(address & 0x3E) | 0x01] << 0x08) | palette->obj_cram[address & 0x3E];

            palette->obj_cram_converted[address >> 0x01] = gb_palette_rgb555_to_rgb888(color);

            if(palette->ocps & 0x80){
                palette->ocps = (palette->ocps & 0x80) | ((address + 0x01) & 0x3F);
            }
            break;
        }
    }
}

uint8_t gb_palette_read_cgb_register(void* data,uint16_t address){
    gb_palette_t* palette = (gb_palette_t*)data;
    gb_t* gb = palette->gb;
    
    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return 0xFF;

    uint8_t value = 0xFF;

    switch(address){
        //BCPS
        case 0xFF68: value = palette->bcps | 0x40; break;
        //BCPD
        case 0xFF69: value = palette->bg_cram[palette->bcps & 0x3F]; break;
        //OCPS
        case 0xFF6A: value = palette->ocps | 0x40; break;
        //OCPD
        case 0xFF6B: value = palette->obj_cram[palette->ocps & 0x3F]; break;
    }

    return value;
}


void gb_palette_map(gb_palette_t* palette){
    gb_memory_t* memory = &palette->gb->memory;

    gb_memory_map_in_range(memory,&palette->dmg_register_handler,0xFF47,0xFF49);

    gb_memory_map_in_range(memory,&palette->cgb_register_handler,0xFF68,0xFF6B);
}


void gb_palette_reset(gb_palette_t* palette){

    palette->bgp = 0x00;
    palette->obp[0] = 0x00;
    palette->obp[1] = 0x00;

    palette->bcps = 0x00;
    palette->ocps = 0x00;

    memset(palette->bg_cram,0x00,sizeof(palette->bg_cram));
    memset(palette->obj_cram,0x00,sizeof(palette->obj_cram));

    memset(palette->bg_cram_converted,0x00,sizeof(palette->bg_cram_converted));
    memset(palette->obj_cram_converted,0x00,sizeof(palette->obj_cram_converted));
}

void gb_palette_skip_boot(gb_palette_t* palette){
    if(palette->gb->is_cgb){
        
        if(palette->gb->cgb_mode){

            palette->bcps = 0x80;
            palette->ocps = 0x81;
            
            for(int i = 0; i < gb_cgb_cram_length; ++i){
                gb_palette_write_cgb_register(palette,(i & 0x01) ? 0x7F : 0xFF,0xFF69);
            }
        }
        else{
            palette->bcps = 0x88;
            palette->ocps = 0x90;

            gb_palette_cgb_dmg_colorization(palette);
        }
    }

    palette->bgp = 0xFC;
}


void gb_palette_save_state(gb_palette_t* palette,gb_state_t* state){
    gb_state_write(state,palette->bgp);
    gb_state_write(state,palette->obp);
    
    gb_state_write(state,palette->bcps);
    gb_state_write(state,palette->ocps);

    gb_state_write(state,palette->bg_cram);
    gb_state_write(state,palette->obj_cram);

    gb_state_write(state,palette->bg_cram_converted);
    gb_state_write(state,palette->obj_cram_converted);
}

void gb_palette_load_state(gb_palette_t* palette,gb_state_t* state){
    gb_state_read(state,palette->bgp);
    gb_state_read(state,palette->obp);
    
    gb_state_read(state,palette->bcps);
    gb_state_read(state,palette->ocps);

    gb_state_read(state,palette->bg_cram);
    gb_state_read(state,palette->obj_cram);

    gb_state_read(state,palette->bg_cram_converted);
    gb_state_read(state,palette->obj_cram_converted);
}