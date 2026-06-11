#include "apu.h"
#include "gb.h"

void gb_apu_init(gb_apu_t* apu,gb_t* gb){
    apu->gb = gb;

    apu->square1_blip = blip_new(gb_audio_frame_samples);
    apu->square2_blip = blip_new(gb_audio_frame_samples);
    apu->wave_blip = blip_new(gb_audio_frame_samples);
    apu->noise_blip = blip_new(gb_audio_frame_samples);

    blip_set_rates(apu->square1_blip,gb_clock_rate,gb_audio_sample_rate * 0.5f);
    blip_set_rates(apu->square2_blip,gb_clock_rate,gb_audio_sample_rate * 0.5f);
    blip_set_rates(apu->wave_blip,gb_clock_rate,gb_audio_sample_rate * 0.5f);
    blip_set_rates(apu->noise_blip,gb_clock_rate,gb_audio_sample_rate * 0.5f);

    apu->mix_blip_left = blip_new(gb_audio_frame_samples);
    apu->mix_blip_right = blip_new(gb_audio_frame_samples);

    blip_set_rates(apu->mix_blip_left,gb_clock_rate,gb_audio_sample_rate);
    blip_set_rates(apu->mix_blip_right,gb_clock_rate,gb_audio_sample_rate);

    apu->square1.apu = apu;
    apu->square1.has_sweep = true;

    apu->square2.apu = apu;
    apu->square2.has_sweep = false;

    apu->wave.apu = apu;

    apu->noise.apu = apu;

    gb_apu_map_registers(apu);
    
    apu->pcm12_register_handler = (gb_memory_handler_t){
        NULL,
        gb_apu_read_pcm12_register,
        apu
    };

    apu->pcm34_register_handler = (gb_memory_handler_t){
        NULL,
        gb_apu_read_pcm34_register,
        apu
    };
}


void gb_apu_mixer(gb_apu_t* apu){

    int8_t square1_output = gb_apu_square_output(&apu->square1);
    int8_t square2_output = gb_apu_square_output(&apu->square2);
    int8_t wave_output = gb_apu_wave_output(&apu->wave);
    int8_t noise_output = gb_apu_noise_output(&apu->noise);

    /*
    if(square1_output != apu->square1_last_output){
        blip_add_delta(apu->square1_blip,apu->frame_cycle,square1_output - apu->square1_last_output);
        apu->square1_last_output = square1_output;
    }

    if(square2_output != apu->square2_last_output){
        blip_add_delta(apu->square2_blip,apu->frame_cycle,square2_output - apu->square2_last_output);
        apu->square2_last_output = square2_output;
    }

    if(wave_output != apu->wave_last_output){
        blip_add_delta(apu->wave_blip,apu->frame_cycle,wave_output - apu->wave_last_output);
        apu->wave_last_output = wave_output;
    }

    if(noise_output != apu->noise_last_output){
        blip_add_delta(apu->noise_blip,apu->frame_cycle,noise_output - apu->noise_last_output);
        apu->noise_last_output = noise_output;
    }
    */

    int left_output = (
        (apu->square1_panning.left ? square1_output : 0x00) +
        (apu->square2_panning.left ? square2_output : 0x00) +
        (apu->wave_panning.left ? wave_output : 0x00) +
        (apu->noise_panning.left ? noise_output: 0x00)
    );
    left_output *= apu->volume.left + 0x01;
    left_output <<= 0x05;

    if(left_output != apu->mix_last_left_output){
        blip_add_delta(apu->mix_blip_left,apu->frame_cycle,left_output - apu->mix_last_left_output);
        apu->mix_last_left_output = left_output;
    }

    int right_output = (
        (apu->square1_panning.right ? square1_output : 0x00) +
        (apu->square2_panning.right ? square2_output : 0x00) +
        (apu->wave_panning.right ? wave_output : 0x00) +
        (apu->noise_panning.right ? noise_output: 0x00)
    );
    right_output *= apu->volume.right + 0x01;
    right_output <<= 0x05;

    if(right_output != apu->mix_last_right_output){
        blip_add_delta(apu->mix_blip_right,apu->frame_cycle,right_output - apu->mix_last_right_output);
        apu->mix_last_right_output = right_output;
    }


    if(++apu->frame_cycle >= gb_frame_cycles){

        /*
        blip_end_frame(apu->square1_blip,apu->frame_cycle);
        blip_end_frame(apu->square2_blip,apu->frame_cycle);
        blip_end_frame(apu->wave_blip,apu->frame_cycle);
        blip_end_frame(apu->noise_blip,apu->frame_cycle);
        */

        blip_end_frame(apu->mix_blip_left,apu->frame_cycle);
        blip_end_frame(apu->mix_blip_right,apu->frame_cycle);

        int left_samples = blip_samples_avail(apu->mix_blip_left);
        int right_samples = blip_samples_avail(apu->mix_blip_right);

        int samples = left_samples * gb_audio_channels;
        int samples_length = samples * gb_audio_bytes_per_sample;

        while(gb_ring_buffer_writeable(&apu->ring_buffer) < samples_length) gb_sleep(1);

        while(samples > 0){
            int count = gb_min(samples,gb_audio_buffer_samples);
            int channel_count = count >> 1;

            blip_read_samples(apu->mix_blip_left,apu->mix_buffer + 0,channel_count,1);
            blip_read_samples(apu->mix_blip_right,apu->mix_buffer + 1,channel_count,1);

            gb_ring_buffer_write(&apu->ring_buffer,(uint8_t*)apu->mix_buffer,count * gb_audio_bytes_per_sample);

            samples -= count;
        }

        apu->frame_cycle = 0;
    }
}

void gb_apu_clock(gb_apu_t* apu){
    if(apu->enabled){
        gb_apu_square_clock(&apu->square1);
        gb_apu_square_clock(&apu->square2);
        gb_apu_wave_clock(&apu->wave);
        gb_apu_noise_clock(&apu->noise);
    }

    gb_apu_mixer(apu);
}

void gb_apu_frame_sequencer_clock(gb_apu_t* apu){
    
    if(!apu->enabled) return;

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


uint16_t gb_apu_sweep_get_new_frequency(gb_apu_sweep_t* sweep){
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

void gb_apu_length_counter_clock(gb_apu_length_counter_t* length_counter,bool *channel_enabled){
    if(!length_counter->enabled || !length_counter->counter) return;

    if(--length_counter->counter == 0x00){
        *channel_enabled = false;
    }
}

void gb_apu_length_counter_extra_clock(gb_apu_t* apu,gb_apu_length_counter_t* length_counter,uint8_t value,uint16_t length,bool* channel_enabled){

    bool new_length_counter_enabled = value & 0x40;

    if((apu->frame_sequencer & 0x01) && !length_counter->enabled && new_length_counter_enabled){
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


void gb_apu_write_register(void* data,uint8_t value,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;

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


const bool square_duty_table[4][8] = {
    {0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,1,1},
    {0,1,1,1,1,1,1,0}
};

void gb_apu_square_clock(gb_apu_square_t* square){

    if(--square->timer > 0x00) return;

    square->timer = (0x800 - square->frequency) << 0x02;

    square->duty_pos = (square->duty_pos + 0x01) & 0x07;
}

void gb_apu_write_square_register(void* data,uint8_t value,uint16_t address){
    gb_apu_square_t* square = (gb_apu_square_t*)data;

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
            if(square->apu->enabled || square->apu->gb->type == gb_dmg){
                square->length_counter.counter = 0x40 - (value & 0x3F);
            }
            break;
        }
        case 0xFF12: case 0xFF17:{
            if(!square->apu->enabled) break;

            square->envelope.initial_volume = value >> 0x04;
            square->envelope.add_mode = value & 0x08;
            square->envelope.period = value & 0x07;

            if(square->enabled){
                square->enabled = square->envelope.initial_volume || square->envelope.add_mode;
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
    if(square->enabled && square_duty_table[square->duty][square->duty_pos]){
        return square->envelope.current_volume;
    }
    return 0x00;
}

int8_t gb_apu_square_output(gb_apu_square_t* square){
    if(square->enabled){
        uint8_t output = square_duty_table[square->duty][square->duty_pos] ? square->envelope.current_volume : 0x00;
        return 0x07 - output;
    }
    return 0x00;
}

void gb_apu_square_reset(gb_apu_square_t* square,bool hardware){
    square->enabled = false;

    if(square->has_sweep){
        memset(&square->sweep,0x00,sizeof(square->sweep));
    }
    
    memset(&square->envelope,0x00,sizeof(square->envelope));

    square->length_counter.enabled = false;
    if(hardware || square->apu->gb->type == gb_cgb){
        square->length_counter.counter = 0x00;
    }

    square->duty = 0x00;
    square->duty_pos = 0x00;

    square->frequency = 0x00;
    square->timer = (0x800 - square->frequency) << 0x02;
}


const uint8_t wave_volume_shift[4] = {
    4,0,1,2
};

void gb_apu_wave_clock(gb_apu_wave_t* wave){

    if(--wave->timer > 0x00) return;
        
    wave->timer = (0x800 - wave->frequency) << 0x01;

    wave->ram_pos = (wave->ram_pos + 0x01) & 0x1F;

    if(wave->ram_pos & 0x01){
        wave->sample_buffer = wave->ram[wave->ram_pos >> 0x01] & 0x0F;
    }
    else{
        wave->sample_buffer = wave->ram[wave->ram_pos >> 0x01] >> 0x04;
    }
}

void gb_apu_write_wave_register(void* data,uint8_t value,uint16_t address){
    gb_apu_wave_t* wave = (gb_apu_wave_t*)data;

    switch(address){
        case 0xFF1A:{
            if(!wave->apu->enabled) break;

            wave->dac_enabled = value & 0x80;

            if(wave->enabled){
                wave->enabled = wave->dac_enabled;
            }
            break;
        }
        case 0xFF1B:{
            if(!wave->apu->enabled && wave->apu->gb->type != gb_dmg) break;

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
    if(!wave->enabled){
        wave->ram[address & 0x0F] = value;
    }
}

uint8_t gb_apu_read_wave_ram(void* data,uint16_t address){
    gb_apu_wave_t* wave = (gb_apu_wave_t*)data;
    if(!wave->enabled){
        return wave->ram[address & 0x0F];
    }
    return 0xFF;
}

uint8_t gb_apu_wave_raw_output(gb_apu_wave_t* wave){
    if(wave->enabled){
        return wave->sample_buffer >> wave_volume_shift[wave->volume_code];
    }
    return 0x00;
}

int8_t gb_apu_wave_output(gb_apu_wave_t* wave){
    if(wave->enabled){
        uint8_t output = wave->sample_buffer >> wave_volume_shift[wave->volume_code];
        return 0x07 - output;
    }
    return 0x00;
}

void gb_apu_wave_reset(gb_apu_wave_t* wave,bool hardware){

    wave->enabled = false;
    wave->dac_enabled = false;
    
    wave->length_counter.enabled = false;
    if(hardware || wave->apu->gb->type == gb_cgb){
        wave->length_counter.counter = 0x00;
    }

    wave->volume_code = 0x00;

    wave->frequency = 0x00;
    wave->timer = (0x800 - wave->frequency) << 0x01;
    
    wave->sample_buffer = 0x00;
    wave->ram_pos = 0x00;

    if(hardware){
        if(wave->apu->gb->type == gb_cgb){
            uint8_t cgb_ram[0x10] = {
                0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF
            };
            memcpy(wave->ram,cgb_ram,sizeof(wave->ram));
        }
        else{
            uint8_t dmg_ram[0x10] = {
                0x84,0x40,0x43,0xAA,0x2D,0x78,0x92,0x3C,0x60,0x59,0x59,0xB0,0x34,0xB8,0x2E,0xDA
            };
            memcpy(wave->ram,dmg_ram,sizeof(wave->ram));
        }
    }
}


const uint8_t noise_divisor[8] = {
    8,16,32,48,64,80,96,112
};

void gb_apu_noise_clock(gb_apu_noise_t* noise){

    if(--noise->timer > 0x00) return;

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
}

void gb_apu_write_noise_register(void* data,uint8_t value,uint16_t address){
    gb_apu_noise_t* noise = (gb_apu_noise_t*)data;

    switch(address){
        case 0xFF20:{
            if(!noise->apu->enabled && noise->apu->gb->type != gb_dmg) break;

            noise->length_counter.counter = 0x40 - (value & 0x3F);
            break;
        }
        case 0xFF21:{
            if(!noise->apu->enabled) break;

            noise->envelope.initial_volume = value >> 0x04;
            noise->envelope.add_mode = value & 0x08;
            noise->envelope.period = value & 0x07;

            if(noise->enabled){
                noise->enabled = noise->envelope.initial_volume || noise->envelope.add_mode;
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
    if(noise->enabled && !(noise->lfsr & 0x01)){
        return noise->envelope.current_volume;
    }
    return 0x00;
}

int8_t gb_apu_noise_output(gb_apu_noise_t* noise){
    if(noise->enabled){
        uint8_t output = !(noise->lfsr & 0x01) ? noise->envelope.current_volume : 0x00;
        return 0x07 - output;
    }
    return 0x00;
}

void gb_apu_noise_reset(gb_apu_noise_t* noise,bool hardware){
    noise->enabled = false;

    memset(&noise->envelope,0x00,sizeof(noise->envelope));

    noise->length_counter.enabled = false;
    if(hardware || noise->apu->gb->type == gb_cgb){
        noise->length_counter.counter = 0x00;
    }
    
    noise->clock_shift = 0x00;
    noise->width_mode = false;
    noise->divisor_code = 0x00;

    noise->lfsr = 0x00;

    noise->timer = noise_divisor[noise->divisor_code] << noise->clock_shift;
}


uint8_t gb_apu_read_pcm12_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    return (gb_apu_square_raw_output(&apu->square2) << 0x04) | gb_apu_square_raw_output(&apu->square1);
}

uint8_t gb_apu_read_pcm34_register(void* data,uint16_t address){
    gb_apu_t* apu = (gb_apu_t*)data;
    return (gb_apu_noise_raw_output(&apu->noise) << 0x04) | gb_apu_wave_raw_output(&apu->wave);
}


void gb_apu_map_registers(gb_apu_t* apu){

    apu->square1_register_handler = (gb_memory_handler_t){
        gb_apu_write_square_register,
        gb_apu_read_square_register,
        &apu->square1
    };

    apu->square2_register_handler = (gb_memory_handler_t){
        gb_apu_write_square_register,
        gb_apu_read_square_register,
        &apu->square2
    };

    apu->wave_register_handler = (gb_memory_handler_t){
        gb_apu_write_wave_register,
        gb_apu_read_wave_register,
        &apu->wave
    };

    apu->noise_register_handler = (gb_memory_handler_t){
        gb_apu_write_noise_register,
        gb_apu_read_noise_register,
        &apu->noise
    };

    apu->register_handler = (gb_memory_handler_t){
        gb_apu_write_register,
        gb_apu_read_register,
        apu
    };

    apu->wave_ram_handler = (gb_memory_handler_t){
        gb_apu_write_wave_ram,
        gb_apu_read_wave_ram,
        &apu->wave
    };

    gb_memory_map(&apu->gb->memory,&apu->square1_register_handler,0xFF10,0xFF14);
    gb_memory_map(&apu->gb->memory,&apu->square2_register_handler,0xFF16,0xFF19);
    gb_memory_map(&apu->gb->memory,&apu->wave_register_handler,0xFF1A,0xFF1E);
    gb_memory_map(&apu->gb->memory,&apu->noise_register_handler,0xFF20,0xFF23);
    gb_memory_map(&apu->gb->memory,&apu->register_handler,0xFF24,0xFF26);
    gb_memory_map(&apu->gb->memory,&apu->wave_ram_handler,0xFF30,0xFF3F);
}


void gb_apu_map_pcm_registers(gb_apu_t* apu){
    gb_memory_handler_t** bus = apu->gb->memory.bus;
    bus[0xFF76] = &apu->pcm12_register_handler;
    bus[0xFF77] = &apu->pcm34_register_handler;
}

void gb_apu_unmap_pcm_registers(gb_apu_t* apu){
    gb_memory_handler_t** bus = apu->gb->memory.bus;
    bus[0xFF76] = NULL;
    bus[0xFF77] = NULL;
}


void gb_apu_reset(gb_apu_t* apu,bool hardware){

    if(hardware){

        blip_clear(apu->square1_blip);
        blip_clear(apu->square2_blip);
        blip_clear(apu->wave_blip);
        blip_clear(apu->noise_blip);

        apu->square1_last_output = 0;
        apu->square2_last_output = 0;
        apu->wave_last_output = 0;
        apu->noise_last_output = 0;

        blip_clear(apu->mix_blip_left);
        blip_clear(apu->mix_blip_right);

        apu->mix_last_left_output = 0;
        apu->mix_last_right_output = 0;

        apu->frame_cycle = 0;

        gb_ring_buffer_clear(&apu->ring_buffer);
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


void gb_apu_free(gb_apu_t* apu){
    blip_delete(apu->square1_blip);
    blip_delete(apu->square2_blip);
    blip_delete(apu->wave_blip);
    blip_delete(apu->noise_blip);

    blip_delete(apu->mix_blip_left);
    blip_delete(apu->mix_blip_right);
}