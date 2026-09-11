#include "printer.h"
#include "gb.h"

#define gb_printer_packet_buffer_push_sb(printer){\
    if((printer)->packet_buffer_length + 0x01 > gb_printer_packet_buffer_length){\
        (printer)->packet_buffer_length = 0x00;\
    }\
    (printer)->packet_buffer[(printer)->packet_buffer_length++] = (printer)->sb;\
}

#define gb_printer_finish_printing(printer)\
    (printer)->status &= ~(gb_printer_currently_printing_status | gb_printer_image_data_full_status)


const uint8_t printer_palette[gb_printer_palette_colors] = {
    0xFF,0xAA,0x55,0x00
};


static inline void gb_printer_start_printing(gb_printer_t* printer){

    if(!(printer->status & gb_printer_image_data_full_status)) return;

    if(printer->packet_buffer_length != 0x04) return;

    if(printer->ram_length < gb_printer_bytes_per_row) return;


    printer->padding = printer->packet_buffer[0x01];

    uint8_t palette = printer->packet_buffer[0x02];

    for(uint8_t i = 0x00; i < gb_printer_palette_colors; ++i){
        printer->palette[i] = printer_palette[(palette >> (i << 0x01)) & 0x03];
    }

    uint32_t rows = printer->ram_length / gb_printer_bytes_per_row;
    printer->lines = rows * gb_tile_size;
    printer->line = 0;

    printer->status &= ~gb_printer_unprocessed_data_status;

    printer->status |= gb_printer_currently_printing_status;

    printer->timer = gb_printer_printing_freq;

    if(printer->padding_enabled && (printer->padding & 0xF0)){
        
        printer->printing_state = gb_printer_top_padding_state;

        memset(printer->line_buffer,printer_palette[0],sizeof(printer->line_buffer));
    }
    else{
        printer->printing_state = gb_printer_image_state;
    }

    printer->accelerate = false;
}

static inline void gb_printer_fill_buffer(gb_printer_t* printer){

    if(printer->ram_length + printer->packet_buffer_length <= gb_printer_ram_length){

        memcpy(printer->ram + printer->ram_length,printer->packet_buffer,printer->packet_buffer_length);
        
        printer->ram_length += printer->packet_buffer_length;

        if(printer->ram_length){
            printer->status |= gb_printer_unprocessed_data_status;
        }
    }

    if(printer->ram_length && !printer->packet_buffer_length){
        printer->status |= gb_printer_image_data_full_status;
    }
}


static inline void gb_printer_magic_byte0(gb_printer_t* printer){
    if(printer->sb == 0x88){
        printer->state = gb_printer_magic_byte1_state;
    }

    printer->status &= ~gb_printer_checksum_error_status;

    printer->sb = 0x00;
}

static inline void gb_printer_magic_byte1(gb_printer_t* printer){
    if(printer->sb == 0x33){
        printer->state = gb_printer_command_state;
    }
    else{
        printer->state = gb_printer_magic_byte0_state;
    }

    printer->sb = 0x00;
}

static inline void gb_printer_command(gb_printer_t* printer){
    printer->command = printer->sb;

    printer->checksum = printer->sb;

    printer->state = gb_printer_compression_flag_state;

    printer->sb = 0x00;
}

static inline void gb_printer_compression_flag(gb_printer_t* printer){
    printer->compression_flag = printer->sb;

    printer->checksum += printer->sb;

    printer->state = gb_printer_length_lsb_state;

    printer->sb = 0x00;
}

static inline void gb_printer_length_lsb(gb_printer_t* printer){
    printer->length = printer->sb;

    printer->checksum += printer->sb;

    printer->state = gb_printer_length_msb_state;

    printer->sb = 0x00;
}

static inline void gb_printer_length_msb(gb_printer_t* printer){
    printer->length |= printer->sb << 0x08;

    printer->checksum += printer->sb;

    printer->packet_buffer_length = 0;

    if(printer->length){
        printer->state = printer->compression_flag ? gb_printer_run_state : gb_printer_data_state;
    }
    else{
        printer->state = gb_printer_checksum_lsb_state;
    }

    printer->sb = 0x00;
}

static inline void gb_printer_data(gb_printer_t* printer){
    gb_printer_packet_buffer_push_sb(printer);            
    
    printer->checksum += printer->sb;

    if(--printer->length == 0x00){
        printer->state = gb_printer_checksum_lsb_state;
    }

    printer->sb = 0x00;
}

static inline void gb_printer_run(gb_printer_t* printer){
    printer->run_compressed = printer->sb & 0x80;

    printer->run_length = (printer->sb & 0x7F) + (printer->run_compressed ? 0x02 : 0x01);
    
    printer->checksum += printer->sb;

    if(--printer->length == 0x00){
        printer->state = gb_printer_checksum_lsb_state;
    }
    else{
        printer->state = gb_printer_run_data_state;
    }

    printer->sb = 0x00;
}

static inline void gb_printer_run_data(gb_printer_t* printer){
    if(printer->run_compressed){
        while(printer->run_length--){
            gb_printer_packet_buffer_push_sb(printer);
        }
        printer->state = gb_printer_run_state;

    }
    else{
        gb_printer_packet_buffer_push_sb(printer);
        if(--printer->run_length == 0x00){
            printer->state = gb_printer_run_state;
        }
    }

    printer->checksum += printer->sb;

    if(--printer->length == 0x00){
        printer->state = gb_printer_checksum_lsb_state;
    }

    printer->sb = 0x00;
}

static inline void gb_printer_checksum_lsb(gb_printer_t* printer){
    printer->checksum ^= printer->sb;

    printer->state = gb_printer_checksum_msb_state;

    printer->sb = 0x00;
}

static inline void gb_printer_checksum_msb(gb_printer_t* printer){
    printer->checksum ^= printer->sb << 0x08;
    
    if(printer->checksum){
        printer->status |= gb_printer_checksum_error_status;
    }

    printer->state = gb_printer_keepalive_state;

    printer->sb = 0x81;
}

static inline void gb_printer_keepalive(gb_printer_t* printer){
    printer->state = gb_printer_status_state;

    printer->sb = printer->status;
}

static inline void gb_printer_status(gb_printer_t* printer){

    if(!(printer->status & gb_printer_checksum_error_status)){
        
        switch(printer->command){
            case gb_printer_initialize_command:
                printer->ram_length = 0x00;
                printer->status = 0x00;
                break;
            case gb_printer_start_printing_command:
                gb_printer_start_printing(printer);
                break;
            case gb_printer_fill_buffer_command:
                gb_printer_fill_buffer(printer);
                break;
            case gb_printer_status_command:
                printer->status &= ~gb_printer_unprocessed_data_status;
                break;
        }

    }

    printer->state = gb_printer_magic_byte0_state;

    printer->sb = 0x00;
}


void gb_printer_init(gb_printer_t* printer,gb_t* gb){
    printer->gb = gb;
}


void gb_printer_set_callback(gb_printer_t* printer,gb_printer_callback_t callback,void* userdata){
    printer->callback = callback;
    printer->userdata = userdata;
}

void gb_printer_remove_callback(gb_printer_t* printer){
    printer->callback = NULL;
    printer->userdata = NULL;
}


void gb_printer_clock(gb_printer_t* printer,int cycles){
    
    if(!printer->accelerate){
        
        printer->timer -= cycles;

        if(printer->timer > 0){
            return;
        }
    }

    printer->timer = gb_printer_printing_freq;
        
    switch(printer->printing_state){
        case gb_printer_top_padding_state:{
                                
            uint8_t top_padding = printer->padding >> 0x04;
            
            if(--top_padding == 0x00){
                printer->printing_state = gb_printer_image_state;
            }
            else{
                printer->padding = ((top_padding & 0x0F) << 0x04) | (printer->padding & 0x0F);
            }

            break;
        }
        case gb_printer_image_state:{

            uint8_t* dst = printer->line_buffer;

            uint32_t row = printer->line >> 0x03;
            uint32_t y = printer->line & 0x07;

            uint8_t* src = printer->ram + row * gb_printer_bytes_per_row + y * gb_printer_bytes_per_tile_line;

            for(int col = 0; col < gb_screen_columns; ++col){
                
                for(uint8_t bit = 0x80; bit > 0x00; bit >>= 0x01){
                    
                    uint8_t color_index = ((src[1] & bit) ? 0x02 : 0x00) | ((src[0] & bit) ? 0x01 : 0x00);
                    
                    uint8_t color = printer->palette[color_index];

                    *dst++ = color;
                    *dst++ = color;
                    *dst++ = color;
                }

                src += gb_printer_bytes_per_tile;
            }

            if(++printer->line >= printer->lines){
                if(printer->padding_enabled && (printer->padding & 0x0F)){
                    
                    printer->printing_state = gb_printer_bottom_padding_state;

                    memset(printer->line_buffer,printer_palette[0],sizeof(printer->line_buffer));
                }
                else{
                    gb_printer_finish_printing(printer);
                }
            }

            break;
        }
        case gb_printer_bottom_padding_state:{

            uint8_t bottom_padding = printer->padding & 0x0F;
            
            if(--bottom_padding == 0x00){
                gb_printer_finish_printing(printer);
            }
            else{
                printer->padding = (printer->padding & 0xF0) | (bottom_padding & 0x0F);
            }

            break;
        }
        default:{
            gb_printf_error("printing state invalid");
            gb_printer_finish_printing(printer);
            break;
        }
    }

    if(printer->callback != NULL){
        printer->callback(printer->userdata,printer->line_buffer,sizeof(printer->line_buffer));
    }
}


bool gb_printer_receive_bit(void* data,bool bit){
    gb_printer_t* printer = (gb_printer_t*)data;

    uint64_t cycle = printer->gb->state.cycle;

    if(cycle - printer->last_bit_received >= gb_printer_timeout){
        printer->bits_received = 0x00;
        printer->state = gb_printer_magic_byte0_state;
    }

    printer->last_bit_received = cycle;

    bool send_bit = printer->sb & 0x80;
    
    printer->sb = (printer->sb << 0x01) | bit;

    if(++printer->bits_received >= 0x08){
        printer->bits_received = 0x00;
        
        gb_printer_execute_state(printer);
    }
    
    return send_bit;
}


void gb_printer_execute_state(gb_printer_t* printer){

    switch(printer->state){
        
        case gb_printer_magic_byte0_state: gb_printer_magic_byte0(printer); break;

        case gb_printer_magic_byte1_state: gb_printer_magic_byte1(printer); break;
        
        case gb_printer_command_state: gb_printer_command(printer); break;
        
        case gb_printer_compression_flag_state: gb_printer_compression_flag(printer); break;
        
        case gb_printer_length_lsb_state: gb_printer_length_lsb(printer); break;

        case gb_printer_length_msb_state: gb_printer_length_msb(printer); break;
        
        case gb_printer_data_state: gb_printer_data(printer); break;
        
        case gb_printer_run_state: gb_printer_run(printer); break;
        
        case gb_printer_run_data_state: gb_printer_run_data(printer); break;
        
        case gb_printer_checksum_lsb_state: gb_printer_checksum_lsb(printer); break;
        
        case gb_printer_checksum_msb_state: gb_printer_checksum_msb(printer); break;
        
        case gb_printer_keepalive_state: gb_printer_keepalive(printer); break;
        
        case gb_printer_status_state: gb_printer_status(printer); break;
        
        default:{            
            printer->state = gb_printer_magic_byte0_state;
            printer->sb = 0x00;
            break;
        }
    }
}


void gb_printer_reset(gb_printer_t* printer){
    printer->state = gb_printer_magic_byte0_state;
    
    printer->status = 0x00;

    printer->sb = 0x00;
    printer->bits_received = 0x00;

    printer->packet_buffer_length = 0x00;

    printer->ram_length = 0x00;

    printer->last_bit_received = 0x00;
}
