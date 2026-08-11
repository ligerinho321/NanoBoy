#include "apu.h"
#include "gb.h"

const bool square_duty_table[4][8] = {
    {0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,1,1},
    {0,1,1,1,1,1,1,0}
};

const uint8_t wave_volume_shift[4] = {
    4,0,1,2
};

const uint8_t noise_divisor[8] = {
    8,16,32,48,64,80,96,112
};


void gb_apu_init(gb_apu_t* apu,gb_t* gb){
    apu->gb = gb;

    apu->square1_frame.blip = blip_new(gb_audio_frame_samples);
    apu->square2_frame.blip = blip_new(gb_audio_frame_samples);
    apu->wave_frame.blip = blip_new(gb_audio_frame_samples);
    apu->noise_frame.blip = blip_new(gb_audio_frame_samples);

    apu->mixer_frame.blip_left = blip_new(gb_audio_frame_samples);
    apu->mixer_frame.blip_right = blip_new(gb_audio_frame_samples);

    gb_apu_update_rates(apu);

    gb_ring_buffer_init(&apu->ring_buffer,gb_ring_buffer_size);

    apu->square1.apu = apu;
    apu->square1.has_sweep = true;

    apu->square2.apu = apu;
    apu->square2.has_sweep = false;

    apu->wave.apu = apu;

    apu->noise.apu = apu;

    apu->square1.external_enabled = true;
    apu->square2.external_enabled = true;
    apu->wave.external_enabled = true;
    apu->noise.external_enabled = true;

    apu->square1_register_descriptor = (gb_memory_descriptor_t){
        gb_apu_write_square_register,
        gb_apu_read_square_register,
        &apu->square1
    };

    apu->square2_register_descriptor = (gb_memory_descriptor_t){
        gb_apu_write_square_register,
        gb_apu_read_square_register,
        &apu->square2
    };

    apu->wave_register_descriptor = (gb_memory_descriptor_t){
        gb_apu_write_wave_register,
        gb_apu_read_wave_register,
        &apu->wave
    };

    apu->noise_register_descriptor = (gb_memory_descriptor_t){
        gb_apu_write_noise_register,
        gb_apu_read_noise_register,
        &apu->noise
    };

    apu->register_descriptor = (gb_memory_descriptor_t){
        gb_apu_write_register,
        gb_apu_read_register,
        apu
    };

    apu->wave_ram_descriptor = (gb_memory_descriptor_t){
        gb_apu_write_wave_ram,
        gb_apu_read_wave_ram,
        &apu->wave
    };
    
    apu->pcm12_register_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_empty,
        gb_apu_read_pcm12_register,
        apu
    };

    apu->pcm34_register_descriptor = (gb_memory_descriptor_t){
        gb_memory_write_empty,
        gb_apu_read_pcm34_register,
        apu
    };
}


void gb_apu_add_handler(gb_apu_t* apu,gb_apu_handler_t* handler){
    gb_list_add_element(apu->handlers,handler,gb_apu_handler_t);
}

void gb_apu_remove_handler(gb_apu_t* apu,gb_apu_handler_t* handler){
    gb_list_remove_element(apu->handlers,handler,gb_apu_handler_t);

    if(!apu->handlers){
        gb_apu_channel_frame_reset(&apu->square1_frame);
        gb_apu_channel_frame_reset(&apu->square2_frame);
        gb_apu_channel_frame_reset(&apu->wave_frame);
        gb_apu_channel_frame_reset(&apu->noise_frame);
    }
}


void gb_apu_update_rates(gb_apu_t* apu){
    int clock_rate = gb_clock_rate * apu->gb->speed;

    blip_set_rates(apu->square1_frame.blip,clock_rate,gb_audio_sample_rate);
    blip_set_rates(apu->square2_frame.blip,clock_rate,gb_audio_sample_rate);
    blip_set_rates(apu->wave_frame.blip,clock_rate,gb_audio_sample_rate);
    blip_set_rates(apu->noise_frame.blip,clock_rate,gb_audio_sample_rate);

    blip_set_rates(apu->mixer_frame.blip_left,clock_rate,gb_audio_sample_rate);
    blip_set_rates(apu->mixer_frame.blip_right,clock_rate,gb_audio_sample_rate);
}


void gb_apu_update_output(gb_apu_t* apu){
    
    int clock_time = apu->frame_cycles - 1;
    
    if(clock_time < 0) return;

    int square1_output = gb_apu_square_output(&apu->square1);
    int square2_output = gb_apu_square_output(&apu->square2);
    int wave_output = gb_apu_wave_output(&apu->wave);
    int noise_output = gb_apu_noise_output(&apu->noise);

    int left_output = (
        square1_output * apu->square1_panning.left +
        square2_output * apu->square2_panning.left +
        wave_output * apu->wave_panning.left +
        noise_output * apu->noise_panning.left
    );
    left_output *= apu->volume.left + 0x01;

    int right_output = (
        square1_output * apu->square1_panning.right +
        square2_output * apu->square2_panning.right +
        wave_output * apu->wave_panning.right +
        noise_output * apu->noise_panning.right
    );
    right_output *= apu->volume.right + 0x01;


    if(left_output != apu->mixer_frame.last_left_output){
        blip_add_delta(apu->mixer_frame.blip_left,clock_time,left_output - apu->mixer_frame.last_left_output);
        apu->mixer_frame.last_left_output = left_output;
    }

    if(right_output != apu->mixer_frame.last_right_output){
        blip_add_delta(apu->mixer_frame.blip_right,clock_time,right_output - apu->mixer_frame.last_right_output);
        apu->mixer_frame.last_right_output = right_output;
    }

    if(apu->handlers != NULL){
        if(square1_output != apu->square1_frame.last_output){
            blip_add_delta(apu->square1_frame.blip,clock_time,square1_output - apu->square1_frame.last_output);
            apu->square1_frame.last_output = square1_output;
        }

        if(square2_output != apu->square2_frame.last_output){
            blip_add_delta(apu->square2_frame.blip,clock_time,square2_output - apu->square2_frame.last_output);
            apu->square2_frame.last_output = square2_output;
        }

        if(wave_output != apu->wave_frame.last_output){
            blip_add_delta(apu->wave_frame.blip,clock_time,wave_output - apu->wave_frame.last_output);
            apu->wave_frame.last_output = wave_output;
        }

        if(noise_output != apu->noise_frame.last_output){
            blip_add_delta(apu->noise_frame.blip,clock_time,noise_output - apu->noise_frame.last_output);
            apu->noise_frame.last_output = noise_output;
        }
    }
}


void gb_apu_channel_frame_end(gb_apu_channel_frame_t* channel_frame){

    blip_end_frame(channel_frame->blip,gb_frame_cycles);
    
    channel_frame->samples_count = blip_samples_avail(channel_frame->blip);

    blip_read_samples(channel_frame->blip,channel_frame->samples,channel_frame->samples_count,0);

    int16_t* s = channel_frame->samples;
    int16_t* e = channel_frame->samples + channel_frame->samples_count;

    float capacitor = channel_frame->capacitor;
    
    while(s != e){
        float out = *s - capacitor;
        capacitor = *s - out * gb_high_pass_factor;
        *s = (int16_t)out;
        ++s;
    }

    channel_frame->capacitor = capacitor;
}

void gb_apu_channel_frame_reset(gb_apu_channel_frame_t* channel_frame){
    blip_clear(channel_frame->blip);
    
    channel_frame->last_output = 0;

    channel_frame->capacitor = 0.0f;
}


void gb_apu_mixer_frame_end(gb_apu_mixer_frame_t* mixer_frame){

    blip_end_frame(mixer_frame->blip_left,gb_frame_cycles);
    blip_end_frame(mixer_frame->blip_right,gb_frame_cycles);

    int left_samples = blip_samples_avail(mixer_frame->blip_left);
    int right_samples = blip_samples_avail(mixer_frame->blip_right);

    blip_read_samples(mixer_frame->blip_left,mixer_frame->samples + 0,left_samples,1);
    blip_read_samples(mixer_frame->blip_right,mixer_frame->samples + 1,right_samples,1);

    mixer_frame->samples_count = left_samples + right_samples;

    int16_t* s = mixer_frame->samples;
    int16_t* e = mixer_frame->samples + mixer_frame->samples_count;

    float left_capacitor = mixer_frame->left_capacitor;
    float right_capacitor = mixer_frame->right_capacitor;

    while(s != e){
        float out_left = *s - left_capacitor;
        left_capacitor = *s - out_left * gb_high_pass_factor;
        *s = (int16_t)out_left;
        ++s;

        float out_right = *s - right_capacitor;
        right_capacitor = *s - out_right * gb_high_pass_factor;
        *s = (int16_t)out_right;
        ++s;
    }

    mixer_frame->left_capacitor = left_capacitor;
    mixer_frame->right_capacitor = right_capacitor;
}

void gb_apu_mixer_frame_reset(gb_apu_mixer_frame_t* mixer_frame){
    blip_clear(mixer_frame->blip_left);
    blip_clear(mixer_frame->blip_right);
    
    mixer_frame->last_left_output = 0;
    mixer_frame->last_right_output = 0;

    mixer_frame->left_capacitor = 0.0f;
    mixer_frame->right_capacitor = 0.0f;
}


void gb_apu_frame_end(gb_apu_t* apu){
    if(apu->frame_cycles < gb_frame_cycles) return;

    apu->frame_cycles = 0;

    gb_apu_mixer_frame_end(&apu->mixer_frame);

    if(apu->handlers != NULL){
        
        gb_apu_channel_frame_end(&apu->square1_frame);
        gb_apu_channel_frame_end(&apu->square2_frame);
        gb_apu_channel_frame_end(&apu->wave_frame);
        gb_apu_channel_frame_end(&apu->noise_frame);

        gb_apu_handler_t* handler = apu->handlers;
        do{
            handler->callback(handler->userdata);
            handler = handler->next;
        }while(handler != NULL);
    }

    int len = apu->mixer_frame.samples_count * gb_audio_bytes_per_sample;

    while(gb_ring_buffer_writeable(&apu->ring_buffer) < len){
#ifdef _WIN32
        Sleep(1);
#else
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1000000;
        nanosleep(&ts,NULL);
#endif
    }

    gb_ring_buffer_write(&apu->ring_buffer,(uint8_t*)apu->mixer_frame.samples,len);
}


void gb_apu_run(gb_apu_t* apu){

    uint64_t cycles_to_run = apu->cycles - apu->last_clock_cycle;
    apu->last_clock_cycle = apu->cycles;

    while(cycles_to_run > 0){

        uint64_t cycles = cycles_to_run;

        uint32_t frame_cycles_remaining = gb_frame_cycles - apu->frame_cycles;

        if(frame_cycles_remaining < cycles){
            cycles = frame_cycles_remaining;
        }
        if(apu->square1.enabled && apu->square1.timer < cycles){
            cycles = apu->square1.timer;
        }
        if(apu->square2.enabled && apu->square2.timer < cycles){
            cycles = apu->square2.timer;
        }
        if(apu->wave.enabled && apu->wave.timer < cycles){
            cycles = apu->wave.timer;
        }
        if(apu->noise.enabled && apu->noise.timer < cycles){
            cycles = apu->noise.timer;
        }

        apu->frame_cycles += cycles;

        gb_apu_square_clock(&apu->square1,cycles);
        gb_apu_square_clock(&apu->square2,cycles);
        gb_apu_wave_clock(&apu->wave,cycles);
        gb_apu_noise_clock(&apu->noise,cycles);

        gb_apu_update_output(apu);

        gb_apu_frame_end(apu);

        cycles_to_run -= cycles;
    }
}


void gb_apu_frame_sequencer_clock(gb_apu_t* apu){
    
    gb_apu_run(apu);

    if(apu->enabled){
        if(!apu->skip_first_frame_sequence_event){
            
            if(!(apu->frame_sequencer & 0x01)){
                gb_apu_length_counter_clock(&apu->square1.length_counter,&apu->square1.enabled);
                gb_apu_length_counter_clock(&apu->square2.length_counter,&apu->square2.enabled);
                gb_apu_length_counter_clock(&apu->wave.length_counter,&apu->wave.enabled);
                gb_apu_length_counter_clock(&apu->noise.length_counter,&apu->noise.enabled);
            }

            if(apu->frame_sequencer == 0x07){
                gb_apu_envelope_clock(&apu->square1.envelope);
                gb_apu_envelope_clock(&apu->square2.envelope);
                gb_apu_envelope_clock(&apu->noise.envelope);
            }

            if((apu->frame_sequencer & 0x03) == 0x02){
                gb_apu_sweep_clock(&apu->square1);
            }

            apu->frame_sequencer = (apu->frame_sequencer + 0x01) & 0x07;
        }
        else{
            apu->skip_first_frame_sequence_event = false;
        }
    }
}


void gb_apu_write_register(void* data,uint8_t value,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;

    gb_apu_run(apu);

    switch(address){
        case 0xFF24:{
            if(!apu->enabled) break;

            apu->vin_panning.left = value & 0x80;
            apu->vin_panning.right = value & 0x08;
            apu->volume.left = (value & 0x70) >> 0x04;
            apu->volume.right = value & 0x07;
            break;
        }
        case 0xFF25:{
            if(!apu->enabled) break;

            apu->square1_panning.left = value & 0x10;
            apu->square1_panning.right = value & 0x01;

            apu->square2_panning.left = value & 0x20;
            apu->square2_panning.right = value & 0x02;

            apu->wave_panning.left = value & 0x40;
            apu->wave_panning.right = value & 0x04;

            apu->noise_panning.left = value & 0x80;
            apu->noise_panning.right = value & 0x08;
            break;
        }
        case 0xFF26:{
            bool new_enabled = value & 0x80;
            
            if(!apu->enabled && new_enabled){
                apu->enabled = true;

                //Segundo o teste div_write_trigger_10 habilitar o canal quando o 4bit (ou 5bit em double speed) 
                //de timer DIV está ativo faz com que o primeiro clock do senquenciador de quadros seja ignorado
                apu->skip_first_frame_sequence_event = apu->gb->timer.div & (apu->gb->double_speed ? 0x2000 : 0x1000);
            }
            else if(apu->enabled && !new_enabled){
                gb_apu_reset(apu,false);
            }
            break;
        }
    }
}

uint8_t gb_apu_read_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;

    gb_apu_run(apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF24:{
            value = (
                (apu->volume.right & 0x07) |
                (apu->vin_panning.right ? 0x08 : 0x00) |
                ((apu->volume.left & 0x07) << 0x04) |
                (apu->vin_panning.left ? 0x80 : 0x00)
            );
            break;
        }
        case 0xFF25:{
            value = (
                (apu->square1_panning.right ? 0x01 : 0x00) |
                (apu->square2_panning.right ? 0x02 : 0x00) |
                (apu->wave_panning.right ? 0x04 : 0x00) |
                (apu->noise_panning.right ? 0x08 : 0x00) |
                (apu->square1_panning.left ? 0x10 : 0x00) |
                (apu->square2_panning.left ? 0x20 : 0x00) | 
                (apu->wave_panning.left ? 0x40 : 0x00) |
                (apu->noise_panning.left ? 0x80 : 0x00)
            );
            break;
        }
        case 0xFF26:{
            value = (
                (apu->square1.enabled ? 0x01 : 0x00) |
                (apu->square2.enabled ? 0x02 : 0x00) |
                (apu->wave.enabled ? 0x04 : 0x00) |
                (apu->noise.enabled ? 0x08 : 0x00) |
                0x70 | 
                (apu->enabled ? 0x80 : 0x00)
            );
            break;
        }
    }

    return value;
}


static inline uint16_t gb_apu_sweep_get_new_frequency(gb_apu_sweep_t* sweep){
    uint16_t delta = sweep->shadow_frequency >> sweep->shift;
    return sweep->negate ? sweep->shadow_frequency - delta : sweep->shadow_frequency + delta;
}

void gb_apu_sweep_clock(gb_apu_square_t* square){
    
    gb_apu_sweep_t* sweep = &square->sweep;

    if(!sweep->enabled || --sweep->timer > 0x00) return;

    if(sweep->period){
        sweep->timer = sweep->period;
    }
    else{
        sweep->timer = 0x08;
        return;
    }

    uint16_t new_frequency = gb_apu_sweep_get_new_frequency(sweep);

    sweep->calc_negate = sweep->negate;
    
    if(new_frequency > 0x7FF){
        square->enabled = false;
        return;
    }

    if(sweep->shift > 0x00){
        sweep->shadow_frequency = new_frequency;
        square->frequency = new_frequency;

        new_frequency = gb_apu_sweep_get_new_frequency(sweep);

        if(new_frequency > 0x7FF){
            square->enabled = false;
        }
    }
}

void gb_apu_sweep_save_state(gb_apu_sweep_t* sweep,gb_state_t* state){
    gb_state_write(state,sweep->enabled);
    gb_state_write(state,sweep->period);
    gb_state_write(state,sweep->timer);
    gb_state_write(state,sweep->negate);
    gb_state_write(state,sweep->calc_negate);
    gb_state_write(state,sweep->shift);
    gb_state_write(state,sweep->shadow_frequency);
}

void gb_apu_sweep_load_state(gb_apu_sweep_t* sweep,gb_state_t* state){
    gb_state_read(state,sweep->enabled);
    gb_state_read(state,sweep->period);
    gb_state_read(state,sweep->timer);
    gb_state_read(state,sweep->negate);
    gb_state_read(state,sweep->calc_negate);
    gb_state_read(state,sweep->shift);
    gb_state_read(state,sweep->shadow_frequency);
}


void gb_apu_envelope_clock(gb_apu_envelope_t* envelope){
    if(--envelope->timer > 0x00) return;

    if(envelope->period){
        envelope->timer = envelope->period;
    }
    else{
        envelope->timer = 0x08;
        return;
    }

    if(!envelope->automatic_change) return;

    if(envelope->add_mode){
        if(envelope->current_volume < 0x0F){
            ++envelope->current_volume;
        }
        else{
            envelope->automatic_change = false;
        }
    }
    else{
        if(envelope->current_volume > 0x00){
            --envelope->current_volume;
        }
        else{
            envelope->automatic_change = false;
        }
    }
}

void gb_apu_envelope_save_state(gb_apu_envelope_t* envelope,gb_state_t* state){
    gb_state_write(state,envelope->initial_volume);
    gb_state_write(state,envelope->current_volume);
    gb_state_write(state,envelope->add_mode);
    gb_state_write(state,envelope->period);
    gb_state_write(state,envelope->timer);
    gb_state_write(state,envelope->automatic_change);
}

void gb_apu_envelope_load_state(gb_apu_envelope_t* envelope,gb_state_t* state){
    gb_state_read(state,envelope->initial_volume);
    gb_state_read(state,envelope->current_volume);
    gb_state_read(state,envelope->add_mode);
    gb_state_read(state,envelope->period);
    gb_state_read(state,envelope->timer);
    gb_state_read(state,envelope->automatic_change);
}


void gb_apu_length_counter_extra_clock(gb_apu_t* apu,gb_apu_length_counter_t* length_counter,uint8_t value,uint16_t length,bool* channel_enabled){

    bool new_length_counter_enabled = value & 0x40;

    if(((apu->frame_sequencer & 0x01) || apu->skip_first_frame_sequence_event) && !length_counter->enabled && new_length_counter_enabled){
        if(length_counter->counter > 0x00 && --length_counter->counter == 0x00){
            if(!(value & 0x80)){
                *channel_enabled = false;
            }
            else{
                length_counter->counter = length;
            }
        }
    }

    length_counter->enabled = new_length_counter_enabled;
}

void gb_apu_length_counter_clock(gb_apu_length_counter_t* length_counter,bool* channel_enabled){
    if(!length_counter->enabled) return;

    if(length_counter->counter && --length_counter->counter == 0x00){
        *channel_enabled = false;
    }
}

void gb_apu_length_counter_save_state(gb_apu_length_counter_t* length_counter,gb_state_t* state){
    gb_state_write(state,length_counter->enabled);
    gb_state_write(state,length_counter->counter);
}

void gb_apu_length_counter_load_state(gb_apu_length_counter_t* length_counter,gb_state_t* state){
    gb_state_read(state,length_counter->enabled);
    gb_state_read(state,length_counter->counter);
}



void gb_apu_square_clock(gb_apu_square_t* square,int timer){

    if(!square->enabled) return;

    square->timer -= timer;

    if(square->timer > 0x00) return;

    square->timer = (0x800 - square->frequency) << 0x02;

    square->duty_pos = (square->duty_pos + 0x01) & 0x07;

    gb_apu_square_update_output(square);
}


void gb_apu_write_square_register(void* data,uint8_t value,uint16_t address){
    gb_apu_square_t* square = (gb_apu_square_t*)data;

    gb_apu_run(square->apu);

    switch(address){
        case 0xFF10:{
            if(!square->apu->enabled) break;

            square->sweep.period = (value & 0x70) >> 0x04;
            square->sweep.shift = value & 0x07;

            bool new_negate = value & 0x08;

            if(square->sweep.negate && !new_negate && square->sweep.calc_negate){
                square->enabled = false;
            }

            square->sweep.negate = new_negate;
            break;
        }
        case 0xFF11: case 0xFF16:{
            if(square->apu->enabled){
                square->duty = value >> 0x06;
            }
            if(square->apu->enabled || !square->apu->gb->is_cgb){
                square->length_counter.counter = 0x40 - (value & 0x3F);
            }
            break;
        }
        case 0xFF12: case 0xFF17:{
            if(!square->apu->enabled) break;

            square->envelope.initial_volume = value >> 0x04;
            square->envelope.add_mode = value & 0x08;
            square->envelope.period = value & 0x07;

            if(square->enabled && !(value & 0xF8)){
                square->enabled = false;
            }
            break;
        }
        case 0xFF13: case 0xFF18:{
            if(!square->apu->enabled) break;

            square->frequency = (square->frequency & 0x700) | value;
            break;
        }
        case 0xFF14: case 0xFF19:{
            if(!square->apu->enabled) break;

            square->frequency = ((value & 0x07) << 0x08) | (square->frequency & 0xFF);

            if(value & 0x80){
                square->enabled = square->envelope.initial_volume || square->envelope.add_mode;

                if(square->length_counter.counter == 0x00){
                    square->length_counter.counter = 0x40;
                    square->length_counter.enabled = false;
                }

                square->timer = (0x800 - square->frequency) << 0x02;

                square->envelope.timer = square->envelope.period ? square->envelope.period : 0x08;
                square->envelope.current_volume = square->envelope.initial_volume;
                square->envelope.automatic_change = true;

                if(square->has_sweep){
                    square->sweep.shadow_frequency = square->frequency;
                    square->sweep.timer = square->sweep.period ? square->sweep.period : 0x08;
                    square->sweep.enabled = (square->sweep.period != 0x00) | (square->sweep.shift != 0x00);
                    square->sweep.calc_negate = false;

                    if(square->sweep.shift != 0x00){
                        if(gb_apu_sweep_get_new_frequency(&square->sweep) > 0x7FF){
                            square->enabled = false;
                        }
                        square->sweep.calc_negate = square->sweep.negate;
                    }
                }
            }

            gb_apu_length_counter_extra_clock(square->apu,&square->length_counter,value,0x3F,&square->enabled);
            break;
        }
    }
}

uint8_t gb_apu_read_square_register(void* data,uint16_t address){
    gb_apu_square_t* square = (gb_apu_square_t*)data;

    gb_apu_run(square->apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF10:
            value = (
                0x80 | 
                ((square->sweep.period & 0x07) << 0x04) | 
                (square->sweep.negate ? 0x08 : 0x00) | 
                (square->sweep.shift & 0x07)
            );
            break;
        case 0xFF11: case 0xFF16:
            value = ((square->duty & 0x03) << 0x06) | 0x3F;
            break;
        case 0xFF12: case 0xFF17:
            value = (
                ((square->envelope.initial_volume & 0x0F) << 0x04) | 
                (square->envelope.add_mode ? 0x08 : 0x00) | 
                (square->envelope.period & 0x07)
            );
            break;
        case 0xFF14: case 0xFF19:
            value = 0xBF | (square->length_counter.enabled ? 0x40 : 0x00);
            break;
    }

    return value;
}


uint8_t gb_apu_square_raw_output(gb_apu_square_t* square){
    if(square->enabled){
        return square->envelope.current_volume * square_duty_table[square->duty][square->duty_pos];
    }
    return 0x00;
}

void gb_apu_square_update_output(gb_apu_square_t* square){
    if(square->enabled){
        uint8_t output = square->envelope.current_volume * square_duty_table[square->duty][square->duty_pos];
        
        square->output = (7 - output) << gb_audio_channel_volume_shift;
    }
    else{
        square->output = 0;
    }
}

int gb_apu_square_output(gb_apu_square_t* square){
    return square->external_enabled ? square->output : 0;
}


void gb_apu_square_reset(gb_apu_square_t* square,bool hardware){
    square->enabled = false;

    if(square->has_sweep){
        memset(&square->sweep,0x00,sizeof(square->sweep));
    }
    
    memset(&square->envelope,0x00,sizeof(square->envelope));

    square->length_counter.enabled = false;
    if(hardware || square->apu->gb->is_cgb){
        square->length_counter.counter = 0x00;
    }

    square->duty = 0x00;
    square->duty_pos = 0x00;

    square->frequency = 0x00;
    square->timer = (0x800 - square->frequency) << 0x02;

    square->output = 0;
}


void gb_apu_square_save_state(gb_apu_square_t* square,gb_state_t* state){
    gb_state_write(state,square->enabled);
    if(square->has_sweep){
        gb_apu_sweep_save_state(&square->sweep,state);
    }
    gb_apu_envelope_save_state(&square->envelope,state);
    gb_apu_length_counter_save_state(&square->length_counter,state);
    gb_state_write(state,square->duty);
    gb_state_write(state,square->duty_pos);
    gb_state_write(state,square->frequency);
    gb_state_write(state,square->timer);
}

void gb_apu_square_load_state(gb_apu_square_t* square,gb_state_t* state){
    gb_state_read(state,square->enabled);
    if(square->has_sweep){
        gb_apu_sweep_load_state(&square->sweep,state);
    }
    gb_apu_envelope_load_state(&square->envelope,state);
    gb_apu_length_counter_load_state(&square->length_counter,state);
    gb_state_read(state,square->duty);
    gb_state_read(state,square->duty_pos);
    gb_state_read(state,square->frequency);
    gb_state_read(state,square->timer);
}



void gb_apu_wave_clock(gb_apu_wave_t* wave,int timer){

    if(!wave->enabled) return;

    wave->timer -= timer;

    if(wave->timer > 0x00) return;
        
    wave->timer = (0x800 - wave->frequency) << 0x01;

    wave->ram_pos = (wave->ram_pos + 0x01) & 0x1F;

    if(wave->ram_pos & 0x01){
        wave->sample_buffer = wave->ram[wave->ram_pos >> 0x01] & 0x0F;
    }
    else{
        wave->sample_buffer = wave->ram[wave->ram_pos >> 0x01] >> 0x04;
    }

    gb_apu_wave_update_output(wave);
}


void gb_apu_write_wave_register(void* data,uint8_t value,uint16_t address){
    gb_apu_wave_t* wave = (gb_apu_wave_t*)data;

    gb_apu_run(wave->apu);

    switch(address){
        case 0xFF1A:{
            if(!wave->apu->enabled) break;

            wave->dac_enabled = value & 0x80;

            if(!wave->dac_enabled && wave->enabled){
                wave->enabled = false;
            }
            break;
        }
        case 0xFF1B:{
            if(!wave->apu->enabled && wave->apu->gb->is_cgb) break;

            wave->length_counter.counter = 0x100 - value;
            break;
        }
        case 0xFF1C:{
            if(!wave->apu->enabled) break;

            wave->volume_code = (value & 0x60) >> 0x05;
            break;
        }
        case 0xFF1D:{
            if(!wave->apu->enabled) break;

            wave->frequency = (wave->frequency & 0x700) | value;
            break;
        }
        case 0xFF1E:{
            if(!wave->apu->enabled) break;

            bool prev_enabled = wave->enabled;

            wave->frequency = ((value & 0x07) << 0x08) | (wave->frequency & 0xFF);

            if(value & 0x80){
                wave->enabled = wave->dac_enabled;

                if(wave->length_counter.counter == 0x00){
                    wave->length_counter.counter = 0x100;
                    wave->length_counter.enabled = false;
                }

                wave->timer = (0x800 - wave->frequency) << 0x01;

                wave->ram_pos = 0x00;
            }

            gb_apu_length_counter_extra_clock(wave->apu,&wave->length_counter,value,0xFF,&wave->enabled);
            break;
        }
    }
}

uint8_t gb_apu_read_wave_register(void* data,uint16_t address){
    gb_apu_wave_t* wave = (gb_apu_wave_t*)data;

    gb_apu_run(wave->apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF1A: value = (wave->dac_enabled ? 0x80 : 0x00) | 0x7F; break;
        case 0xFF1C: value = 0x9F | ((wave->volume_code & 0x03) << 0x05); break;
        case 0xFF1E: value = 0xBF | (wave->length_counter.enabled ? 0x40 : 0x00); break;
    }

    return value;
}


void gb_apu_write_wave_ram(void* data,uint8_t value,uint16_t address){
    gb_apu_wave_t* wave = (gb_apu_wave_t*)data;
    
    gb_apu_run(wave->apu);

    if(!wave->enabled){
        wave->ram[address & 0x0F] = value;
    }
}

uint8_t gb_apu_read_wave_ram(void* data,uint16_t address){
    gb_apu_wave_t* wave = (gb_apu_wave_t*)data;

    gb_apu_run(wave->apu);

    if(!wave->enabled){
        return wave->ram[address & 0x0F];
    }
    else if(wave->apu->gb->is_cgb){
        return wave->ram[wave->ram_pos >> 0x01];
    }

    return 0xFF;
}


uint8_t gb_apu_wave_raw_output(gb_apu_wave_t* wave){
    if(wave->enabled){
        return wave->sample_buffer >> wave_volume_shift[wave->volume_code];
    }
    return 0x00;
}

void gb_apu_wave_update_output(gb_apu_wave_t* wave){
    if(wave->enabled){

        uint8_t output = wave->sample_buffer >> wave_volume_shift[wave->volume_code];

        wave->output = (7 - output) << gb_audio_channel_volume_shift;
    }
    else{
        wave->output = 0;
    }
}

int gb_apu_wave_output(gb_apu_wave_t* wave){
    return wave->external_enabled ? wave->output : 0;
}


void gb_apu_wave_reset(gb_apu_wave_t* wave,bool hardware){

    wave->enabled = false;
    wave->dac_enabled = false;
    
    wave->length_counter.enabled = false;
    if(hardware || wave->apu->gb->is_cgb){
        wave->length_counter.counter = 0x00;
    }

    wave->volume_code = 0x00;

    wave->frequency = 0x00;
    wave->timer = (0x800 - wave->frequency) << 0x01;
    
    wave->sample_buffer = 0x00;
    wave->ram_pos = 0x00;

    if(hardware){
        if(wave->apu->gb->is_cgb){
            const uint8_t cgb_ram[0x10] = {
                0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF
            };
            memcpy(wave->ram,cgb_ram,sizeof(wave->ram));
        }
        else{
            const uint8_t dmg_ram[0x10] = {
                0x84,0x40,0x43,0xAA,0x2D,0x78,0x92,0x3C,0x60,0x59,0x59,0xB0,0x34,0xB8,0x2E,0xDA
            };
            memcpy(wave->ram,dmg_ram,sizeof(wave->ram));
        }
    }

    wave->output = 0;
}


void gb_apu_wave_save_state(gb_apu_wave_t* wave,gb_state_t* state){
    gb_state_write(state,wave->enabled);
    gb_state_write(state,wave->dac_enabled);
    gb_apu_length_counter_save_state(&wave->length_counter,state);
    gb_state_write(state,wave->volume_code);
    gb_state_write(state,wave->frequency);
    gb_state_write(state,wave->timer);
    gb_state_write(state,wave->sample_buffer);
    gb_state_write(state,wave->ram_pos);
    gb_state_write_ex(state,wave->ram,sizeof(wave->ram));
}

void gb_apu_wave_load_state(gb_apu_wave_t* wave,gb_state_t* state){
    gb_state_read(state,wave->enabled);
    gb_state_read(state,wave->dac_enabled);
    gb_apu_length_counter_load_state(&wave->length_counter,state);
    gb_state_read(state,wave->volume_code);
    gb_state_read(state,wave->frequency);
    gb_state_read(state,wave->timer);
    gb_state_read(state,wave->sample_buffer);
    gb_state_read(state,wave->ram_pos);
    gb_state_read_ex(state,wave->ram,sizeof(wave->ram));
}



void gb_apu_noise_clock(gb_apu_noise_t* noise,int timer){

    if(!noise->enabled) return;

    noise->timer -= timer;

    if(noise->timer > 0x00) return;

    noise->timer = noise_divisor[noise->divisor_code] << noise->clock_shift;

    //Using a noise channel clock shift of 14 or 15 results in the LFSR receiving no clocks.
    if(noise->clock_shift > 0x0D) return;

    bool bit = ((noise->lfsr & 0x02) ? 0x01 : 0x00) ^ ((noise->lfsr & 0x01) ? 0x01 : 0x00);

    noise->lfsr &= ~0x8000;
    noise->lfsr |= bit ? 0x8000 : 0x0000;

    if(noise->width_mode){
        noise->lfsr &= ~0x0080;
        noise->lfsr |= bit ? 0x0080 : 0x0000;
    }

    noise->lfsr >>= 0x01;

    gb_apu_noise_update_output(noise);
}


void gb_apu_write_noise_register(void* data,uint8_t value,uint16_t address){
    gb_apu_noise_t* noise = (gb_apu_noise_t*)data;

    gb_apu_run(noise->apu);

    switch(address){
        case 0xFF20:{
            if(!noise->apu->enabled && noise->apu->gb->is_cgb) break;

            noise->length_counter.counter = 0x40 - (value & 0x3F);
            break;
        }
        case 0xFF21:{
            if(!noise->apu->enabled) break;

            noise->envelope.initial_volume = value >> 0x04;
            noise->envelope.add_mode = value & 0x08;
            noise->envelope.period = value & 0x07;

            if(noise->enabled && !(value & 0xF8)){
                noise->enabled = false;
            }
            break;
        }
        case 0xFF22:{
            if(!noise->apu->enabled) break;

            noise->clock_shift = value >> 0x04;
            noise->width_mode = value & 0x08;
            noise->divisor_code = value & 0x07;
            break;
        }
        case 0xFF23:{
            if(!noise->apu->enabled) break;

            if(value & 0x80){
                noise->enabled = noise->envelope.initial_volume || noise->envelope.add_mode;

                if(noise->length_counter.counter == 0x00){
                    noise->length_counter.counter = 0x40;
                    noise->length_counter.enabled = false;
                }

                noise->timer = noise_divisor[noise->divisor_code] << noise->clock_shift;

                noise->envelope.timer = noise->envelope.period ? noise->envelope.period : 0x08;
                noise->envelope.current_volume = noise->envelope.initial_volume;
                noise->envelope.automatic_change = true;

                noise->lfsr = 0x7FFF;
            }

            gb_apu_length_counter_extra_clock(noise->apu,&noise->length_counter,value,0x3F,&noise->enabled);
            break;
        }
    }
}

uint8_t gb_apu_read_noise_register(void* data,uint16_t address){
    gb_apu_noise_t* noise = (gb_apu_noise_t*)data;

    gb_apu_run(noise->apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF21:
            value = (
                ((noise->envelope.initial_volume & 0x0F) << 0x04) | 
                (noise->envelope.add_mode ? 0x08 : 0x00) | 
                (noise->envelope.period & 0x07)
            );
            break; 
        case 0xFF22:
            value = (
                ((noise->clock_shift & 0x0F) << 0x04) |
                (noise->width_mode ? 0x08 : 0x00) |
                (noise->divisor_code & 0x07)
            );
            break;
        case 0xFF23:
            value = 0xBF | (noise->length_counter.enabled ? 0x40 : 0x00);
            break;
    }

    return value;
}


uint8_t gb_apu_noise_raw_output(gb_apu_noise_t* noise){
    if(noise->enabled){
        return noise->envelope.current_volume * !(noise->lfsr & 0x01);
    }
    return 0x00;
}

void gb_apu_noise_update_output(gb_apu_noise_t* noise){
    if(noise->enabled){
        
        uint8_t output = noise->envelope.current_volume * !(noise->lfsr & 0x01);

        noise->output = (7 - output) << gb_audio_channel_volume_shift; 
    }
    else{
        noise->output = 0;
    }
}

int gb_apu_noise_output(gb_apu_noise_t* noise){
    return noise->external_enabled ? noise->output : 0;
}


void gb_apu_noise_reset(gb_apu_noise_t* noise,bool hardware){
    noise->enabled = false;

    memset(&noise->envelope,0x00,sizeof(noise->envelope));

    noise->length_counter.enabled = false;
    if(hardware || noise->apu->gb->is_cgb){
        noise->length_counter.counter = 0x00;
    }
    
    noise->clock_shift = 0x00;
    noise->width_mode = false;
    noise->divisor_code = 0x00;

    noise->lfsr = 0x00;

    noise->timer = noise_divisor[noise->divisor_code] << noise->clock_shift;

    noise->output = 0;
}


void gb_apu_noise_save_state(gb_apu_noise_t* noise,gb_state_t* state){
    gb_state_write(state,noise->enabled);
    gb_apu_envelope_save_state(&noise->envelope,state);
    gb_apu_length_counter_save_state(&noise->length_counter,state);
    gb_state_write(state,noise->clock_shift);
    gb_state_write(state,noise->width_mode);
    gb_state_write(state,noise->divisor_code);
    gb_state_write(state,noise->lfsr);
    gb_state_write(state,noise->timer);
}

void gb_apu_noise_load_state(gb_apu_noise_t* noise,gb_state_t* state){
    gb_state_read(state,noise->enabled);
    gb_apu_envelope_load_state(&noise->envelope,state);
    gb_apu_length_counter_load_state(&noise->length_counter,state);
    gb_state_read(state,noise->clock_shift);
    gb_state_read(state,noise->width_mode);
    gb_state_read(state,noise->divisor_code);
    gb_state_read(state,noise->lfsr);
    gb_state_read(state,noise->timer);
}


uint8_t gb_apu_read_pcm12_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    gb_t* gb = apu->gb;

    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return 0xFF;

    gb_apu_run(apu);

    return (gb_apu_square_raw_output(&apu->square2) << 0x04) | gb_apu_square_raw_output(&apu->square1);
}

uint8_t gb_apu_read_pcm34_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    gb_t* gb = apu->gb;

    if(!(gb->is_cgb && (gb->cgb_mode || gb->boot.mapped))) return 0xFF;

    gb_apu_run(apu);

    return (gb_apu_noise_raw_output(&apu->noise) << 0x04) | gb_apu_wave_raw_output(&apu->wave);
}


void gb_apu_map_registers(gb_apu_t* apu){
    gb_memory_t* memory = &apu->gb->memory;
    gb_memory_map_in_range(memory,&apu->square1_register_descriptor,0xFF10,0xFF14);
    gb_memory_map_in_range(memory,&apu->square2_register_descriptor,0xFF16,0xFF19);
    gb_memory_map_in_range(memory,&apu->wave_register_descriptor,0xFF1A,0xFF1E);
    gb_memory_map_in_range(memory,&apu->noise_register_descriptor,0xFF20,0xFF23);
    gb_memory_map_in_range(memory,&apu->register_descriptor,0xFF24,0xFF26);
    gb_memory_map_in_range(memory,&apu->wave_ram_descriptor,0xFF30,0xFF3F);
    gb_memory_map(memory,&apu->pcm12_register_descriptor,0xFF76);
    gb_memory_map(memory,&apu->pcm34_register_descriptor,0xFF77);
}


void gb_apu_reset(gb_apu_t* apu,bool hardware){

    if(hardware){

        if(apu->handlers != NULL){
            gb_apu_channel_frame_reset(&apu->square1_frame);
            gb_apu_channel_frame_reset(&apu->square2_frame);
            gb_apu_channel_frame_reset(&apu->wave_frame);
            gb_apu_channel_frame_reset(&apu->noise_frame);
        }

        gb_apu_mixer_frame_reset(&apu->mixer_frame);

        apu->frame_cycles = 0;

        gb_ring_buffer_clear(&apu->ring_buffer);

        apu->skip_first_frame_sequence_event = false;
        
        apu->last_clock_cycle = 0;
        apu->cycles = 0;
    }

    gb_apu_square_reset(&apu->square1,hardware);
    gb_apu_square_reset(&apu->square2,hardware);
    gb_apu_wave_reset(&apu->wave,hardware);
    gb_apu_noise_reset(&apu->noise,hardware);

    apu->square1_panning.left = false;
    apu->square1_panning.right = false;

    apu->square2_panning.left = false;
    apu->square2_panning.right = false;

    apu->wave_panning.left = false;
    apu->wave_panning.right = false;

    apu->noise_panning.left = false;
    apu->noise_panning.right = false;

    apu->vin_panning.left = false;
    apu->vin_panning.right = false;

    apu->volume.left = 0x00;
    apu->volume.right = 0x00;

    apu->enabled = false;

    apu->frame_sequencer = 0x00;
}

void gb_apu_skip_boot(gb_apu_t* apu){

    apu->square1.enabled = true;
    
    apu->square1.envelope.initial_volume = 0x0F;
    apu->square1.envelope.period = 0x03;
    
    apu->square1.length_counter.counter = 0x40;

    apu->square1.duty = 0x02;
    
    apu->square1.frequency = 1985;

    
    apu->square1_panning.left = true;
    apu->square1_panning.right = true;

    apu->square2_panning.left = true;
    apu->square2_panning.right = true;
    
    apu->wave_panning.left = true;
    apu->wave_panning.right = false;

    apu->noise_panning.left = true;
    apu->noise_panning.right = false;

    apu->volume.left = 0x07;
    apu->volume.right = 0x07;

    apu->enabled = true;

    apu->frame_sequencer = 0x01;
}

void gb_apu_free(gb_apu_t* apu){
    blip_delete(apu->square1_frame.blip);
    blip_delete(apu->square2_frame.blip);
    blip_delete(apu->wave_frame.blip);
    blip_delete(apu->noise_frame.blip);

    blip_delete(apu->mixer_frame.blip_left);
    blip_delete(apu->mixer_frame.blip_right);

    gb_ring_buffer_free(&apu->ring_buffer);
}


void gb_apu_save_state(gb_apu_t* apu,gb_state_t* state){
    gb_apu_square_save_state(&apu->square1,state);
    gb_apu_square_save_state(&apu->square2,state);
    gb_apu_wave_save_state(&apu->wave,state);
    gb_apu_noise_save_state(&apu->noise,state);

    gb_state_write(state,apu->square1_panning);
    gb_state_write(state,apu->square2_panning);
    gb_state_write(state,apu->wave_panning);
    gb_state_write(state,apu->noise_panning);
    gb_state_write(state,apu->vin_panning);

    gb_state_write(state,apu->volume);

    gb_state_write(state,apu->enabled);

    gb_state_write(state,apu->frame_sequencer);
    gb_state_write(state,apu->skip_first_frame_sequence_event);

    gb_state_write(state,apu->last_clock_cycle);
    gb_state_write(state,apu->cycles);
}

void gb_apu_load_state(gb_apu_t* apu,gb_state_t* state){

    gb_apu_square_load_state(&apu->square1,state);
    gb_apu_square_load_state(&apu->square2,state);
    gb_apu_wave_load_state(&apu->wave,state);
    gb_apu_noise_load_state(&apu->noise,state);

    gb_state_read(state,apu->square1_panning);
    gb_state_read(state,apu->square2_panning);
    gb_state_read(state,apu->wave_panning);
    gb_state_read(state,apu->noise_panning);
    gb_state_read(state,apu->vin_panning);

    gb_state_read(state,apu->volume);

    gb_state_read(state,apu->enabled);

    gb_state_read(state,apu->frame_sequencer);
    gb_state_read(state,apu->skip_first_frame_sequence_event);

    gb_state_read(state,apu->last_clock_cycle);
    gb_state_read(state,apu->cycles);

    if(apu->handlers != NULL){
        gb_apu_channel_frame_reset(&apu->square1_frame);
        gb_apu_channel_frame_reset(&apu->square2_frame);
        gb_apu_channel_frame_reset(&apu->wave_frame);
        gb_apu_channel_frame_reset(&apu->noise_frame);
    }

    gb_apu_mixer_frame_reset(&apu->mixer_frame);

    apu->frame_cycles = 0;

    gb_ring_buffer_clear(&apu->ring_buffer);
}