#pragma once

#include "../utils.h"

enum{
    gb_event_screen_bytes_per_pixel = 3,
    gb_event_screen_pitch = gb_scanline_cycles * gb_event_screen_bytes_per_pixel,
    gb_event_screen_length = gb_event_screen_pitch * gb_scanlines,

    gb_event_frame_inital_capacity = gb_frame_cycles / 2
};

typedef enum _gb_event_type_t {
    gb_event_none_type,
    
    gb_event_halt_type,
    gb_event_stop_type,
    
    gb_event_irq_vblank_type,
    gb_event_irq_lcd_type,
    gb_event_irq_timer_type,
    gb_event_irq_serial_type,
    gb_event_irq_joypad_type,

    gb_event_vram_type,         //0x8000-0x9FFF
    gb_event_wram_type,         //0xC000-0xDFFF,0xE000-0xFDFF
    gb_event_oam_type,          //0xFE00-0xFE9F
    gb_event_hram_type,         //0xFF80-0xFFFE
    
    gb_event_joypad_type,       //0xFF00
    gb_event_serial_type,       //0xFF01-0xFF02
    gb_event_timer_type,        //0xFF04-0xFF07
    gb_event_interrupt_type,    //0xFF0F,0xFFFF
    gb_event_square1_type,      //0xFF10-0xFF14
    gb_event_square2_type,      //0xFF16-0xFF19
    gb_event_wave_type,         //0xFF1A-0xFF1E
    gb_event_noise_type,        //0xFF20-0xFF23
    gb_event_apu_control_type,  //0xFF24-0xFF26
    gb_event_wave_ram_type,     //0xFF30-0xFF3F
    gb_event_ppu_type,          //0xFF40-0xFF45,0xFF4A-0xFF4B
    gb_event_palette_type,      //0xFF47-0xFF49,0xFF68-FF6B
    gb_event_dma_type,          //0xFF46,0xFF51-0xFF55
    gb_event_others_type,       //0xFF4C,0xFF4D,0xFF4F,0xFF50,0xFF56,0xFF6C,0xFF70,0xFF72-0xFF75,0xFF76-0xFF77
    
    gb_event_type_count
} gb_event_type_t;

extern const char* gb_event_type_names[gb_event_type_count];


typedef enum _gb_event_flag_t {
    gb_event_write_flag = 0x01,
    gb_event_read_flag = 0x02,
    gb_event_interrupt_flag = 0x04,
} gb_event_flag_t;


typedef enum _gb_event_screen_color {
    gb_event_screen_hblank_color,
    gb_event_screen_vblank_color,
    gb_event_screen_oam_scan_color,
    gb_event_screen_fictitious_fetch_color,
    gb_event_screen_background_fetch_color,
    gb_event_screen_window_fetch_color,
    gb_event_screen_object_fetch_color,
    gb_event_screen_color_count
} gb_event_screen_color;

extern const gb_rgb_t gb_event_screen_palette[gb_event_screen_color_count];

extern const char* gb_event_screen_color_names[gb_event_screen_color_count];


typedef struct _gb_event_t {
    uint8_t scanline;
    uint16_t cycle;
    uint16_t pc;
    uint8_t type;
    uint8_t flag;
    uint16_t address;
    uint8_t value;
} gb_event_t;

typedef struct _gb_event_range_t {
    uint32_t start;
    uint32_t count;
} gb_event_range_t;

typedef struct _gb_event_frame_t {
    gb_event_range_t range[gb_frame_cycles];
    gb_event_t* event;
    uint32_t count;
    uint32_t capacity;
} gb_event_frame_t;

typedef struct _gb_event_register_map_t {
    uint8_t type;
    uint8_t flags;
} gb_event_register_map_t;

typedef struct _gb_event_manager_t {
    gb_t* gb;

    bool enabled;
    
    gb_event_register_map_t registers_map[0x100];

    uint8_t screen[gb_event_screen_length];

    gb_event_frame_t frame[2];

    gb_event_frame_t* current_frame;
} gb_event_manager_t;


#ifdef __cplusplus
extern "C" {
#endif

void gb_event_manager_init(gb_event_manager_t* event_manager,gb_t* gb);

void gb_event_manager_map(gb_event_manager_t* event_manager);
void gb_event_manager_update_mapping(gb_event_manager_t* event_manager);

void gb_event_manager_screen_blank(gb_t* gb);

void gb_event_manager_halt(gb_t* gb);
void gb_event_manager_stop(gb_t* gb);
void gb_event_manager_irq(gb_t* gb,uint8_t vector);
void gb_event_manager_io(gb_t* gb,uint8_t flag,uint8_t value,uint16_t address);

void gb_event_manager_swap_frame(gb_t* gb);

const gb_event_frame_t* gb_event_manager_current_frame(gb_t* gb);
const gb_event_frame_t* gb_event_manager_previous_frame(gb_t* gb);
const uint8_t* gb_event_manager_screen(gb_t* gb);

void gb_event_manager_enable(gb_t* gb,bool enabled);

void gb_event_manager_reset(gb_event_manager_t* event_manager);

void gb_event_manager_free(gb_event_manager_t* event_manager);

#ifdef __cplusplus
}
#endif