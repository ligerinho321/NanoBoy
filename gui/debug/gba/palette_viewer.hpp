#pragma once

#include <gui/utils/utils.hpp>

class gba_palette_viewer_t {
private:
    gba_t* gba = nullptr;

    bool open = false;
public:
    void set_open(bool _open) noexcept {
        open = _open;
    }

    bool get_open() const noexcept {
        return open;
    }
};