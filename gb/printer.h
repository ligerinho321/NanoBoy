#pragma once

#include "utils.h"

enum{
    gb_printer_palette_colors = 0x04,
    
    gb_printer_image_bytes_per_pixel = 3,
    gb_printer_image_pitch = gb_screen_width * gb_printer_image_bytes_per_pixel,

    gb_printer_bytes_per_tile_line = 2,
    gb_printer_bytes_per_tile = gb_tile_size * gb_printer_bytes_per_tile_line,
    gb_printer_bytes_per_row = gb_screen_columns * gb_printer_bytes_per_tile,

    gb_printer_packet_buffer_length = 0x280,
    gb_printer_ram_length = 0x2000,

    gb_printer_printing_freq = gb_clock_rate >> 0x05,
    gb_printer_timeout = gb_clock_rate >> 0x03
};

typedef enum _gb_printer_command_t {
    gb_printer_initialize_command = 0x01,
    gb_printer_start_printing_command = 0x02,
    gb_printer_fill_buffer_command = 0x04,
    gb_printer_status_command = 0x0F
} gb_printer_command_t;

typedef enum _gb_printer_status_t {
    gb_printer_checksum_error_status = 0x01,
    gb_printer_currently_printing_status = 0x02,
    gb_printer_image_data_full_status = 0x04,
    gb_printer_unprocessed_data_status = 0x08,
    gb_printer_packet_error_status = 0x10,
    gb_printer_paper_jam_status = 0x20,
    gb_printer_other_error_status = 0x40,
    gb_printer_low_battery_status = 0x80
} gb_printer_status_t;

typedef enum _gb_printer_packet_state_t {
    gb_printer_magic_byte0_state,
    gb_printer_magic_byte1_state,
    gb_printer_command_state,
    gb_printer_compression_flag_state,
    gb_printer_length_lsb_state,
    gb_printer_length_msb_state,
    gb_printer_data_state,
    gb_printer_run_state,
    gb_printer_run_data_state,
    gb_printer_checksum_lsb_state,
    gb_printer_checksum_msb_state,
    gb_printer_keepalive_state,
    gb_printer_status_state
} gb_printer_packet_state_t;

typedef enum _gb_printer_printing_state_t {
    gb_printer_top_padding_state,
    gb_printer_image_state,
    gb_printer_bottom_padding_state
} gb_printer_printing_state_t;


typedef void (*gb_printer_callback_t)(void* userdata,const uint8_t* data,int len);

typedef struct _gb_printer_t {
    gb_t* gb;

    gb_printer_packet_state_t state;
    
    uint8_t status;
    
    uint8_t sb;
    uint8_t bits_received;

    uint8_t command;
    bool compression_flag;
    uint16_t length;
    uint16_t checksum;

    bool run_compressed;
    uint8_t run_length;

    uint8_t packet_buffer[gb_printer_packet_buffer_length];
    uint32_t packet_buffer_length;

    uint8_t ram[gb_printer_ram_length];
    uint32_t ram_length;

    uint8_t printing_state;
    bool padding_enabled;
    uint8_t padding;
    uint8_t palette[gb_printer_palette_colors];
    uint32_t lines;
    uint32_t line;
    uint8_t line_buffer[gb_printer_image_pitch];

    int timer;
    bool accelerate;

    uint64_t last_bit_received;

    gb_printer_callback_t callback;
    void* userdata;
} gb_printer_t;


#ifdef __cplusplus
extern "C" {
#endif

void gb_printer_init(gb_printer_t* printer,gb_t* gb);

void gb_printer_set_callback(gb_printer_t* printer,gb_printer_callback_t callback,void* userdata);
void gb_printer_remove_callback(gb_printer_t* printer);

void gb_printer_clock(gb_printer_t* printer,int cycles);

bool gb_printer_receive_bit(void* data,bool bit);

void gb_printer_execute_state(gb_printer_t* printer);

void gb_printer_reset(gb_printer_t* printer);

#ifdef __cplusplus
}
#endif