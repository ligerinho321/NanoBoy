#include <gui/register_viewer/register_viewer.hpp>

const uint32_t value_color = 0xFFB3FFB3;

void register_viewer_t::init(gb_t* _gb){
    gb = _gb;
}


void register_viewer_t::leaf(uint16_t address,uint8_t value){    
    ImGui::BulletText("%s ($%04X):",gb_registers_label[address & 0xFF],address);
    
    ImGui::SameLine(0.0f,ImGui::GetStyle().ItemInnerSpacing.x);

    ImGui::PushStyleColor(ImGuiCol_Text,value_color);

    ImGui::Text("$%02X",value);

    ImGui::PopStyleColor();
}

void register_viewer_t::leaf(const char* label,const char* fmt,...){
    ImGui::BulletText(label);

    ImGui::SameLine(0.0f,ImGui::GetStyle().ItemInnerSpacing.x);

    ImGui::PushStyleColor(ImGuiCol_Text,value_color);

    va_list args;
    va_start(args,fmt);
    ImGui::TextV(fmt,args);
    va_end(args);

    ImGui::PopStyleColor();
}


bool register_viewer_t::tree(uint16_t address,uint8_t value){
    
    size_t id = address;

    bool result = ImGui::TreeNode((const void*)id,"%s ($%04X):",gb_registers_label[address & 0xFF],address);

    ImGui::SameLine(0.0f,ImGui::GetStyle().ItemInnerSpacing.x);

    ImGui::PushStyleColor(ImGuiCol_Text,value_color);

    ImGui::Text("$%02X",value);

    ImGui::PopStyleColor();

    return result;
}

bool register_viewer_t::tree(const char* label){
    return ImGui::TreeNode(label);
}

void register_viewer_t::tree_end(){
    ImGui::TreePop();
}


void register_viewer_t::joypad(){
    if(!tree("Joypad")) return;

    uint8_t joyp = gb_joypad_peek_register(gb);

    if(tree(0xFF00,joyp)){
        leaf("A/Right (bit0):","%d",bit_active(joyp,0x00));

        leaf("B/Left (bit1):","%d",bit_active(joyp,0x01));
        
        leaf("Select/Up (bit2):","%d",bit_active(joyp,0x02));
        
        leaf("Start/Down (bit3):","%d",bit_active(joyp,0x03));
        
        leaf("Select d-pad (bit4):","%d",bit_active(joyp,0x04));
        
        leaf("Select buttons (bit5):","%d",bit_active(joyp,0x05));
        
        tree_end();
    }

    tree_end();
}

void register_viewer_t::serial(){
    if(!tree("Serial")) return;

    leaf(0xFF01,gb_serial_peek_register(gb,0xFF01));

    uint8_t sc = gb_serial_peek_register(gb,0xFF02);

    if(tree(0xFF02,sc)){
        leaf("Clock select (bit0):","%d",bit_active(sc,0x00));
        
        leaf("Clock speed (bit1):","%d",bit_active(sc,0x01));
        
        leaf("Transfer running (bit7):","%d",bit_active(sc,0x07));

        tree_end();
    }

    tree_end();
}

void register_viewer_t::timer(){
    if(!tree("Timer")) return;

    leaf(0xFF04,gb_timer_peek_register(gb,0xFF04));

    leaf(0xFF05,gb_timer_peek_register(gb,0xFF05));
    
    leaf(0xFF06,gb_timer_peek_register(gb,0xFF06));

    uint8_t tac = gb_timer_peek_register(gb,0xFF07);

    if(tree(0xFF07,tac)){
        leaf("Clock select (bit0.1):","$%02X",tac & 0x03);
        
        leaf("Enabled (bit2):","%d",bit_active(tac,0x02));

        tree_end();
    }

    tree_end();
}

void register_viewer_t::interrupt(){
    if(!tree("Interrupt")) return;

    uint8_t interrupt_flag = gb_interrupt_peek_register(gb,0xFF0F);

    if(tree(0xFF0F,interrupt_flag)){
        leaf("Vblank (bit0):","%d",bit_active(interrupt_flag,0x00));
        
        leaf("LCD (bit1):","%d",bit_active(interrupt_flag,0x01));
        
        leaf("Timer (bit2):","%d",bit_active(interrupt_flag,0x02));
        
        leaf("Serial (bit3):","%d",bit_active(interrupt_flag,0x03));
        
        leaf("Joypad (bit4):","%d",bit_active(interrupt_flag,0x04));

        tree_end();
    }

    uint8_t interrupt_enable = gb_interrupt_peek_register(gb,0xFFFF);

    if(tree(0xFFFF,interrupt_enable)){
        leaf("Vblank (bit0):","%d",bit_active(interrupt_enable,0x00));
        
        leaf("LCD (bit1):","%d",bit_active(interrupt_enable,0x01));
        
        leaf("Timer (bit2):","%d",bit_active(interrupt_enable,0x02));
        
        leaf("Serial (bit3):","%d",bit_active(interrupt_enable,0x03));
        
        leaf("Joypad (bit4):","%d",bit_active(interrupt_enable,0x04));

        tree_end();
    }

    tree_end();
}


void register_viewer_t::square1_channel(){
    if(!tree("Square1 Channel")) return;

    uint8_t nr10 = gb_square1_peek_register(gb,0xFF10);
    
    if(tree(0xFF10,nr10)){
        leaf("Shift (bit0.2):","$%02X",nr10 & 0x07);
        
        leaf("Negate (bit3):","%d",bit_active(nr10,0x03));
        
        leaf("Period (bit4.6):","$%02X",(nr10 & 0x70) >> 0x04);

        tree_end();
    }

    uint8_t nr11 = gb_square1_peek_register(gb,0xFF11);
    
    if(tree(0xFF11,nr11)){
        leaf("Duty (bit6.7):","$%02X",nr11 >> 0x06);

        tree_end();
    }

    uint8_t nr12 = gb_square1_peek_register(gb,0xFF12);
    
    if(tree(0xFF12,nr12)){
        leaf("Period (bit0.2):","$%02X",nr12 & 0x07);
        
        leaf("Add mode (bit3):","%d",bit_active(nr12,0x03));
        
        leaf("Initial volume (bit4.7):","$%02X",nr12 >> 0x04);

        tree_end();
    }

    uint8_t nr14 = gb_square1_peek_register(gb,0xFF14);
    
    if(tree(0xFF14,nr14)){
        leaf("Length enabled (bit6):","%d",bit_active(nr14,0x06));

        tree_end();
    }
    
    tree_end();
}

void register_viewer_t::square2_channel(){
    if(!tree("Square2 Channel")) return;

    uint8_t nr21 = gb_square2_peek_register(gb,0xFF16);

    if(tree(0xFF16,nr21)){
        leaf("Duty (bit6.7):","$%02X",nr21 >> 0x06);

        tree_end();
    }

    uint8_t nr22 = gb_square2_peek_register(gb,0xFF17);

    if(tree(0xFF17,nr22)){
        leaf("Period (bit0.2):","$%02X",nr22 & 0x07);
        
        leaf("Add mode (bit3):,","%d",bit_active(nr22,0x03));
        
        leaf("Initial volume (bit4.7):","$%02X",nr22 >> 0x04);

        tree_end();
    }

    uint8_t nr24 = gb_square2_peek_register(gb,0xFF19);
    
    if(tree(0xFF19,nr24)){
        leaf("Length enabled (bit6):","%d",bit_active(nr24,0x06));

        tree_end();
    }

    tree_end();
}

void register_viewer_t::wave_channel(){
    if(!tree("Wave Channel")) return;

    uint8_t nr30 = gb_wave_peek_register(gb,0xFF1A);

    if(tree(0xFF1A,nr30)){
        leaf("DAC enabled (bit7):","%d",bit_active(nr30,0x07));

        tree_end();
    }

    uint8_t nr32 = gb_wave_peek_register(gb,0xFF1C);

    if(tree(0xFF1C,nr32)){
        leaf("Volume code (bit5.6):","$%02X",(nr32 & 0x60) >> 0x05);

        tree_end();
    }

    uint8_t nr34 = gb_wave_peek_register(gb,0xFF1E);

    if(tree(0xFF1E,nr34)){
        leaf("Length enabled (bit.6):","%d",bit_active(nr34,0x06));

        tree_end();
    }

    tree_end();
}

void register_viewer_t::noise_channel(){
    if(!tree("Noise Channel")) return;

    uint8_t nr41 = gb_noise_peek_register(gb,0xFF21);
    
    if(tree(0xFF21,nr41)){
        
        leaf("Period (bit0.2):","$%02X",nr41 & 0x07);
        
        leaf("Add mode:","%d",bit_active(nr41,0x03));
        
        leaf("Initial volume (bit4.7):","$%02X",nr41 >> 0x04);

        tree_end();
    }

    uint8_t nr43 = gb_noise_peek_register(gb,0xFF22);

    if(tree(0xFF22,nr43)){
        leaf("Divisor code (bit0.2):","$%02X",nr43 & 0x07);
        
        leaf("Width mode (bit3):","%d",bit_active(nr43,0x03));

        leaf("Clock shift (bit4.7):","$%02X",nr43 >> 0x04);
        
        tree_end();
    }

    uint8_t nr44 = gb_noise_peek_register(gb,0xFF23);

    if(tree(0xFF23,nr44)){
        leaf("Length enabled (bit7):","%d",bit_active(nr44,0x06));

        tree_end();
    }

    tree_end();
}

void register_viewer_t::apu_control(){
    if(!tree("APU Control")) return;

    uint8_t nr50 = gb_apu_peek_register(gb,0xFF24);

    if(tree(0xFF24,nr50)){
        leaf("Right volume (bit0.2):","$%02X",nr50 & 0x07);
        
        leaf("VIN right (bit3):","%d",bit_active(nr50,0x03));
        
        leaf("Left volume (bit4.6):","$%02X",(nr50 & 0x70) >> 0x04);
        
        leaf("VIN left (bit7):","%d",bit_active(nr50,0x07));

        tree_end();
    }

    uint8_t nr51 = gb_apu_peek_register(gb,0xFF25);
    
    if(tree(0xFF25,nr51)){    
        leaf("Square1 right (bit0):","%d",bit_active(nr51,0x00));
        
        leaf("Square2 right (bit1):","%d",bit_active(nr51,0x01));
        
        leaf("Wave right (bit2):","%d",bit_active(nr51,0x02));
        
        leaf("Noise right (bit3):","%d",bit_active(nr51,0x03));
        
        leaf("Square1 left (bit4):","%d",bit_active(nr51,0x04));
        
        leaf("Square2 left (bit5):","%d",bit_active(nr51,0x05));
        
        leaf("Wave left (bit6):","%d",bit_active(nr51,0x06));
        
        leaf("Noise left (bit7):","%d",bit_active(nr51,0x07));

        tree_end();
    }

    uint8_t nr52 = gb_apu_peek_register(gb,0xFF26);
    
    if(tree(0xFF26,nr52)){
        leaf("Square1 enabled (bit0):","%d",bit_active(nr52,0x00));
        
        leaf("Square2 enabled (bit1):","%d",bit_active(nr52,0x01));
        
        leaf("Wave enabled (bit2):","%d",bit_active(nr52,0x02));
        
        leaf("Noise enabled (bit3):","%d",bit_active(nr52,0x03));
        
        leaf("APU enabled (bit7):","%d",bit_active(nr52,0x07));

        tree_end();
    }

    tree_end();
}

void register_viewer_t::wave_ram(){
    if(!tree("Wave RAM")) return;

    for(int i = 0; i < 0x10; ++i){

        uint16_t address = 0xFF30 | i;
        
        ImGui::BulletText("$%04X:",address);

        ImGui::SameLine(0.0f,ImGui::GetStyle().ItemInnerSpacing.x);

        ImGui::PushStyleColor(ImGuiCol_Text,value_color);

        ImGui::Text("$%02X",gb_wave_peek_ram(gb,address));

        ImGui::PopStyleColor();
    }

    tree_end();
}

void register_viewer_t::ppu(){
    if(!tree("PPU")) return;

    uint8_t lcdc = gb_ppu_peek_register(gb,0xFF40);
    
    if(tree(0xFF40,lcdc)){
        
        leaf("Background enabled (bit0):","%d",bit_active(lcdc,0x00));
        
        leaf("Object enabled (bit1):","%d",bit_active(lcdc,0x01));

        bool object_size = bit_active(lcdc,0x02);
        leaf("Object size (bit2):","%d (8x%d)",object_size,object_size ? 16 : 8);

        bool bg_tilemap_area = bit_active(lcdc,0x03);
        leaf("Background tile map area (bit3):","%d (%s)",bg_tilemap_area,bg_tilemap_area ? "$9C00" : "$9800");
        
        bool tile_data_area = bit_active(lcdc,0x04);
        leaf("Background/Window tile data area (bit4):","%d (%s)",tile_data_area,tile_data_area ? "$8000" : "$8800");
        
        leaf("Window enabled (bit5):","%d",bit_active(lcdc,0x05));
        
        bool window_tilemap_area = bit_active(lcdc,0x06);
        leaf("Window tile map area (bit6):","%d (%s)",window_tilemap_area,window_tilemap_area ? "$9C00" : "$9800");
        
        leaf("LCDC enabled (bit7):","%d",bit_active(lcdc,0x07));

        tree_end();
    }

    uint8_t stat = gb_ppu_peek_register(gb,0xFF41);

    if(tree(0xFF41,stat)){

        uint8_t mode = stat & 0x03;
        leaf("Mode (bit0.1):","$%02X (%s)",mode,gb_ppu_mode_names[mode]);

        leaf("LYC == LY (bit2):","%d",bit_active(stat,0x02));

        leaf("Mode 0 select (bit3):","%d",bit_active(stat,0x03));

        leaf("Mode 1 select (bit4):","%d",bit_active(stat,0x04));

        leaf("Mode 2 select (bit5):","%d",bit_active(stat,0x05));

        leaf("LYC select (bit6):","%d",bit_active(stat,0x06));

        tree_end();
    }

    leaf(0xFF42,gb_ppu_peek_register(gb,0xFF42));
    
    leaf(0xFF43,gb_ppu_peek_register(gb,0xFF43));
    
    leaf(0xFF44,gb_ppu_peek_register(gb,0xFF44));
    
    leaf(0xFF45,gb_ppu_peek_register(gb,0xFF45));

    leaf(0xFF4A,gb_ppu_peek_register(gb,0xFF4A));

    leaf(0xFF4B,gb_ppu_peek_register(gb,0xFF4B));

    tree_end();
}

void register_viewer_t::palette(){
    if(!tree("Palette")) return;

    leaf(0xFF47,gb_palette_peek_dmg_register(gb,0xFF47));
    
    leaf(0xFF48,gb_palette_peek_dmg_register(gb,0xFF48));

    leaf(0xFF49,gb_palette_peek_dmg_register(gb,0xFF49));

    if(gb->state.is_cgb){
        uint8_t bgpi = gb_palette_peek_cgb_register(gb,0xFF68);

        if(tree(0xFF68,bgpi)){
            leaf("Address (bit0.5):","$%02X",bgpi & 0x1F);
            leaf("Auto increment (bit7):","%d",bit_active(bgpi,0x07));
            tree_end();
        }

        leaf(0xFF69,gb_palette_peek_cgb_register(gb,0xFF69));

        uint8_t obpi = gb_palette_peek_cgb_register(gb,0xFF6A);

        if(tree(0xFF6A,obpi)){
            leaf("Address (bit0.5):","$%02X",obpi & 0x1F);
            leaf("Auto increment (bit7):","%d",bit_active(obpi,0x07));
            tree_end();
        }

        leaf(0xFF6B,gb_palette_peek_cgb_register(gb,0xFF6B));
    }

    tree_end();
}

void register_viewer_t::dma(){
    if(!tree("DMA")) return;

    leaf(0xFF46,gb_oam_dma_peek_register(gb));

    if(gb->state.is_cgb){
        uint8_t hdma5 = gb_vram_dma_peek_register(gb,0xFF55);

        if(tree(0xFF55,hdma5)){
            uint8_t transfer_length = hdma5 & 0x7F;
            leaf("Transfer length (bit0.6):","$%02X ($%02X)",transfer_length,(transfer_length + 0x01) << 0x04);

            leaf("Transfer not running (bit7):","%d",bit_active(hdma5,0x07));

            tree_end();
        }
    }

    tree_end();
}

void register_viewer_t::others(){
    if(!tree("Others")) return;

    if(gb->state.is_cgb){
        uint8_t key0 = gb_peek_key0_register(gb);
        
        if(tree(0xFF4C,key0)){
            leaf("DMG mode (bit2):","%d",bit_active(key0,0x02));

            tree_end();
        }

        uint8_t key1 = gb_peek_key1_register(gb);
        
        if(tree(0xFF4D,key1)){
            leaf("Switch armed (bit0):","%d",bit_active(key1,0x00));
            
            leaf("Double speed (bit7):","%d",bit_active(key1,0x07));

            tree_end();
        }

        uint8_t vbk = gb_ppu_peek_vbk_register(gb);

        if(tree(0xFF4F,vbk)){
            leaf("VRAM bank (bit0):","$%02X",vbk & 0x01);

            tree_end();
        }
    }

    uint8_t bank = gb_boot_peek_bank_register(gb);

    if(tree(0xFF50,bank)){
        leaf("Boot ROM disabled (bit0):","%d",bit_active(bank,0x00));

        tree_end();
    }

    if(gb->state.is_cgb){
        uint8_t rp = gb_infrared_peek_register(gb);

        if(tree(0xFF56,rp)){
            leaf("Emitting (bit0):","%d",bit_active(rp,0x00));
            
            leaf("Receiving (bit1):","%d",bit_active(rp,0x01));

            leaf("Read enable (bit6.7):","$%02X",rp >> 0x06);

            tree_end();
        }


        uint8_t opri = gb_peek_opri_register(gb);

        if(tree(0xFF6C,opri)){
            leaf("Priority mode (bit0):","%d",bit_active(opri,0x00));

            tree_end();
        }


        uint8_t wbk = gb_memory_peek_wbk_register(gb);
        
        if(tree(0xFF70,wbk)){
            leaf("WRAM bank (bit0.2):","$%02X",wbk & 0x07);

            tree_end();
        }


        leaf(0xFF72,gb_peek_undocumented_register(gb,0xFF72));
        
        leaf(0xFF73,gb_peek_undocumented_register(gb,0xFF73));
        
        leaf(0xFF74,gb_peek_undocumented_register(gb,0xFF74));

        leaf(0xFF75,gb_peek_undocumented_register(gb,0xFF75));


        uint8_t pcm12 = gb_pcm12_peek_register(gb);
        
        if(tree(0xFF76,pcm12)){
            leaf("Square1 (bit0.3):","$%02X",pcm12 & 0x0F);

            leaf("Square2 (bit4.7):","$%02X",pcm12 >> 0x04);
            
            tree_end();
        }

        uint8_t pcm34 = gb_pcm34_peek_register(gb);

        if(tree(0xFF77,pcm34)){
            leaf("Wave (bit0.3):","$%02X",pcm34 & 0x0F);
            
            leaf("Noise (bit4.7):","$%02X",pcm34 >> 0x04);

            tree_end();
        }
    }

    tree_end();
}


void register_viewer_t::render(){
    if(!open) return;

    if(!gb->cartridge_inserted){
        open = false;
        return;
    }

    if(ImGui::Begin("Register Viewer",&open)){
        joypad();
        serial();
        timer();
        interrupt();
        square1_channel();
        square2_channel();
        wave_channel();
        noise_channel();
        apu_control();
        wave_ram();
        ppu();
        palette();
        dma();
        others();
    }

    ImGui::End();
}