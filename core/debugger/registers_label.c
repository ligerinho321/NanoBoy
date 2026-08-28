
const char* gb_registers_label[0x100] = {
    [0x00] = "JOYP",
    
    [0x01] = "SB",
    [0x02] = "SC",

    [0x04] = "DIV",
    [0x05] = "TIMA",
    [0x06] = "TMA",
    [0x07] = "TAC",

    [0x0F] = "IF",
    
    [0x10] = "NR10",
    [0x11] = "NR11",
    [0x12] = "NR12",
    [0x13] = "NR13",
    [0x14] = "NR14",

    [0x16] = "NR21",
    [0x17] = "NR22",
    [0x18] = "NR23",
    [0x19] = "NR24",

    [0x1A] = "NR30",
    [0x1B] = "NR31",
    [0x1C] = "NR32",
    [0x1D] = "NR33",
    [0x1E] = "NR34",

    [0x20] = "NR41",
    [0x21] = "NR42",
    [0x22] = "NR43",
    [0x23] = "NR44",

    [0x24] = "NR50",
    [0x25] = "NR51",
    [0x26] = "NR52",
    
    [0x30] = "Wave RAM",
    [0x31] = "Wave RAM",
    [0x32] = "Wave RAM",
    [0x33] = "Wave RAM",
    [0x34] = "Wave RAM",
    [0x35] = "Wave RAM",
    [0x36] = "Wave RAM",
    [0x37] = "Wave RAM",
    [0x38] = "Wave RAM",
    [0x39] = "Wave RAM",
    [0x3A] = "Wave RAM",
    [0x3B] = "Wave RAM",
    [0x3C] = "Wave RAM",
    [0x3D] = "Wave RAM",
    [0x3E] = "Wave RAM",
    [0x3F] = "Wave RAM",

    [0x40] = "LCDC",
    [0x41] = "STAT",
    [0x42] = "SCY",
    [0x43] = "SCX",
    [0x44] = "LY",
    [0x45] = "LYC",
    [0x46] = "DMA",
    [0x47] = "BGP",
    [0x48] = "OBP0",
    [0x49] = "OBP1",
    [0x4A] = "WY",
    [0x4B] = "WX",
    [0x4C] = "KEY0",
    [0x4D] = "KEY1",

    [0x4F] = "VBK",
    [0x50] = "BANK",
    [0x51] = "HDMA1",
    [0x52] = "HDMA2",
    [0x53] = "HDMA3",
    [0x54] = "HDMA4",
    [0x55] = "HDMA5",
    [0x56] = "RP",

    [0x68] = "BGPI",
    [0x69] = "BGPD",
    [0x6A] = "OBPI",
    [0x6B] = "OBPD",
    [0x6C] = "OPRI",

    [0x70] = "WBK",

    [0x76] = "PCM12",
    [0x77] = "PCM34",

    [0xFF] = "IE"
};