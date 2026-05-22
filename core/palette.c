#include "./palette.h"
#include "./gb.h"

static const gb_rgb_t dmg_palette[4] = {
    {0x08,0x18,0x20},
    {0x34,0x68,0x56},
    {0x88,0xC0,0x70},
    {0xE0,0xF8,0xD0}
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
        case 0xFF68: break;
        case 0xFF69: break;
        case 0xFF6A: break;
        case 0xFF6B: break;
    }
}

uint8_t gb_palette_read_cgb_register(void* data,uint16_t address){
    gb_palette_t* palette = (gb_palette_t*)data;

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF68: break;
        case 0xFF69: break;
        case 0xFF6A: break;
        case 0xFF6B: break;
    }

    return value;
}

void gb_palette_map_dmg_registers(gb_palette_t* palette){
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
}