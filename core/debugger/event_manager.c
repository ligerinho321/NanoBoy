#include "event_manager.h"
#include "../gb.h"

const char* gb_event_type_names[gb_event_type_count] = {
    "",
    "Halt",
    "Stop",
    "IRQ Vblank",
    "IRQ LCD",
    "IRQ Timer",
    "IRQ Serial",
    "IRQ Joypad",
    "VRAM",
    "WRAM",
    "OAM",
    "HRAM",
    "Joypad",
    "Serial",
    "Timer",
    "Interrupt",
    "Square1 Channel",
    "Square2 Channel",
    "Wave Channel",
    "Noise Channel",
    "APU Control",
    "Wave RAM",
    "PPU",
    "Palette",
    "DMA",
    "Others"
};

const gb_rgb_t gb_event_screen_palette[gb_event_screen_color_count] = {
    {50,50,50},
    {100,100,100},
    {210,170,80},
    {150,150,150},
    {80,160,190},
    {210,130,80},
    {57,169,73}
};

const char* gb_event_screen_color_names[gb_event_screen_color_count] = {
    "Hblank",
    "Vblank",
    "OAM Scan",
    "Fictitious Fetch",
    "Background Fetch",
    "Window Fetch",
    "Object Fetch",
};

static inline void gb_event_frame_clear(gb_event_frame_t* frame){
    memset(frame->range,0,sizeof(frame->range));
    frame->count = 0;
}

static inline void gb_event_frame_free(gb_event_frame_t* frame){
    if(frame->event != NULL){
        free(frame->event);
        frame->event = NULL;
    }
    frame->count = 0;
    frame->capacity = 0;
}


void gb_event_manager_init(gb_event_manager_t* event_manager,gb_t* gb){
    event_manager->gb = gb;

    gb_event_manager_map(event_manager);
}


void gb_event_manager_map(gb_event_manager_t* event_manager){

    gb_event_register_map_t* map = event_manager->registers_map;

    //Joypad
    gb_event_register_map_t joypad = {
        .type = gb_event_joypad_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    map[0x00] = joypad;


    //Serial
    gb_event_register_map_t serial = {
        .type = gb_event_serial_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    map[0x01] = serial;
    map[0x02] = serial;


    //Timer
    gb_event_register_map_t timer = {
        .type = gb_event_timer_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    for(int i = 0x04; i <= 0x07; ++i){
        map[i] = timer;
    }


    //Interrupt
    gb_event_register_map_t interrupt = {
        .type = gb_event_interrupt_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };
    
    map[0x0F] = interrupt;
    map[0xFF] = interrupt;


    //Square1
    gb_event_register_map_t square1 = {
        .type = gb_event_square1_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    gb_event_register_map_t square1_write = {
        .type = gb_event_square1_type,
        .flags = gb_event_write_flag
    };

    map[0x10] = square1;
    map[0x11] = square1;
    map[0x12] = square1;
    map[0x13] = square1_write;
    map[0x14] = square1;


    //Square2
    gb_event_register_map_t square2 = {
        .type = gb_event_square2_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    gb_event_register_map_t square2_write = {
        .type = gb_event_square2_type,
        .flags = gb_event_write_flag
    };

    map[0x16] = square2;
    map[0x17] = square2;
    map[0x18] = square2_write;
    map[0x19] = square2;


    //Wave
    gb_event_register_map_t wave = {
        .type = gb_event_wave_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    gb_event_register_map_t wave_write = {
        .type = gb_event_wave_type,
        .flags = gb_event_write_flag
    };

    map[0x1A] = wave;
    map[0x1B] = wave_write;
    map[0x1C] = wave;
    map[0x1D] = wave_write;
    map[0x1E] = wave;


    //Noise
    gb_event_register_map_t noise = {
        .type = gb_event_noise_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    gb_event_register_map_t noise_write = {
        .type = gb_event_noise_type,
        .flags = gb_event_write_flag
    };

    map[0x20] = noise_write;
    map[0x21] = noise;
    map[0x22] = noise;
    map[0x23] = noise;


    //General
    gb_event_register_map_t apu_control = {
        .type = gb_event_apu_control_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    map[0x24] = apu_control;
    map[0x25] = apu_control;
    map[0x26] = apu_control;


    //Wave RAM
    gb_event_register_map_t wave_ram = {
        .type = gb_event_wave_ram_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    for(int i = 0x30; i <= 0x3F; ++i){
        map[i] = wave_ram;
    }


    //PPU
    gb_event_register_map_t ppu = {
        .type = gb_event_ppu_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    gb_event_register_map_t ppu_read = {
        .type = gb_event_ppu_type,
        .flags = gb_event_read_flag
    };

    map[0x40] = ppu;
    map[0x41] = ppu;
    map[0x42] = ppu;
    map[0x43] = ppu;
    map[0x44] = ppu_read;
    map[0x45] = ppu;
    map[0x4A] = ppu;
    map[0x4B] = ppu;


    //Palette DMG
    gb_event_register_map_t palette = {
        .type = gb_event_palette_type,
        .flags = gb_event_write_flag | gb_event_read_flag
    };

    map[0x47] = palette;
    map[0x48] = palette;
    map[0x49] = palette;

    //OAM DMA
    gb_event_register_map_t dma = {
        .type = gb_event_dma_type,
        .flags = gb_event_write_flag | gb_event_read_flag,
    };

    map[0x46] = dma;


    //HRAM
    gb_event_register_map_t hram = {
        .type = gb_event_hram_type,
        .flags = gb_event_write_flag | gb_event_read_flag,
    };

    for(int i = 0x80; i <= 0xFE; ++i){
        map[i] = hram;
    }
}

void gb_event_manager_update_mapping(gb_event_manager_t* event_manager){
    gb_t* gb = event_manager->gb;

    gb_event_register_map_t* map = event_manager->registers_map;

    if(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped)){
        //VRAM DMA
        gb_event_register_map_t dma_write = {
            .type = gb_event_dma_type,
            .flags = gb_event_write_flag,
        };

        gb_event_register_map_t dma = {
            .type = gb_event_dma_type,
            .flags = gb_event_write_flag | gb_event_read_flag,
        };

        map[0x51] = dma_write; //HDMA1
        map[0x52] = dma_write; //HDMA2
        map[0x53] = dma_write; //HDMA3
        map[0x54] = dma_write; //HDMA4
        map[0x55] = dma; //HDMA5


        //Palette
        gb_event_register_map_t palette = {
            .type = gb_event_palette_type,
            .flags = gb_event_write_flag | gb_event_read_flag,
        };

        map[0x68] = palette; //BGPI
        map[0x69] = palette; //BGPD
        map[0x6A] = palette; //OBPI
        map[0x6B] = palette; //OBPD


        //Others
        gb_event_register_map_t others = {
            .type = gb_event_others_type,
            .flags = gb_event_write_flag | gb_event_read_flag,
        };
        gb_event_register_map_t others_read = {
            .type = gb_event_others_type,
            .flags = gb_event_read_flag,
        };

        map[0x4D] = others; //KEY1
        map[0x4F] = others; //VBK
        map[0x56] = others; //RP
        map[0x6C] = others; //OPRI
        map[0x70] = others; //WBK
        map[0x72] = others; //Undocumented
        map[0x73] = others; //Undocumented
        map[0x74] = others; //Undocumented
        map[0x75] = others; //Undocumented
        map[0x76] = others_read; //PCM12
        map[0x77] = others_read; //PCM34
    }
    else{
        map[0x51].type = gb_event_none_type;
        map[0x52].type = gb_event_none_type;
        map[0x53].type = gb_event_none_type;
        map[0x54].type = gb_event_none_type;
        map[0x55].type = gb_event_none_type;

        map[0x68].type = gb_event_none_type;
        map[0x69].type = gb_event_none_type;
        map[0x6A].type = gb_event_none_type;
        map[0x6B].type = gb_event_none_type;

        map[0x4D].type = gb_event_none_type;
        map[0x4F].type = gb_event_none_type;
        map[0x56].type = gb_event_none_type;
        map[0x6C].type = gb_event_none_type;
        map[0x70].type = gb_event_none_type;
        map[0x72].type = gb_event_none_type;
        map[0x73].type = gb_event_none_type;
        map[0x74].type = gb_event_none_type;
        map[0x75].type = gb_event_none_type;
        map[0x76].type = gb_event_none_type;
        map[0x77].type = gb_event_none_type;
    }

    if(gb->is_cgb && gb->boot.mapped){
        gb_event_register_map_t others = {
            .type = gb_event_others_type,
            .flags = gb_event_write_flag | gb_event_read_flag,
        };

        map[0x4C] = others; //KEY0
    }
    else{
        map[0x4C].type = gb_event_none_type;
    }

    if(gb->boot.mapped){
        gb_event_register_map_t others = {
            .type = gb_event_others_type,
            .flags = gb_event_write_flag | gb_event_read_flag,
        };

        map[0x50] = others; //BANK
    }
    else{
        map[0x50].type = gb_event_none_type;
    }
}


void gb_event_manager_screen_blank(gb_t* gb){

    gb_event_manager_t* event_manager = &gb->event_manager;

    if(!event_manager->enabled) return;

    int len = gb_frame_cycles;
    
    uint8_t* pixel = event_manager->screen;
    
    gb_rgb_t color = gb_event_screen_palette[gb_event_screen_hblank_color];
    
    while(len--){
        *pixel++ = color.r;
        *pixel++ = color.g;
        *pixel++ = color.b;
    }    
}


static void gb_event_manager_push_event(gb_event_manager_t* event_manager,gb_event_t* event){

    gb_event_frame_t* frame = event_manager->current_frame;

    gb_ppu_t* ppu = &event_manager->gb->ppu;

    gb_event_range_t* range = frame->range + ppu->scanline * gb_scanline_cycles + ppu->cycle;

    if(range->count == 0x00){
        range->start = frame->count;
    }

    if(frame->count >= frame->capacity){

        uint32_t new_capacity = frame->capacity ? frame->capacity * 2 : gb_event_frame_inital_capacity;

        void* ptr = realloc(frame->event,sizeof(gb_event_t) * new_capacity);
        
        if(!ptr){
            gb_printf_errno(realloc);
            return;
        }

        frame->event = (gb_event_t*)ptr;
        frame->capacity = new_capacity;
    }
    
    frame->event[frame->count] = *event;

    ++frame->count;
    ++range->count;
}


void gb_event_manager_halt(gb_t* gb){

    gb_event_manager_t* event_manager = &gb->event_manager;

    gb_event_t event = {
        .scanline = gb->ppu.scanline,
        .cycle = gb->ppu.cycle,
        .pc = gb->cpu.instruction_pc,
        .type = gb_event_halt_type,
        .flag = gb_event_interrupt_flag,
        .address = 0,
        .value = 0
    };
    gb_event_manager_push_event(event_manager,&event);
}

void gb_event_manager_stop(gb_t* gb){

    gb_event_manager_t* event_manager = &gb->event_manager;

    gb_event_t event = {
        .scanline = gb->ppu.scanline,
        .cycle = gb->ppu.cycle,
        .pc = gb->cpu.instruction_pc,
        .type = gb_event_stop_type,
        .flag = gb_event_interrupt_flag,
        .address = 0,
        .value = 0
    };
    gb_event_manager_push_event(event_manager,&event);
}

void gb_event_manager_irq(gb_t* gb,uint8_t vector){

    gb_event_manager_t* event_manager = &gb->event_manager;

    uint8_t type = gb_event_none_type;

    switch(vector){
        case gb_interrupt_vblank_vector:
            type = gb_event_irq_vblank_type;
            break;
        case gb_interrupt_lcd_vector:
            type = gb_event_irq_lcd_type;
            break;
        case gb_interrupt_timer_vector:
            type = gb_event_irq_timer_type;
            break;
        case gb_interrupt_serial_vector:
            type = gb_event_irq_serial_type;
            break;
        case gb_interrupt_joypad_vector:
            type = gb_event_irq_joypad_type;
            break;       
    }

    if(type != gb_event_none_type){
        gb_event_t event = {
            .scanline = gb->ppu.scanline,
            .cycle = gb->ppu.cycle,
            .pc = vector,
            .type = type,
            .flag = gb_event_interrupt_flag,
            .address = 0,
            .value = 0
        };
        gb_event_manager_push_event(event_manager,&event);
    }
}

void gb_event_manager_io(gb_t* gb,uint8_t flag,uint8_t value,uint16_t address){

    gb_event_manager_t* event_manager = &gb->event_manager;

    uint8_t type = gb_event_none_type;

    if(address >= 0x8000 && address <= 0x9FFF){
        type = gb_event_vram_type;
    }
    else if(address >= 0xC000 && address <= 0xFDFF){
        type = gb_event_wram_type;
    }
    else if(address >= 0xFE00 && address <= 0xFE9F){
        type = gb_event_oam_type;
    }
    else if(address >= 0xFF00){
        gb_event_register_map_t* map = event_manager->registers_map + (address & 0xFF);
        
        if(map->type != gb_event_none_type && (map->flags & flag) != 0){
            type = map->type;
        }
    }

    if(type != gb_event_none_type){
        gb_event_t event = {
            .scanline = gb->ppu.scanline,
            .cycle = gb->ppu.cycle,
            .pc = gb->cpu.instruction_pc,
            .type = type,
            .flag = flag,
            .address = address,
            .value = value
        };
        gb_event_manager_push_event(event_manager,&event);
    }
}


void gb_event_manager_swap_frame(gb_t* gb){
    gb_event_manager_t* event_manager = &gb->event_manager;

    if(!event_manager->enabled) return;
    
    event_manager->current_frame = (event_manager->current_frame == event_manager->frame + 0) ? event_manager->frame + 1 : event_manager->frame + 0;

    gb_event_frame_clear(event_manager->current_frame);
}


const gb_event_frame_t* gb_event_manager_current_frame(gb_t* gb){
    gb_event_manager_t* event_manager = &gb->event_manager;
    return event_manager->current_frame;
}

const gb_event_frame_t* gb_event_manager_previous_frame(gb_t* gb){
    gb_event_manager_t* event_manager = &gb->event_manager;
    return (event_manager->current_frame == event_manager->frame + 0) ? event_manager->frame + 1 : event_manager->frame + 0;
}

const uint8_t* gb_event_manager_screen(gb_t* gb){
    gb_event_manager_t* event_manager = &gb->event_manager;
    return event_manager->screen;
}


void gb_event_manager_enable(gb_t* gb,bool enabled){
    gb_event_manager_t* event_manager = &gb->event_manager;
    
    if(event_manager->enabled == enabled) return;

    event_manager->enabled = enabled;

    if(!event_manager->enabled){
        gb_event_manager_reset(event_manager);
    }
}

void gb_event_manager_reset(gb_event_manager_t* event_manager){

    event_manager->current_frame = event_manager->frame + 0;

    memset(event_manager->screen,0,sizeof(event_manager->screen));

    gb_event_frame_clear(event_manager->frame + 0);
    gb_event_frame_clear(event_manager->frame + 1);
}

void gb_event_manager_free(gb_event_manager_t* event_manager){
    gb_event_frame_free(event_manager->frame + 0);
    gb_event_frame_free(event_manager->frame + 1);
}