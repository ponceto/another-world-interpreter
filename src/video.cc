/*
 * video.cc - Copyright (c) 2004-2025
 *
 * Gregory Montoir, Fabien Sanglard, Olivier Poncet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdint>
#include <climits>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "logger.h"
#include "engine.h"
#include "video.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_VIDEO, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// Font
// ---------------------------------------------------------------------------

namespace {

struct Font
{
    static const uint8_t data[96][8];
};

const uint8_t Font::data[96][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* SPC */
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00 }, /* !   */
    { 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* "   */
    { 0x00, 0x24, 0x7e, 0x24, 0x24, 0x7e, 0x24, 0x00 }, /* #   */
    { 0x08, 0x3e, 0x48, 0x3c, 0x12, 0x7c, 0x10, 0x00 }, /* $   */
    { 0x42, 0xa4, 0x48, 0x10, 0x24, 0x4a, 0x84, 0x00 }, /* %   */
    { 0x60, 0x90, 0x90, 0x70, 0x8a, 0x84, 0x7a, 0x00 }, /* &   */
    { 0x08, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* '   */
    { 0x06, 0x08, 0x10, 0x10, 0x10, 0x08, 0x06, 0x00 }, /* (   */
    { 0xc0, 0x20, 0x10, 0x10, 0x10, 0x20, 0xc0, 0x00 }, /* )   */
    { 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00 }, /* *   */
    { 0x00, 0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00 }, /* +   */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20 }, /* ,   */
    { 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00 }, /* -   */
    { 0x00, 0x00, 0x00, 0x00, 0x10, 0x28, 0x10, 0x00 }, /* .   */
    { 0x00, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00 }, /* /   */
    { 0x78, 0x84, 0x8c, 0x94, 0xa4, 0xc4, 0x78, 0x00 }, /* 0   */
    { 0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x7c, 0x00 }, /* 1   */
    { 0x78, 0x84, 0x04, 0x08, 0x30, 0x40, 0xfc, 0x00 }, /* 2   */
    { 0x78, 0x84, 0x04, 0x38, 0x04, 0x84, 0x78, 0x00 }, /* 3   */
    { 0x08, 0x18, 0x28, 0x48, 0xfc, 0x08, 0x08, 0x00 }, /* 4   */
    { 0xfc, 0x80, 0xf8, 0x04, 0x04, 0x84, 0x78, 0x00 }, /* 5   */
    { 0x38, 0x40, 0x80, 0xf8, 0x84, 0x84, 0x78, 0x00 }, /* 6   */
    { 0xfc, 0x04, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00 }, /* 7   */
    { 0x78, 0x84, 0x84, 0x78, 0x84, 0x84, 0x78, 0x00 }, /* 8   */
    { 0x78, 0x84, 0x84, 0x7c, 0x04, 0x08, 0x70, 0x00 }, /* 9   */
    { 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00 }, /* :   */
    { 0x00, 0x00, 0x18, 0x18, 0x00, 0x10, 0x10, 0x60 }, /* ;   */
    { 0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x00 }, /* <   */
    { 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00 }, /* =   */
    { 0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20, 0x00 }, /* >   */
    { 0x7c, 0x82, 0x02, 0x0c, 0x10, 0x00, 0x10, 0x00 }, /* ?   */
    { 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00 }, /* @   */
    { 0x78, 0x84, 0x84, 0xfc, 0x84, 0x84, 0x84, 0x00 }, /* A   */
    { 0xf8, 0x84, 0x84, 0xf8, 0x84, 0x84, 0xf8, 0x00 }, /* B   */
    { 0x78, 0x84, 0x80, 0x80, 0x80, 0x84, 0x78, 0x00 }, /* C   */
    { 0xf8, 0x84, 0x84, 0x84, 0x84, 0x84, 0xf8, 0x00 }, /* D   */
    { 0x7c, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7c, 0x00 }, /* E   */
    { 0xfc, 0x80, 0x80, 0xf0, 0x80, 0x80, 0x80, 0x00 }, /* F   */
    { 0x7c, 0x80, 0x80, 0x8c, 0x84, 0x84, 0x7c, 0x00 }, /* G   */
    { 0x84, 0x84, 0x84, 0xfc, 0x84, 0x84, 0x84, 0x00 }, /* H   */
    { 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00 }, /* I   */
    { 0x04, 0x04, 0x04, 0x04, 0x84, 0x84, 0x78, 0x00 }, /* J   */
    { 0x8c, 0x90, 0xa0, 0xe0, 0x90, 0x88, 0x84, 0x00 }, /* K   */
    { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xfc, 0x00 }, /* L   */
    { 0x82, 0xc6, 0xaa, 0x92, 0x82, 0x82, 0x82, 0x00 }, /* M   */
    { 0x84, 0xc4, 0xa4, 0x94, 0x8c, 0x84, 0x84, 0x00 }, /* N   */
    { 0x78, 0x84, 0x84, 0x84, 0x84, 0x84, 0x78, 0x00 }, /* O   */
    { 0xf8, 0x84, 0x84, 0xf8, 0x80, 0x80, 0x80, 0x00 }, /* P   */
    { 0x78, 0x84, 0x84, 0x84, 0x84, 0x8c, 0x7c, 0x03 }, /* Q   */
    { 0xf8, 0x84, 0x84, 0xf8, 0x90, 0x88, 0x84, 0x00 }, /* R   */
    { 0x78, 0x84, 0x80, 0x78, 0x04, 0x84, 0x78, 0x00 }, /* S   */
    { 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 }, /* T   */
    { 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x78, 0x00 }, /* U   */
    { 0x84, 0x84, 0x84, 0x84, 0x84, 0x48, 0x30, 0x00 }, /* V   */
    { 0x82, 0x82, 0x82, 0x82, 0x92, 0xaa, 0xc6, 0x00 }, /* W   */
    { 0x82, 0x44, 0x28, 0x10, 0x28, 0x44, 0x82, 0x00 }, /* X   */
    { 0x82, 0x44, 0x28, 0x10, 0x10, 0x10, 0x10, 0x00 }, /* Y   */
    { 0xfc, 0x04, 0x08, 0x10, 0x20, 0x40, 0xfc, 0x00 }, /* Z   */
    { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 }, /* [   */
    { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 }, /* \   */
    { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 }, /* ]   */
    { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 }, /* ^   */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe }, /* _   */
    { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 }, /* `   */
    { 0x00, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3c, 0x00 }, /* a   */
    { 0x40, 0x40, 0x78, 0x44, 0x44, 0x44, 0x78, 0x00 }, /* b   */
    { 0x00, 0x00, 0x3c, 0x40, 0x40, 0x40, 0x3c, 0x00 }, /* c   */
    { 0x04, 0x04, 0x3c, 0x44, 0x44, 0x44, 0x3c, 0x00 }, /* d   */
    { 0x00, 0x00, 0x38, 0x44, 0x7c, 0x40, 0x3c, 0x00 }, /* e   */
    { 0x38, 0x44, 0x40, 0x60, 0x40, 0x40, 0x40, 0x00 }, /* f   */
    { 0x00, 0x00, 0x3c, 0x44, 0x44, 0x3c, 0x04, 0x78 }, /* g   */
    { 0x40, 0x40, 0x58, 0x64, 0x44, 0x44, 0x44, 0x00 }, /* h   */
    { 0x10, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 }, /* i   */
    { 0x02, 0x00, 0x02, 0x02, 0x02, 0x02, 0x42, 0x3c }, /* j   */
    { 0x40, 0x40, 0x46, 0x48, 0x70, 0x48, 0x46, 0x00 }, /* k   */
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 }, /* l   */
    { 0x00, 0x00, 0xec, 0x92, 0x92, 0x92, 0x92, 0x00 }, /* m   */
    { 0x00, 0x00, 0x78, 0x44, 0x44, 0x44, 0x44, 0x00 }, /* n   */
    { 0x00, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00 }, /* o   */
    { 0x00, 0x00, 0x78, 0x44, 0x44, 0x78, 0x40, 0x40 }, /* p   */
    { 0x00, 0x00, 0x3c, 0x44, 0x44, 0x3c, 0x04, 0x04 }, /* q   */
    { 0x00, 0x00, 0x4c, 0x70, 0x40, 0x40, 0x40, 0x00 }, /* r   */
    { 0x00, 0x00, 0x3c, 0x40, 0x38, 0x04, 0x78, 0x00 }, /* s   */
    { 0x10, 0x10, 0x3c, 0x10, 0x10, 0x10, 0x0c, 0x00 }, /* t   */
    { 0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x78, 0x00 }, /* u   */
    { 0x00, 0x00, 0x44, 0x44, 0x44, 0x28, 0x10, 0x00 }, /* v   */
    { 0x00, 0x00, 0x82, 0x82, 0x92, 0xaa, 0xc6, 0x00 }, /* w   */
    { 0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00 }, /* x   */
    { 0x00, 0x00, 0x42, 0x22, 0x24, 0x18, 0x08, 0x30 }, /* y   */
    { 0x00, 0x00, 0x7c, 0x08, 0x10, 0x20, 0x7c, 0x00 }, /* z   */
    { 0x60, 0x90, 0x20, 0x40, 0xf0, 0x00, 0x00, 0x00 }, /* {   */
    { 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x00 }, /* |   */
    { 0x38, 0x44, 0xba, 0xa2, 0xba, 0x44, 0x38, 0x00 }, /* }   */
    { 0x38, 0x44, 0x82, 0x82, 0x44, 0x28, 0xee, 0x00 }, /* ~   */
    { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa }, /* DEL */
};

}

// ---------------------------------------------------------------------------
// VGA
// ---------------------------------------------------------------------------

namespace {

struct VGA
{
    static const Color3u8 palette[16];
};

const Color3u8 VGA::palette[] = {
    { 0x00, 0x00, 0x00 }, // dark black
    { 0x00, 0x00, 0x80 }, // dark blue
    { 0x00, 0x80, 0x00 }, // dark green
    { 0x00, 0x80, 0x80 }, // dark cyan
    { 0x80, 0x00, 0x00 }, // dark red
    { 0x80, 0x00, 0x80 }, // dark magenta
    { 0x80, 0x80, 0x00 }, // dark yellow
    { 0x80, 0x80, 0x80 }, // dark white
    { 0xc0, 0xc0, 0xc0 }, // bright black
    { 0x00, 0x00, 0xff }, // bright blue
    { 0x00, 0xff, 0x00 }, // bright green
    { 0x00, 0xff, 0xff }, // bright cyan
    { 0xff, 0x00, 0x00 }, // bright red
    { 0xff, 0x00, 0xff }, // bright magenta
    { 0xff, 0xff, 0x00 }, // bright yellow
    { 0xff, 0xff, 0xff }, // bright white
};

}

// ---------------------------------------------------------------------------
// EGA
// ---------------------------------------------------------------------------

namespace {

struct EGA
{
    static const Color3u8 palette[16];
};

const Color3u8 EGA::palette[] = {
    { 0x00, 0x00, 0x00 }, // dark black
    { 0x00, 0x00, 0xaa }, // dark blue
    { 0x00, 0xaa, 0x00 }, // dark green
    { 0x00, 0xaa, 0xaa }, // dark cyan
    { 0xaa, 0x00, 0x00 }, // dark red
    { 0xaa, 0x00, 0xaa }, // dark magenta
    { 0xaa, 0x55, 0x00 }, // dark yellow
    { 0xaa, 0xaa, 0xaa }, // dark white
    { 0x55, 0x55, 0x55 }, // bright black
    { 0x55, 0x55, 0xff }, // bright blue
    { 0x55, 0xff, 0x55 }, // bright green
    { 0x55, 0xff, 0xff }, // bright cyan
    { 0xff, 0x55, 0x55 }, // bright red
    { 0xff, 0x55, 0xff }, // bright magenta
    { 0xff, 0xff, 0x55 }, // bright yellow
    { 0xff, 0xff, 0xff }, // bright white
};

}

// ---------------------------------------------------------------------------
// CGA
// ---------------------------------------------------------------------------

namespace {

struct CGA
{
    static const Color3u8 palette[16];
};

const Color3u8 CGA::palette[] = {
    { 0x00, 0x00, 0x00 }, // dark black
    { 0x00, 0xaa, 0xaa }, // dark blue
    { 0x00, 0xaa, 0xaa }, // dark green
    { 0x00, 0xaa, 0xaa }, // dark cyan
    { 0xaa, 0x00, 0xaa }, // dark red
    { 0xaa, 0x00, 0xaa }, // dark magenta
    { 0xaa, 0x00, 0xaa }, // dark yellow
    { 0xaa, 0xaa, 0xaa }, // dark white
    { 0x55, 0x55, 0x55 }, // bright black
    { 0x55, 0xff, 0xff }, // bright blue
    { 0x55, 0xff, 0xff }, // bright green
    { 0x55, 0xff, 0xff }, // bright cyan
    { 0xff, 0x55, 0xff }, // bright red
    { 0xff, 0x55, 0xff }, // bright magenta
    { 0xff, 0x55, 0xff }, // bright yellow
    { 0xff, 0xff, 0xff }, // bright white
};

}

// ---------------------------------------------------------------------------
// Video
// ---------------------------------------------------------------------------

Video::Video(Engine& engine)
    : SubSystem(engine, "Video")
    , _pages()
    , _palettes()
    , _page0(nullptr)
    , _page1(nullptr)
    , _page2(nullptr)
    , _palette(nullptr)
    , _polygon()
    , _interpolate()
{
    auto init_pages = [&]() -> void
    {
        int index = 0;
        for(auto& page : _pages) {
            page.id = index;
            ++index;
        }
    };

    auto init_interpolate = [&]() -> void
    {
        int index = 0;
        for(auto& value : _interpolate) {
            if(index == 0) {
                value = 0x4000;
            }
            else {
                value = 0x4000 / index;
            }
            ++index;
        }
    };

    auto init_all = [&]() -> void
    {
        init_pages();
        init_interpolate();
    };

    init_all();
}

auto Video::start() -> void
{
    LOG_DEBUG("starting...");
    LOG_DEBUG("started!");
}

auto Video::reset() -> void
{
    auto reset_pages = [&]() -> void
    {
        int index = 0;
        for(auto& page : _pages) {
            page.id = index;
            for(auto& data : page.data) {
                data = 0;
            }
            ++index;
        }
        _page0 = getPage(VIDEO_PAGE0);
        _page1 = getPage(VIDEO_PAGE1);
        _page2 = getPage(VIDEO_PAGE2);
    };

    auto reset_palettes = [&]() -> void
    {
        int index = 0;
        for(auto& palette : _palettes) {
            palette.id = index;
            for(auto& data : palette.data) {
                data.r = 0;
                data.g = 0;
                data.b = 0;
            }
            ++index;
        }
        _palette = &_palettes[0];
    };

    auto reset_polygon = [&]() -> void
    {
        _polygon = Polygon();
    };

    LOG_DEBUG("resetting...");
    reset_pages();
    reset_palettes();
    reset_polygon();
    LOG_DEBUG("reset!");
}

auto Video::stop() -> void
{
    LOG_DEBUG("stopping...");
    LOG_DEBUG("stopped!");
}

auto Video::setPalettes(const uint8_t* palettes, uint8_t mode) -> void
{
    LOG_DEBUG("set palettes [mode: %d]", mode);

    auto read_rgb_palettes = [&](const uint8_t* palptr) -> void
    {
        int index = 0;
        for(auto& palette : _palettes) {
            palette.id = index;
            for(auto& color : palette.data) {
                uint16_t rgb = 0;
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                color.r = ((rgb & 0xf00) >> 4) | ((rgb & 0xf00) >> 8);
                color.g = ((rgb & 0x0f0) >> 0) | ((rgb & 0x0f0) >> 4);
                color.b = ((rgb & 0x00f) << 4) | ((rgb & 0x00f) >> 0);
            }
            ++index;
        }
    };

    auto read_vga_palettes = [&](const uint8_t* palptr) -> void
    {
        int index = 0;
        for(auto& palette : _palettes) {
            palette.id = index;
            for(auto& color : palette.data) {
                uint16_t rgb = 0;
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                color = VGA::palette[(rgb >> 12) & 0x0f];
            }
            ++index;
        }
    };

    auto read_ega_palettes = [&](const uint8_t* palptr) -> void
    {
        int index = 0;
        for(auto& palette : _palettes) {
            palette.id = index;
            for(auto& color : palette.data) {
                uint16_t rgb = 0;
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                color = EGA::palette[(rgb >> 12) & 0x0f];
            }
            ++index;
        }
    };

    auto read_cga_palettes = [&](const uint8_t* palptr) -> void
    {
        int index = 0;
        for(auto& palette : _palettes) {
            palette.id = index;
            for(auto& color : palette.data) {
                uint16_t rgb = 0;
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                rgb = ((rgb << 8) | static_cast<uint16_t>(*palptr++));
                color = CGA::palette[(rgb >> 12) & 0x0f];
            }
            ++index;
        }
    };

    auto set_palettes = [&]() -> void
    {
        switch(mode) {
            case 0: // RGB main palettes (32 x 16 rgb colors)
                read_rgb_palettes(palettes);
                break;
            case 1: // RGB alt. palettes (32 x 16 rgb colors)
                read_rgb_palettes(palettes + 1024);
                break;
            case 2: // VGA palettes (32 x 16 fixed colors)
                read_vga_palettes(palettes + 1024);
                break;
            case 3: // EGA palettes (32 x 16 fixed colors)
                read_ega_palettes(palettes + 1024);
                break;
            case 4: // CGA palettes (32 x 8 fixed colors)
                read_cga_palettes(palettes + 1024);
                break;
            default:
                break;
        }
    };

    return set_palettes();
}

auto Video::selectPalette(uint8_t palette) -> void
{
    LOG_DEBUG("select palette [palette: $%02x]", palette);

    auto select_palette = [&]() -> void
    {
        if(palette < 32) {
            _palette = &_palettes[palette];
        }
    };

    return select_palette();
}

auto Video::selectPage(uint8_t dst) -> void
{
    LOG_DEBUG("select page [dst: $%02x]", dst);

    auto select_page = [&]() -> void
    {
        _page0 = getPage(dst);
    };

    return select_page();
}

auto Video::fillPage(uint8_t dst, uint8_t col) -> void
{
    LOG_DEBUG("fill page [dst: $%02x, col: $%02x]", dst, col);

    auto fill_page = [&]() -> void
    {
        Page* page = getPage(dst);

        static_cast<void>(::memset(page->data, ((col << 4) | (col << 0)), sizeof(Page::data)));
    };

    return fill_page();
}

auto Video::copyPage(uint8_t dst, uint8_t src, int16_t vscroll) -> void
{
    LOG_DEBUG("copy page [dst: $%02x, src: $%02x, vscroll: %d]", dst, src, vscroll);

    auto copy_page = [&]() -> void
    {
        uint8_t* dstptr = nullptr;
        uint8_t* srcptr = nullptr;
        uint16_t height = PAGE_H;

        if(src == dst) {
            height = 0;
        }
        else if((src == VIDEO_PAGEV) || (src == VIDEO_PAGEI) || !((src &= 0xbf) & 0x80)) {
            Page* dstpage = getPage(dst);
            Page* srcpage = getPage(src);
            dstptr = dstpage->data;
            srcptr = srcpage->data;
        }
        else {
            Page* dstpage = getPage(dst);
            Page* srcpage = getPage(src & 3);
            dstptr = dstpage->data;
            srcptr = srcpage->data;
            if((vscroll >= -(PAGE_H - 1))
            && (vscroll <= +(PAGE_H - 1))) {
                if(vscroll < 0) {
                    height -= -vscroll;
                    srcptr += (-vscroll * BPL);
                }
                else {
                    height -= +vscroll;
                    dstptr += (+vscroll * BPL);
                }
            }
            else {
                height = 0;
            }
        }
        static_cast<void>(::memcpy(dstptr, srcptr, height * BPL));
    };

    return copy_page();
}
auto Video::blitPage(uint8_t src) -> void
{
    LOG_DEBUG("blit page [src: $%02x]", src);

    auto blit_page = [&]() -> void
    {
        if(src != VIDEO_PAGEV) {
            if(src == VIDEO_PAGEI) {
                std::swap(_page1, _page2);
            }
            else {
                _page1 = getPage(src);
            }
        }
        _engine.updateScreen(*_page1, *_palette);
    };

    return blit_page();
}

auto Video::drawBitmap(const uint8_t* buffer) -> void
{
    LOG_DEBUG("draw bitmap [buffer: %p]", buffer);

    auto copy_bitmap = [](uint8_t* dst, const uint8_t* src) -> void
    {
        constexpr int total_w = (PAGE_W / 8);
        constexpr int total_h = (PAGE_H / 1);

        for(int y = total_h; y != 0; --y) {
            for(int x = total_w; x != 0; --x) {
                uint8_t planes[4] = {
                    src[0 * 8000],
                    src[1 * 8000],
                    src[2 * 8000],
                    src[3 * 8000],
                };
                for(int cnt = 4; cnt != 0; --cnt) {
                    uint8_t pixels = 0;
                    for(int bit = 7; bit >= 0; --bit) {
                        uint8_t& plane(planes[bit & 3]);
                        pixels = (pixels << 1) | (plane & 0x80 ? 1 : 0);
                        plane <<= 1;
                    }
                    *dst++ = pixels;
                }
                ++src;
            }
        }
    };

    auto draw_bitmap = [&]() -> void
    {
        copy_bitmap(_pages[VIDEO_PAGE0].data, buffer);
    };

    return draw_bitmap();
}

auto Video::drawString(uint16_t id, uint16_t x, uint16_t y, uint8_t color) -> void
{
    LOG_DEBUG("draw string [id: $%04x, x: %d, y: %d, color: $%02x]", id, x, y, color);

    auto draw_string = [&]() -> void
    {
        renderString(_engine.getString(id), x, y, color);
    };

    return draw_string();
}

auto Video::drawPolygons(const uint8_t* buffer, uint16_t offset, const Point& position, uint16_t zoom) -> void
{
    LOG_DEBUG("draw polygons [buffer: %p, offset: %u, x: %d, y: %d, zoom: %d]", buffer, offset, position.x, position.y, zoom);

    auto draw_polygon = [&]() -> void
    {
        renderPolygons(buffer, offset, position, zoom, 0xff);
    };

    return draw_polygon();
}

auto Video::getPage(uint8_t page) -> Page*
{
    switch(page) {
        case VIDEO_PAGE0:
        case VIDEO_PAGE1:
        case VIDEO_PAGE2:
        case VIDEO_PAGE3:
            return &_pages[page];
        case VIDEO_PAGEV:
            return _page1;
        case VIDEO_PAGEI:
            return _page2;
        default:
            log_error("get video page has failed [page: 0x%02x]", page);
            break;
    }
    return _page0;
}

auto Video::renderString(const char* string, uint16_t x, uint16_t y, uint8_t color) -> void
{
    constexpr char     min_char = 0x20;
    constexpr char     max_char = 0x7f;
    constexpr uint16_t max_xcol = ((40 - 1) * 1);
    constexpr uint16_t max_yrow = ((25 - 1) * 8);

    auto render_char = [](uint8_t* dstbuf, const uint8_t* srcbuf, uint8_t color) -> void
    {
        auto srcptr = srcbuf;
        for(int row = 0; row < 8; ++row, ++srcptr) {
            auto srcval = *srcptr;
            auto dstptr = dstbuf;
            for(int col = 0; col < 4; ++col, ++dstptr) {
                auto dstval = *dstptr;
                uint8_t bits = 0x00;
                uint8_t mask = 0xff;
                /* 1st pixel */ {
                    if(srcval & 0x80) {
                        bits |= (color << 4);
                        mask &= 0x0f;
                    }
                    srcval <<= 1;
                }
                /* 2nd pixel */ {
                    if(srcval & 0x80) {
                        bits |= (color << 0);
                        mask &= 0xf0;
                    }
                    srcval <<= 1;
                }
                /* render the two pixels */ {
                    *dstptr = ((dstval & mask) | bits);
                }
            }
            dstbuf += BPL;
        }
    };

    auto render_character = [&](char character, uint16_t cx, uint16_t cy) -> void
    {
        if((character >= min_char) && (character <= max_char)) {
            auto srcbuf = &Font::data[character - min_char][0];
            if((cx <= max_xcol) && (cy <= max_yrow)) {
                auto dstbuf = &_page0->data[(cy * BPL) + (cx * 4)];
                render_char(dstbuf, srcbuf, color);
            }
        }
    };

    auto render_string = [&]() -> void
    {
        if((string != nullptr) && (*string != '\0')) {
            uint16_t cx = x;
            uint16_t cy = y;
            do {
                const char character = *string;
                if(character != '\n') {
                    render_character(character, cx, cy);
                    cx += 1;
                }
                else {
                    cx  = x;
                    cy += 8;
                }
            } while(*++string != '\0');
        }
    };

    return render_string();
}

auto Video::renderPolygon(const Polygon& polygon, const Point& position, uint16_t zoom, uint16_t color) -> void
{
    auto render_line_plain = +[](Page* dst, Page* src, int16_t x1, int16_t x2, int16_t yl, uint8_t color) -> void
    {
        uint16_t offset = (yl * BPL) + (x1 / PPB);
        uint8_t* dstptr = &dst->data[offset];
        uint16_t width  = (x2 / PPB) - (x1 / PPB) + 1;
        uint8_t  mask1  = 0;
        uint8_t  mask2  = 0;

        if((x1 & 1) != 0) {
            mask1 = 0xf0;
            --width;
        }
        if((x2 & 1) == 0) {
            mask2 = 0x0f;
            --width;
        }

        color = ((color & 0x0f) << 4) | ((color & 0x0f) << 0);
        if(mask1 != 0) {
            *dstptr = (*dstptr & mask1) | (color & 0x0f);
            ++dstptr;
        }
        while(width--) {
            *dstptr = color;
            ++dstptr;
        }
        if(mask2 != 0) {
            *dstptr = (*dstptr & mask2) | (color & 0xf0);
            ++dstptr;
        }
    };

    auto render_line_vcopy = +[](Page* dst, Page* src, int16_t x1, int16_t x2, int16_t yl, uint8_t color) -> void
    {
        uint16_t offset = (yl * BPL) + (x1 / PPB);
        uint8_t* dstptr = &dst->data[offset];
        uint8_t* srcptr = &src->data[offset];
        uint16_t width  = (x2 / PPB) - (x1 / PPB) + 1;
        uint8_t  mask1  = 0;
        uint8_t  mask2  = 0;

        if((x1 & 1) != 0) {
            mask1 = 0xf0;
            --width;
        }
        if((x2 & 1) == 0) {
            mask2 = 0x0f;
            --width;
        }

        if(mask1 != 0) {
            *dstptr = (*dstptr & mask1) | (*srcptr & 0x0f);
            ++dstptr;
            ++srcptr;
        }
        while(width--) {
            *dstptr++ = *srcptr++;
        }
        if(mask2 != 0) {
            *dstptr = (*dstptr & mask2) | (*srcptr & 0xf0);
            ++dstptr;
            ++srcptr;
        }
    };

    auto render_line_blend = +[](Page* dst, Page* src, int16_t x1, int16_t x2, int16_t yl, uint8_t color) -> void
    {
        uint16_t offset = (yl * BPL) + (x1 / PPB);
        uint8_t* dstptr = &dst->data[offset];
        uint16_t width  = (x2 / PPB) - (x1 / PPB) + 1;
        uint8_t  mask1  = 0;
        uint8_t  mask2  = 0;

        if((x1 & 1) != 0) {
            mask1 = 0xf7;
            --width;
        }
        if((x2 & 1) == 0) {
            mask2 = 0x7f;
            --width;
        }

        if(mask1 != 0) {
            *dstptr = (*dstptr & mask1) | 0x08;
            ++dstptr;
        }
        while(width--) {
            *dstptr = (*dstptr | 0x88);
            ++dstptr;
        }
        if(mask2 != 0) {
            *dstptr = (*dstptr & mask2) | 0x80;
            ++dstptr;
        }
    };

    auto get_line_proc = [&]() -> auto
    {
        if(color < 0x10) {
            return render_line_plain;
        }
        if(color > 0x10) {
            return render_line_vcopy;
        }
        return render_line_blend;
    };

    auto render_line = get_line_proc();

    auto draw_point = [&](int16_t xp, int16_t yp) -> void
    {
        if((xp > XMAX) || (xp < XMIN)
        || (yp > YMAX) || (yp < YMIN)) {
            return;
        }
        return render_line(_page0, &_pages[VIDEO_PAGE0], xp, xp, yp, color);
    };

    auto draw_line = [&](int16_t x1, int16_t x2, int16_t yl) -> void
    {
        if(x1 > x2) {
            std::swap(x1, x2);
        }
        if((x1 > XMAX) || (x2 < XMIN)
        || (yl > YMAX) || (yl < YMIN)) {
            return;
        }
        if(x1 < XMIN) {
            x1 = XMIN;
        }
        if(x2 > XMAX) {
            x2 = XMAX;
        }
        return render_line(_page0, &_pages[VIDEO_PAGE0], x1, x2, yl, color);
    };

    auto calc_step = [&](const Point& p1, const Point& p2, int32_t dx, int32_t& dy) -> int32_t
    {
        dx = p2.x - p1.x;
        dy = p2.y - p1.y;

        return dx * _interpolate[dy] * 4;
    };

    auto fill_polygon = [&]() -> void
    {
        auto count = polygon.count;
        const auto full_bbw = polygon.bbw;
        const auto full_bbh = polygon.bbh;
        const auto half_bbw = (full_bbw / 2);
        const auto half_bbh = (full_bbh / 2);
        const int16_t x1 = (position.x - half_bbw);
        const int16_t x2 = (position.x + half_bbw);
        const int16_t y1 = (position.y - half_bbh);
        const int16_t y2 = (position.y + half_bbh);
        /* clip the polygon */ {
            if((x1 > XMAX) || (x2 < XMIN)
            || (y1 > YMAX) || (y2 < YMIN)) {
                return;
            }
        }
        /* the polygon is just a point */ {
            if(count == 4) {
                if(((full_bbw == 1) && (full_bbh <= 1))
                || ((full_bbh == 1) && (full_bbw <= 1))) {
                    draw_point(position.x, position.y);
                    return;
                }
            }
        }
        /* the polygon is a real polygon */ {
            auto p1 = &polygon.points[0];
            auto p2 = &polygon.points[count - 1];
            int32_t xa = static_cast<int32_t>(x1 + p1->x) << 16;
            int32_t xb = static_cast<int32_t>(x1 + p2->x) << 16;
            int32_t yl = y1;
            while(count != 0) {
                int32_t dy = 0;
                const int32_t step1 = calc_step(*p1, *(p1 + 1), 0, dy);
                const int32_t step2 = calc_step(*p2, *(p2 - 1), 0, dy);
                xa = (xa & 0xffff0000) | 0x8000;
                xb = (xb & 0xffff0000) | 0x7fff;
                if(dy != 0) {
                    do {
                        draw_line((xa >> 16), (xb >> 16), yl);
                        xa += step1;
                        xb += step2;
                        ++yl;
                    } while(--dy != 0);
                }
                else {
                    xa += step1;
                    xb += step2;
                }
                ++p1; --count;
                --p2; --count;
            }
        }
    };

    return fill_polygon();
}

auto Video::renderPolygons(const uint8_t* buffer, uint32_t offset, const Point& position, uint16_t zoom, uint8_t color) -> void
{
    Data data(buffer, offset);

    auto read_and_fill_polygon_single = [&]() -> void
    {
        _polygon.bbw   = static_cast<uint16_t>(data.fetchByte()) * zoom / 64;
        _polygon.bbh   = static_cast<uint16_t>(data.fetchByte()) * zoom / 64;
        _polygon.count = static_cast<uint8_t>(data.fetchByte());

        assert(((_polygon.count & 1) == 0) && (_polygon.count < countof(_polygon.points)));

        int       index = -1;
        const int count = _polygon.count;
        for(auto& point : _polygon.points) {
            if(++index >= count) {
                break;
            }
            point.x = static_cast<uint16_t>(data.fetchByte()) * zoom / 64;
            point.y = static_cast<uint16_t>(data.fetchByte()) * zoom / 64;
        }
        renderPolygon(_polygon, position, zoom, color);
    };

    auto read_and_fill_polygon_hierarchy = [&]()
    {
        Point parent_position(position);
        const uint16_t parent_x = static_cast<uint16_t>(data.fetchByte());
        const uint16_t parent_y = static_cast<uint16_t>(data.fetchByte());
        parent_position.x -= (parent_x * zoom) / 64;
        parent_position.y -= (parent_y * zoom) / 64;

        int children = data.fetchByte() + 1;
        do {
            Point child_position(parent_position);
            const uint8_t* child_buffer = buffer;
            uint16_t       child_offset = data.fetchWordBE();
            uint16_t       child_x      = static_cast<uint16_t>(data.fetchByte());
            uint16_t       child_y      = static_cast<uint16_t>(data.fetchByte());
            uint16_t       child_zoom   = zoom;
            uint8_t        child_color  = 0xff;
            child_position.x += (child_x * zoom) / 64;
            child_position.y += (child_y * zoom) / 64;
            if(child_offset & 0x8000) {
                child_color = (data.fetchWordBE() >> 8) & 0x7f;
            }
            child_offset &= 0x7fff;
            child_offset *= 2;
            renderPolygons(child_buffer, child_offset, child_position, child_zoom, child_color);
        } while(--children != 0);
    };

    auto read_and_fill_polygon = [&]() -> void
    {
        const uint8_t type = data.fetchByte();

        if((type & 0xc0) == 0xc0) {
            if(color & 0x80) {
                color = (type & 0x3f);
            }
            read_and_fill_polygon_single();
        }
        else if((type & 0x3f) == 0x02) {
            read_and_fill_polygon_hierarchy();
        }
        else {
            log_alert("bad polygon!");
        }
    };

    return read_and_fill_polygon();
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------
