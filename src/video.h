/*
 * video.h - Copyright (c) 2004-2025
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
#ifndef __AW_VIDEO_H__
#define __AW_VIDEO_H__

#include "intern.h"

// ---------------------------------------------------------------------------
// Video
// ---------------------------------------------------------------------------

class Video final
    : public SubSystem
{
public: // public interface
    Video(Engine& engine);

    Video(Video&&) = delete;

    Video(const Video&) = delete;

    Video& operator=(Video&&) = delete;

    Video& operator=(const Video&) = delete;

    virtual ~Video() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public video interface
    auto setPalettes(const uint8_t* palettes, uint8_t mode) -> void;

    auto selectPalette(uint8_t palette) -> void;

    auto selectPage(uint8_t dst) -> void;

    auto fillPage(uint8_t dst, uint8_t col) -> void;

    auto copyPage(uint8_t dst, uint8_t src, int16_t vscroll) -> void;

    auto blitPage(uint8_t src) -> void;

    auto drawBitmap(const uint8_t* buffer) -> void;

    auto drawString(uint16_t id, uint16_t x, uint16_t y, uint8_t color) -> void;

    auto drawPolygons(const uint8_t* buffer, uint16_t offset, const Point& position, uint16_t zoom) -> void;

private: // private interface
    static constexpr uint8_t VIDEO_PAGE0 = 0x00;
    static constexpr uint8_t VIDEO_PAGE1 = 0x01;
    static constexpr uint8_t VIDEO_PAGE2 = 0x02;
    static constexpr uint8_t VIDEO_PAGE3 = 0x03;
    static constexpr uint8_t VIDEO_PAGEV = 0xfe;
    static constexpr uint8_t VIDEO_PAGEI = 0xff;

    static constexpr int16_t PAGE_W = 320; // page width
    static constexpr int16_t PAGE_H = 200; // page height
    static constexpr int16_t XMIN   =   0; // x-min
    static constexpr int16_t YMIN   =   0; // y-min
    static constexpr int16_t XMAX   = 319; // x-max
    static constexpr int16_t YMAX   = 199; // y-max
    static constexpr int16_t BPP    =   4; // bits per pixel
    static constexpr int16_t PPB    =   2; // pixels per byte
    static constexpr int16_t BPL    = 160; // bytes per line

    auto getPage(uint8_t page) -> Page*;

    auto renderString(const char* string, uint16_t x, uint16_t y, uint8_t color) -> void;

    auto renderPolygon(const Polygon& polygon, const Point& position, uint16_t zoom, uint16_t color) -> void;

    auto renderPolygons(const uint8_t* buffer, uint32_t offset, const Point& position, uint16_t zoom, uint8_t color) -> void;

private: // private data
    Page     _pages[4];
    Palette  _palettes[32];
    Page*    _page0;
    Page*    _page1;
    Page*    _page2;
    Palette* _palette;
    Polygon  _polygon;
    uint16_t _interpolate[0x400];
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_VIDEO_H__ */
