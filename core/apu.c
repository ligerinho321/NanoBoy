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


void gb_channel_frame_end(gb_channel_frame_t* channel_frame){

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

void gb_channel_frame_reset(gb_channel_frame_t* channel_frame){
    blip_clear(channel_frame->blip);
    
    channel_frame->last_output = 0;

    channel_frame->capacitor = 0.0f;
}


void gb_mixer_frame_end(gb_mixer_frame_t* mixer_frame){

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

void gb_mixer_frame_reset(gb_mixer_frame_t* mixer_frame){
    blip_clear(mixer_frame->blip_left);
    blip_clear(mixer_frame->blip_right);
    
    mixer_frame->last_left_output = 0;
    mixer_frame->last_right_output = 0;

    mixer_frame->left_capacitor = 0.0f;
    mixer_frame->right_capacitor = 0.0f;
}


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
        gb_square_write_register,
        gb_square_read_register,
        &apu->square1
    };

    apu->square2_register_descriptor = (gb_memory_descriptor_t){
        gb_square_write_register,
        gb_square_read_register,
        &apu->square2
    };

    apu->wave_register_descriptor = (gb_memory_descriptor_t){
        gb_wave_write_register,
        gb_wave_read_register,
        &apu->wave
    };

    apu->noise_register_descriptor = (gb_memory_descriptor_t){
        gb_noise_write_register,
        gb_noise_read_register,
        &apu->noise
    };

    apu->register_descriptor = (gb_memory_descriptor_t){
        gb_apu_write_register,
        gb_apu_read_register,
        apu
    };

    apu->wave_ram_descriptor = (gb_memory_descriptor_t){
        gb_wave_write_ram,
        gb_wave_read_ram,
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


void gb_apu_add_handler(gb_t* gb,gb_apu_handler_t* handler){
    gb_list_add_element(gb->apu.handlers,handler,gb_apu_handler_t);
}

void gb_apu_remove_handler(gb_t* gb,gb_apu_handler_t* handler){
    gb_apu_t* apu = &gb->apu;

    gb_list_remove_element(apu->handlers,handler,gb_apu_handler_t);

    if(!apu->handlers){
        gb_channel_frame_reset(&apu->square1_frame);
        gb_channel_frame_reset(&apu->square2_frame);
        gb_channel_frame_reset(&apu->wave_frame);
        gb_channel_frame_reset(&apu->noise_frame);
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
    
    uint32_t clock_time = apu->frame_cycles;

    if(clock_time > 0){
        --clock_time;
    }

    int square1_output = gb_square_output(&apu->square1);
    int square2_output = gb_square_output(&apu->square2);
    int wave_output = gb_wave_output(&apu->wave);
    int noise_output = gb_noise_output(&apu->noise);

    gb_apu_state_t* state = &apu->state;

    int left_output = (
        square1_output * state->square1_panning.left +
        square2_output * state->square2_panning.left +
        wave_output * state->wave_panning.left +
        noise_output * state->noise_panning.left
    );
    left_output *= state->volume.left + 0x01;

    int right_output = (
        square1_output * state->square1_panning.right +
        square2_output * state->square2_panning.right +
        wave_output * state->wave_panning.right +
        noise_output * state->noise_panning.right
    );
    right_output *= state->volume.right + 0x01;


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


void gb_apu_frame_end(gb_apu_t* apu){
    if(apu->frame_cycles < gb_frame_cycles) return;

    apu->frame_cycles = 0;

    gb_mixer_frame_end(&apu->mixer_frame);

    if(apu->handlers != NULL){
        
        gb_channel_frame_end(&apu->square1_frame);
        gb_channel_frame_end(&apu->square2_frame);
        gb_channel_frame_end(&apu->wave_frame);
        gb_channel_frame_end(&apu->noise_frame);

        gb_apu_handler_t* handler = apu->handlers;
        do{
            handler->callback(handler->userdata);
            handler = handler->next;
        }while(handler != NULL);
    }

    size_t len = apu->mixer_frame.samples_count * gb_audio_bytes_per_sample;
    
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


void gb_apu_update(gb_apu_t* apu){

    uint64_t cycles_to_run = apu->state.cycle - apu->state.last_clock_cycle;
    apu->state.last_clock_cycle = apu->state.cycle;

    gb_square_state_t* square1_state = &apu->square1.state;
    gb_square_state_t* square2_state = &apu->square2.state;
    gb_wave_state_t* wave_state = &apu->wave.state;
    gb_noise_state_t* noise_state = &apu->noise.state;

    while(cycles_to_run > 0){

        uint64_t cycles = cycles_to_run;

        uint32_t frame_cycles_remaining = gb_frame_cycles - apu->frame_cycles;

        if(frame_cycles_remaining < cycles){
            cycles = frame_cycles_remaining;
        }
        if(square1_state->enabled && square1_state->timer < cycles){
            cycles = square1_state->timer;
        }
        if(square2_state->enabled && square2_state->timer < cycles){
            cycles = square2_state->timer;
        }
        if(wave_state->enabled && wave_state->timer < cycles){
            cycles = wave_state->timer;
        }
        if(noise_state->enabled && noise_state->timer < cycles){
            cycles = noise_state->timer;
        }

        apu->frame_cycles += cycles;

        if(square1_state->enabled){
            gb_square_clock(square1_state,cycles);
        }
        if(square2_state->enabled){
            gb_square_clock(square2_state,cycles);
        }
        if(wave_state->enabled){
            gb_wave_clock(wave_state,cycles);
        }
        if(noise_state->enabled){
            gb_noise_clock(noise_state,cycles);
        }

        gb_apu_update_output(apu);
        gb_apu_frame_end(apu);

        cycles_to_run -= cycles;
    }
}


void gb_apu_frame_sequencer_clock(gb_apu_t* apu){
    gb_apu_state_t* state = &apu->state;

    gb_apu_update(apu);

    if(state->enabled){
        if(!state->skip_first_frame_sequence_event){
            
            if(!(state->frame_sequencer & 0x01)){

                gb_length_counter_clock(
                    &apu->square1.state.length_counter,
                    &apu->square1.state.enabled
                );

                gb_length_counter_clock(
                    &apu->square2.state.length_counter,
                    &apu->square2.state.enabled
                );

                gb_length_counter_clock(
                    &apu->wave.state.length_counter,
                    &apu->wave.state.enabled
                );

                gb_length_counter_clock(
                    &apu->noise.state.length_counter,
                    &apu->noise.state.enabled
                );
            }

            if(state->frame_sequencer == 0x07){
                gb_envelope_clock(&apu->square1.state.envelope);
                
                gb_envelope_clock(&apu->square2.state.envelope);

                gb_envelope_clock(&apu->noise.state.envelope);
            }

            if((state->frame_sequencer & 0x03) == 0x02){
                gb_sweep_clock(&apu->square1);
            }

            gb_apu_update_output(apu);

            state->frame_sequencer = (state->frame_sequencer + 0x01) & 0x07;
        }
        else{
            state->skip_first_frame_sequence_event = false;
        }
    }
}


void gb_apu_write_register(void* data,uint8_t value,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    gb_apu_state_t* state = &apu->state;

    gb_apu_update(apu);

    switch(address){
        case 0xFF24:{
            if(!state->enabled) break;

            state->vin_panning.left = value & 0x80;
            state->vin_panning.right = value & 0x08;
            state->volume.left = (value & 0x70) >> 0x04;
            state->volume.right = value & 0x07;
            break;
        }
        case 0xFF25:{
            if(!state->enabled) break;

            state->square1_panning.left = value & 0x10;
            state->square1_panning.right = value & 0x01;

            state->square2_panning.left = value & 0x20;
            state->square2_panning.right = value & 0x02;

            state->wave_panning.left = value & 0x40;
            state->wave_panning.right = value & 0x04;

            state->noise_panning.left = value & 0x80;
            state->noise_panning.right = value & 0x08;
            break;
        }
        case 0xFF26:{
            bool new_enabled = value & 0x80;
            
            if(!state->enabled && new_enabled){
                state->enabled = true;

                gb_timer_t* timer = &apu->gb->timer;

                gb_timer_update(timer);
                gb_timer_schedule_next_event(timer);
                
                //Segundo o teste div_write_trigger_10 habilitar o canal quando o 4bit (ou 5bit em double speed) 
                //de timer DIV está ativo faz com que o primeiro clock do senquenciador de quadros seja ignorado
                
                state->skip_first_frame_sequence_event = timer->state.div & (apu->gb->state.double_speed ? 0x2000 : 0x1000);
            }
            else if(state->enabled && !new_enabled){
                gb_apu_reset(apu,false);
            }
            break;
        }
    }

    gb_apu_update_output(apu);
}

uint8_t gb_apu_read_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    gb_apu_state_t* state = &apu->state;

    gb_apu_update(apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF24:{
            value = (
                (state->volume.right & 0x07) |
                (state->vin_panning.right ? 0x08 : 0x00) |
                ((state->volume.left & 0x07) << 0x04) |
                (state->vin_panning.left ? 0x80 : 0x00)
            );
            break;
        }
        case 0xFF25:{
            value = (
                (state->square1_panning.right ? 0x01 : 0x00) |
                (state->square2_panning.right ? 0x02 : 0x00) |
                (state->wave_panning.right ? 0x04 : 0x00) |
                (state->noise_panning.right ? 0x08 : 0x00) |
                (state->square1_panning.left ? 0x10 : 0x00) |
                (state->square2_panning.left ? 0x20 : 0x00) | 
                (state->wave_panning.left ? 0x40 : 0x00) |
                (state->noise_panning.left ? 0x80 : 0x00)
            );
            break;
        }
        case 0xFF26:{
            value = (
                (apu->square1.state.enabled ? 0x01 : 0x00) |
                (apu->square2.state.enabled ? 0x02 : 0x00) |
                (apu->wave.state.enabled ? 0x04 : 0x00) |
                (apu->noise.state.enabled ? 0x08 : 0x00) |
                0x70 | 
                (state->enabled ? 0x80 : 0x00)
            );
            break;
        }
    }
    
    return value;
}


static inline uint16_t gb_sweep_get_new_frequency(gb_sweep_t* sweep){
    uint16_t delta = sweep->shadow_frequency >> sweep->shift;
    return sweep->negate ? sweep->shadow_frequency - delta : sweep->shadow_frequency + delta;
}

void gb_sweep_clock(gb_square_t* square){
    
    gb_sweep_t* sweep = &square->state.sweep;

    if(!sweep->enabled || --sweep->timer > 0x00) return;

    if(sweep->period){
        sweep->timer = sweep->period;
    }
    else{
        sweep->timer = 0x08;
        return;
    }

    uint16_t new_frequency = gb_sweep_get_new_frequency(sweep);

    sweep->calc_negate = sweep->negate;
    
    if(new_frequency > 0x7FF){
        square->state.enabled = false;
        return;
    }

    if(sweep->shift > 0x00){
        sweep->shadow_frequency = new_frequency;
        square->state.frequency = new_frequency;

        new_frequency = gb_sweep_get_new_frequency(sweep);

        if(new_frequency > 0x7FF){
            square->state.enabled = false;
        }
    }
}


void gb_envelope_clock(gb_envelope_t* envelope){
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


void gb_length_counter_extra_clock(gb_apu_t* apu,gb_length_counter_t* length_counter,uint8_t value,uint16_t length,bool* channel_enabled){

    bool new_length_counter_enabled = value & 0x40;

    if(((apu->state.frame_sequencer & 0x01) || apu->state.skip_first_frame_sequence_event) && !length_counter->enabled && new_length_counter_enabled){
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

void gb_length_counter_clock(gb_length_counter_t* length_counter,bool* channel_enabled){
    if(!length_counter->enabled) return;

    if(length_counter->counter && --length_counter->counter == 0x00){
        *channel_enabled = false;
    }
}



void gb_square_clock(gb_square_state_t* square_state,int timer){

    square_state->timer -= timer;

    if(square_state->timer > 0x00) return;

    square_state->timer = (0x800 - square_state->frequency) << 0x02;

    square_state->duty_pos = (square_state->duty_pos + 0x01) & 0x07;
}


void gb_square_write_register(void* data,uint8_t value,uint16_t address){
    gb_square_t* square = (gb_square_t*)data;
    gb_square_state_t* state = &square->state;

    gb_apu_update(square->apu);

    switch(address){
        case 0xFF10:{
            if(!square->apu->state.enabled) break;

            state->sweep.period = (value & 0x70) >> 0x04;
            state->sweep.shift = value & 0x07;

            bool new_negate = value & 0x08;

            if(state->sweep.negate && !new_negate && state->sweep.calc_negate){
                state->enabled = false;
            }

            state->sweep.negate = new_negate;
            break;
        }
        case 0xFF11: case 0xFF16:{
            if(square->apu->state.enabled){
                state->duty = value >> 0x06;
            }
            if(square->apu->state.enabled || !square->apu->gb->state.is_cgb){
                state->length_counter.counter = 0x40 - (value & 0x3F);
            }
            break;
        }
        case 0xFF12: case 0xFF17:{
            if(!square->apu->state.enabled) break;

            state->envelope.initial_volume = value >> 0x04;
            state->envelope.add_mode = value & 0x08;
            state->envelope.period = value & 0x07;

            if(state->enabled && !(value & 0xF8)){
                state->enabled = false;
            }
            break;
        }
        case 0xFF13: case 0xFF18:{
            if(!square->apu->state.enabled) break;

            state->frequency = (state->frequency & 0x700) | value;
            break;
        }
        case 0xFF14: case 0xFF19:{
            if(!square->apu->state.enabled) break;

            state->frequency = ((value & 0x07) << 0x08) | (state->frequency & 0xFF);

            if(value & 0x80){
                state->enabled = state->envelope.initial_volume || state->envelope.add_mode;

                if(state->length_counter.counter == 0x00){
                    state->length_counter.counter = 0x40;
                    state->length_counter.enabled = false;
                }

                state->timer = (0x800 - state->frequency) << 0x02;

                state->envelope.timer = state->envelope.period ? state->envelope.period : 0x08;
                state->envelope.current_volume = state->envelope.initial_volume;
                state->envelope.automatic_change = true;

                if(square->has_sweep){
                    state->sweep.shadow_frequency = state->frequency;
                    state->sweep.timer = state->sweep.period ? state->sweep.period : 0x08;
                    state->sweep.enabled = (state->sweep.period != 0x00) | (state->sweep.shift != 0x00);
                    state->sweep.calc_negate = false;

                    if(state->sweep.shift != 0x00){
                        if(gb_sweep_get_new_frequency(&state->sweep) > 0x7FF){
                            state->enabled = false;
                        }
                        state->sweep.calc_negate = state->sweep.negate;
                    }
                }
            }

            gb_length_counter_extra_clock(square->apu,&state->length_counter,value,0x3F,&state->enabled);
            break;
        }
    }

    gb_apu_update_output(square->apu);
}

uint8_t gb_square_read_register(void* data,uint16_t address){
    gb_square_t* square = (gb_square_t*)data;
    gb_square_state_t* state = &square->state;

    gb_apu_update(square->apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF10:
            value = (
                0x80 | 
                ((state->sweep.period & 0x07) << 0x04) | 
                (state->sweep.negate ? 0x08 : 0x00) | 
                (state->sweep.shift & 0x07)
            );
            break;
        case 0xFF11: case 0xFF16:
            value = ((state->duty & 0x03) << 0x06) | 0x3F;
            break;
        case 0xFF12: case 0xFF17:
            value = (
                ((state->envelope.initial_volume & 0x0F) << 0x04) | 
                (state->envelope.add_mode ? 0x08 : 0x00) | 
                (state->envelope.period & 0x07)
            );
            break;
        case 0xFF14: case 0xFF19:
            value = 0xBF | (state->length_counter.enabled ? 0x40 : 0x00);
            break;
    }

    return value;
}


uint8_t gb_square_raw_output(gb_square_state_t* square_state){
    if(square_state->enabled){
        return square_state->envelope.current_volume * square_duty_table[square_state->duty][square_state->duty_pos];
    }
    return 0x00;
}

int gb_square_output(gb_square_t* square){
    
    if(square->external_enabled && square->state.enabled){

        uint8_t output = square->state.envelope.current_volume * square_duty_table[square->state.duty][square->state.duty_pos];
        
        return (7 - output) << gb_audio_channel_volume_shift;
    }

    return 0;
}


void gb_square_reset(gb_square_t* square,bool hardware){
    gb_square_state_t* state = &square->state;

    state->enabled = false;

    if(square->has_sweep){
        memset(&state->sweep,0x00,sizeof(state->sweep));
    }
    
    memset(&state->envelope,0x00,sizeof(state->envelope));

    state->length_counter.enabled = false;
    if(hardware || square->apu->gb->state.is_cgb){
        state->length_counter.counter = 0x00;
    }

    state->duty = 0x00;
    state->duty_pos = 0x00;

    state->frequency = 0x00;
    state->timer = (0x800 - state->frequency) << 0x02;
}



void gb_wave_clock(gb_wave_state_t* wave_state,int timer){

    wave_state->timer -= timer;

    if(wave_state->timer > 0x00) return;
        
    wave_state->timer = (0x800 - wave_state->frequency) << 0x01;

    wave_state->ram_pos = (wave_state->ram_pos + 0x01) & 0x1F;

    if(wave_state->ram_pos & 0x01){
        wave_state->sample_buffer = wave_state->ram[wave_state->ram_pos >> 0x01] & 0x0F;
    }
    else{
        wave_state->sample_buffer = wave_state->ram[wave_state->ram_pos >> 0x01] >> 0x04;
    }
}


void gb_wave_write_register(void* data,uint8_t value,uint16_t address){
    gb_wave_t* wave = (gb_wave_t*)data;
    gb_wave_state_t* state = &wave->state;

    gb_apu_update(wave->apu);

    switch(address){
        case 0xFF1A:{
            if(!wave->apu->state.enabled) break;

            state->dac_enabled = value & 0x80;

            if(!state->dac_enabled && state->enabled){
                state->enabled = false;
            }
            break;
        }
        case 0xFF1B:{
            if(!wave->apu->state.enabled && wave->apu->gb->state.is_cgb) break;

            state->length_counter.counter = 0x100 - value;
            break;
        }
        case 0xFF1C:{
            if(!wave->apu->state.enabled) break;

            state->volume_code = (value & 0x60) >> 0x05;
            break;
        }
        case 0xFF1D:{
            if(!wave->apu->state.enabled) break;

            state->frequency = (state->frequency & 0x700) | value;
            break;
        }
        case 0xFF1E:{
            if(!wave->apu->state.enabled) break;

            state->frequency = ((value & 0x07) << 0x08) | (state->frequency & 0xFF);

            if(value & 0x80){
                state->enabled = state->dac_enabled;

                if(state->length_counter.counter == 0x00){
                    state->length_counter.counter = 0x100;
                    state->length_counter.enabled = false;
                }

                state->timer = (0x800 - state->frequency) << 0x01;

                state->ram_pos = 0x00;
            }

            gb_length_counter_extra_clock(wave->apu,&state->length_counter,value,0xFF,&state->enabled);
            break;
        }
    }

    gb_apu_update_output(wave->apu);
}

uint8_t gb_wave_read_register(void* data,uint16_t address){
    gb_wave_t* wave = (gb_wave_t*)data;
    gb_wave_state_t* state = &wave->state;

    gb_apu_update(wave->apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF1A: value = (state->dac_enabled ? 0x80 : 0x00) | 0x7F; break;
        case 0xFF1C: value = 0x9F | ((state->volume_code & 0x03) << 0x05); break;
        case 0xFF1E: value = 0xBF | (state->length_counter.enabled ? 0x40 : 0x00); break;
    }

    return value;
}


void gb_wave_write_ram(void* data,uint8_t value,uint16_t address){
    gb_wave_t* wave = (gb_wave_t*)data;
    
    gb_apu_update(wave->apu);

    if(!wave->state.enabled){
        wave->state.ram[address & 0x0F] = value;
    }
}

uint8_t gb_wave_read_ram(void* data,uint16_t address){
    gb_wave_t* wave = (gb_wave_t*)data;

    gb_apu_update(wave->apu);

    if(!wave->state.enabled){
        return wave->state.ram[address & 0x0F];
    }
    else if(wave->apu->gb->state.is_cgb){
        return wave->state.ram[wave->state.ram_pos >> 0x01];
    }

    return 0xFF;
}


uint8_t gb_wave_raw_output(gb_wave_state_t* wave_state){
    if(wave_state->enabled){
        return wave_state->sample_buffer >> wave_volume_shift[wave_state->volume_code];
    }
    return 0x00;
}

int gb_wave_output(gb_wave_t* wave){
    
    if(wave->external_enabled && wave->state.enabled){

        uint8_t output = wave->state.sample_buffer >> wave_volume_shift[wave->state.volume_code];

        return (7 - output) << gb_audio_channel_volume_shift;
    }
    
    return 0;
}


void gb_wave_reset(gb_wave_t* wave,bool hardware){
    gb_wave_state_t* state = &wave->state;

    state->enabled = false;
    state->dac_enabled = false;
    
    state->length_counter.enabled = false;
    if(hardware || wave->apu->gb->state.is_cgb){
        state->length_counter.counter = 0x00;
    }

    state->volume_code = 0x00;

    state->frequency = 0x00;
    state->timer = (0x800 - state->frequency) << 0x01;
    
    state->sample_buffer = 0x00;
    state->ram_pos = 0x00;

    if(hardware){
        if(wave->apu->gb->state.is_cgb){
            const uint8_t cgb_ram[0x10] = {
                0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF
            };
            memcpy(state->ram,cgb_ram,sizeof(state->ram));
        }
        else{
            const uint8_t dmg_ram[0x10] = {
                0x84,0x40,0x43,0xAA,0x2D,0x78,0x92,0x3C,0x60,0x59,0x59,0xB0,0x34,0xB8,0x2E,0xDA
            };
            memcpy(state->ram,dmg_ram,sizeof(state->ram));
        }
    }
}



void gb_noise_clock(gb_noise_state_t* noise_state,int timer){

    noise_state->timer -= timer;

    if(noise_state->timer > 0x00) return;

    noise_state->timer = noise_divisor[noise_state->divisor_code] << noise_state->clock_shift;

    //Using a noise channel clock shift of 14 or 15 results in the LFSR receiving no clocks.
    if(noise_state->clock_shift > 0x0D) return;

    bool bit = ((noise_state->lfsr & 0x02) ? 0x01 : 0x00) ^ ((noise_state->lfsr & 0x01) ? 0x01 : 0x00);

    noise_state->lfsr &= ~0x8000;
    noise_state->lfsr |= bit ? 0x8000 : 0x0000;

    if(noise_state->width_mode){
        noise_state->lfsr &= ~0x0080;
        noise_state->lfsr |= bit ? 0x0080 : 0x0000;
    }

    noise_state->lfsr >>= 0x01;
}


void gb_noise_write_register(void* data,uint8_t value,uint16_t address){
    gb_noise_t* noise = (gb_noise_t*)data;
    gb_noise_state_t* state = &noise->state;

    gb_apu_update(noise->apu);

    switch(address){
        case 0xFF20:{
            if(!noise->apu->state.enabled && noise->apu->gb->state.is_cgb) break;

            state->length_counter.counter = 0x40 - (value & 0x3F);
            break;
        }
        case 0xFF21:{
            if(!noise->apu->state.enabled) break;
            
            state->envelope.initial_volume = value >> 0x04;
            state->envelope.add_mode = value & 0x08;
            state->envelope.period = value & 0x07;

            if(state->enabled && !(value & 0xF8)){
                state->enabled = false;
            }
            break;
        }
        case 0xFF22:{
            if(!noise->apu->state.enabled) break;

            state->clock_shift = value >> 0x04;
            state->width_mode = value & 0x08;
            state->divisor_code = value & 0x07;
            break;
        }
        case 0xFF23:{
            if(!noise->apu->state.enabled) break;

            if(value & 0x80){
                state->enabled = state->envelope.initial_volume || state->envelope.add_mode;

                if(state->length_counter.counter == 0x00){
                    state->length_counter.counter = 0x40;
                    state->length_counter.enabled = false;
                }

                state->timer = noise_divisor[state->divisor_code] << state->clock_shift;

                state->envelope.timer = state->envelope.period ? state->envelope.period : 0x08;
                state->envelope.current_volume = state->envelope.initial_volume;
                state->envelope.automatic_change = true;

                state->lfsr = 0x7FFF;
            }

            gb_length_counter_extra_clock(noise->apu,&state->length_counter,value,0x3F,&state->enabled);
            break;
        }
    }

    gb_apu_update_output(noise->apu);
}

uint8_t gb_noise_read_register(void* data,uint16_t address){
    gb_noise_t* noise = (gb_noise_t*)data;
    gb_noise_state_t* state = &noise->state;

    gb_apu_update(noise->apu);

    uint8_t value = 0xFF;

    switch(address){
        case 0xFF21:
            value = (
                ((state->envelope.initial_volume & 0x0F) << 0x04) | 
                (state->envelope.add_mode ? 0x08 : 0x00) | 
                (state->envelope.period & 0x07)
            );
            break; 
        case 0xFF22:
            value = (
                ((state->clock_shift & 0x0F) << 0x04) |
                (state->width_mode ? 0x08 : 0x00) |
                (state->divisor_code & 0x07)
            );
            break;
        case 0xFF23:
            value = 0xBF | (state->length_counter.enabled ? 0x40 : 0x00);
            break;
    }

    return value;
}


uint8_t gb_noise_raw_output(gb_noise_state_t* noise_state){
    if(noise_state->enabled){
        return noise_state->envelope.current_volume * !(noise_state->lfsr & 0x01);
    }
    return 0x00;
}

int gb_noise_output(gb_noise_t* noise){

    if(noise->external_enabled && noise->state.enabled){

        uint8_t output = noise->state.envelope.current_volume * !(noise->state.lfsr & 0x01);

        return (7 - output) << gb_audio_channel_volume_shift; 
    }
    
    return 0;
}


void gb_noise_reset(gb_noise_t* noise,bool hardware){
    gb_noise_state_t* state = &noise->state;

    state->enabled = false;

    memset(&state->envelope,0x00,sizeof(state->envelope));

    state->length_counter.enabled = false;
    if(hardware || noise->apu->gb->state.is_cgb){
        state->length_counter.counter = 0x00;
    }
    
    state->clock_shift = 0x00;
    state->width_mode = false;
    state->divisor_code = 0x00;

    state->lfsr = 0x00;

    state->timer = noise_divisor[state->divisor_code] << state->clock_shift;
}



uint8_t gb_apu_read_pcm12_register(void* data,uint16_t address){
    gb_unused(address);
    
    gb_apu_t* apu = (gb_apu_t*)data;

    gb_apu_update(apu);

    return (gb_square_raw_output(&apu->square2.state) << 0x04) | gb_square_raw_output(&apu->square1.state);
}

uint8_t gb_apu_read_pcm34_register(void* data,uint16_t address){
    gb_unused(address);

    gb_apu_t* apu = (gb_apu_t*)data;

    gb_apu_update(apu);

    return (gb_noise_raw_output(&apu->noise.state) << 0x04) | gb_wave_raw_output(&apu->wave.state);
}


void gb_apu_map_registers(gb_apu_t* apu){
    gb_memory_t* memory = &apu->gb->memory;
    gb_memory_map_in_range(memory,&apu->square1_register_descriptor,0xFF10,0xFF14);
    gb_memory_map_in_range(memory,&apu->square2_register_descriptor,0xFF16,0xFF19);
    gb_memory_map_in_range(memory,&apu->wave_register_descriptor,0xFF1A,0xFF1E);
    gb_memory_map_in_range(memory,&apu->noise_register_descriptor,0xFF20,0xFF23);
    gb_memory_map_in_range(memory,&apu->register_descriptor,0xFF24,0xFF26);
    gb_memory_map_in_range(memory,&apu->wave_ram_descriptor,0xFF30,0xFF3F);
}


void gb_apu_reset(gb_apu_t* apu,bool hardware){

    gb_apu_state_t* state = &apu->state;

    if(hardware){

        apu->frame_cycles = 0;

        if(apu->handlers != NULL){
            gb_channel_frame_reset(&apu->square1_frame);
            gb_channel_frame_reset(&apu->square2_frame);
            gb_channel_frame_reset(&apu->wave_frame);
            gb_channel_frame_reset(&apu->noise_frame);
        }

        gb_mixer_frame_reset(&apu->mixer_frame);

        gb_ring_buffer_clear(&apu->ring_buffer);


        state->skip_first_frame_sequence_event = false;
        
        state->last_clock_cycle = 0;
        state->cycle = 0;
    }

    gb_square_reset(&apu->square1,hardware);
    gb_square_reset(&apu->square2,hardware);
    gb_wave_reset(&apu->wave,hardware);
    gb_noise_reset(&apu->noise,hardware);

    state->square1_panning.left = false;
    state->square1_panning.right = false;

    state->square2_panning.left = false;
    state->square2_panning.right = false;

    state->wave_panning.left = false;
    state->wave_panning.right = false;

    state->noise_panning.left = false;
    state->noise_panning.right = false;

    state->vin_panning.left = false;
    state->vin_panning.right = false;

    state->volume.left = 0x00;
    state->volume.right = 0x00;

    state->enabled = false;

    state->frame_sequencer = 0x00;
}

void gb_apu_skip_boot(gb_apu_t* apu){

    gb_square_state_t* square1_state = &apu->square1.state;

    square1_state->enabled = true;
    
    square1_state->envelope.initial_volume = 0x0F;
    square1_state->envelope.period = 0x03;
    
    square1_state->length_counter.counter = 0x40;

    square1_state->duty = 0x02;
    
    square1_state->frequency = 1985;


    gb_apu_state_t* state = &apu->state;

    state->square1_panning.left = true;
    state->square1_panning.right = true;

    state->square2_panning.left = true;
    state->square2_panning.right = true;
    
    state->wave_panning.left = true;
    state->wave_panning.right = false;

    state->noise_panning.left = true;
    state->noise_panning.right = false;

    state->volume.left = 0x07;
    state->volume.right = 0x07;

    state->enabled = true;

    state->frame_sequencer = 0x01;
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


void gb_apu_save_state(gb_apu_t* apu,gb_snapshot_t* snapshot){
    snapshot->apu = apu->state;
    snapshot->square1 = apu->square1.state;
    snapshot->square2 = apu->square2.state;
    snapshot->wave = apu->wave.state;
    snapshot->noise = apu->noise.state;
}

void gb_apu_load_state(gb_apu_t* apu,gb_snapshot_t* snapshot){
    apu->state = snapshot->apu;
    apu->square1.state = snapshot->square1;
    apu->square2.state = snapshot->square2;
    apu->wave.state = snapshot->wave;
    apu->noise.state = snapshot->noise;

    apu->frame_cycles = 0;

    if(apu->handlers != NULL){
        gb_channel_frame_reset(&apu->square1_frame);
        gb_channel_frame_reset(&apu->square2_frame);
        gb_channel_frame_reset(&apu->wave_frame);
        gb_channel_frame_reset(&apu->noise_frame);
    }

    gb_mixer_frame_reset(&apu->mixer_frame);

    gb_ring_buffer_clear(&apu->ring_buffer);
}
