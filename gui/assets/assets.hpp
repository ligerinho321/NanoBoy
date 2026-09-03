#pragma once

#include <gui/utils/utils.hpp>

bool assets_decompress(const uint8_t* src,size_t src_len,uint8_t** dst,size_t* dst_len);

void assets_load_window_icon(SDL_Window* window);