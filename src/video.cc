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
    , _interpolate()
    , _hliney()
    , _polygonData()
    , _polygon()
{
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

    auto init_pages = [&]() -> void
    {
        int index = 0;
        for(auto& page : _pages) {
            page.id = index;
            ++index;
        }
    };

    auto init_all = [&]() -> void
    {
        init_interpolate();
        init_pages();
    };

    init_all();
}

auto Video::start() -> void
{
    log_debug(SYS_VIDEO, "starting...");
    log_debug(SYS_VIDEO, "started!");
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
        _polygonData.reset();
        _polygon = Polygon();
    };

    log_debug(SYS_VIDEO, "resetting...");
    reset_pages();
    reset_palettes();
    reset_polygon();
    log_debug(SYS_VIDEO, "reset!");
}

auto Video::stop() -> void
{
    log_debug(SYS_VIDEO, "stopping...");
    log_debug(SYS_VIDEO, "stopped!");
}

auto Video::setPalettes(const uint8_t* palettes) -> void
{
    int index = 0;
    for(auto& palette : _palettes) {
        palette.id = index;
        for(auto& color : palette.data) {
            uint16_t rgb = 0;
            rgb = ((rgb << 8) | static_cast<uint16_t>(*palettes++));
            rgb = ((rgb << 8) | static_cast<uint16_t>(*palettes++));
            color.r = ((rgb & 0xf00) >> 4) | ((rgb & 0xf00) >> 8);
            color.g = ((rgb & 0x0f0) >> 0) | ((rgb & 0x0f0) >> 4);
            color.b = ((rgb & 0x00f) << 4) | ((rgb & 0x00f) >> 0);
        }
        ++index;
    }
}

auto Video::selectPalette(uint8_t palette) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "select palette [palette: $%02x]", palette);
#endif
    if(palette < 32) {
        _palette = &_palettes[palette];
    }
}

auto Video::selectPage(uint8_t dst) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "select page [dst: $%02x]", dst);
#endif
    _page0 = getPage(dst);
}

auto Video::fillPage(uint8_t dst, uint8_t col) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "fill page [dst: $%02x, col: $%02x]", dst, col);
#endif
    Page* page = getPage(dst);

    static_cast<void>(::memset(page->data, ((col << 4) | (col << 0)), sizeof(Page::data)));
}

auto Video::copyPage(uint8_t dst, uint8_t src, int16_t vscroll) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "copy page [dst: $%02x, src: $%02x, vscroll: %d]", dst, src, vscroll);
#endif
    uint8_t* dstptr = nullptr;
    uint8_t* srcptr = nullptr;
    uint16_t height = 200;

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
        if((vscroll >= -199) && (vscroll <= +199)) {
            if(vscroll < 0) {
                height -= -vscroll;
                srcptr += (-vscroll * 160);
            }
            else {
                height -= +vscroll;
                dstptr += (+vscroll * 160);
            }
        }
        else {
            height = 0;
        }
    }
    static_cast<void>(::memcpy(dstptr, srcptr, height * 160));
}
auto Video::blitPage(uint8_t src) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "blit page [src: $%02x]", src);
#endif
    if(src != VIDEO_PAGEV) {
        if(src == VIDEO_PAGEI) {
            std::swap(_page1, _page2);
        }
        else {
            _page1 = getPage(src);
        }
    }
    _engine.updateScreen(*_page1, *_palette);
}

auto Video::drawBitmap(const uint8_t* buffer) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "draw bitmap [buffer: %p]", buffer);
#endif
    auto draw_bitmap = [](uint8_t* dst, const uint8_t* src) -> void
    {
        constexpr int total_w = (320 / 8);
        constexpr int total_h = (200 / 1);

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

    return draw_bitmap(_pages[VIDEO_PAGE0].data, buffer);
}

auto Video::drawString(uint16_t id, uint16_t x, uint16_t y, uint8_t color) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "draw string [id: $%04x, x: %d, y: %d, color: $%02x]", id, x, y, color);
#endif
    const char* string = _engine.getString(id);
    if((string != nullptr) && (*string != '\0')) {
        uint16_t cx = x;
        uint16_t cy = y;
        do {
            const char character = *string++;
            if(character != '\n') {
                drawChar(character, cx++, cy, color, _page0->data);
            }
            else {
                cx  = x;
                cy += 8;
                continue;
            }
        } while(*string != '\0');
    }
}

auto Video::drawPolygon(const uint8_t* buffer, uint16_t offset, uint16_t zoom, const Point& point) -> void
{
#ifndef NDEBUG
    log_debug(SYS_VIDEO, "draw polygon [buffer: %p, offset: %u, zoom: %d, x: %d, y: %d]", buffer, offset, zoom, point.x, point.y);
#endif
    _polygonData.reset(buffer, offset);
    readAndDrawPolygon(0xff, zoom, point);
    _polygonData.reset();
}

// ---------------------------------------------------------------------------
// Video
// ---------------------------------------------------------------------------

auto Video::fillPolygon(uint16_t color, uint16_t zoom, const Point &pt) -> void
{
    using DrawLine = auto (Video::*)(int16_t x1, int16_t x2, uint8_t col) -> void;

    auto calcStep = [&](const Point &p1, const Point &p2, uint16_t &dy) -> int32_t
    {
        dy = p2.y - p1.y;
        return (p2.x - p1.x) * _interpolate[dy] * 4;
    };

    if (_polygon.bbw == 0 && _polygon.bbh == 1 && _polygon.count == 4) {
        drawPoint(color, pt.x, pt.y);

        return;
    }

    int16_t x1 = pt.x - _polygon.bbw / 2;
    int16_t x2 = pt.x + _polygon.bbw / 2;
    int16_t y1 = pt.y - _polygon.bbh / 2;
    int16_t y2 = pt.y + _polygon.bbh / 2;

    if (x1 > 319 || x2 < 0 || y1 > 199 || y2 < 0)
        return;

    _hliney = y1;

    uint16_t i, j;
    i = 0;
    j = _polygon.count - 1;

    x2 = _polygon.points[i].x + x1;
    x1 = _polygon.points[j].x + x1;

    ++i;
    --j;

    DrawLine drawFct;
    if (color < 0x10) {
        drawFct = &Video::drawLineN;
    } else if (color > 0x10) {
        drawFct = &Video::drawLineP;
    } else {
        drawFct = &Video::drawLineBlend;
    }

    uint32_t cpt1 = x1 << 16;
    uint32_t cpt2 = x2 << 16;

    while (1) {
        _polygon.count -= 2;
        if (_polygon.count == 0) {
            break;
        }
        uint16_t h;
        int32_t step1 = calcStep(_polygon.points[j + 1], _polygon.points[j], h);
        int32_t step2 = calcStep(_polygon.points[i - 1], _polygon.points[i], h);

        ++i;
        --j;

        cpt1 = (cpt1 & 0xFFFF0000) | 0x7FFF;
        cpt2 = (cpt2 & 0xFFFF0000) | 0x8000;

        if (h == 0) {
            cpt1 += step1;
            cpt2 += step2;
        } else {
            for (; h != 0; --h) {
                if (_hliney >= 0) {
                    x1 = cpt1 >> 16;
                    x2 = cpt2 >> 16;
                    if (x1 <= 319 && x2 >= 0) {
                        if (x1 < 0) x1 = 0;
                        if (x2 > 319) x2 = 319;
                        (this->*drawFct)(x1, x2, color);
                    }
                }
                cpt1 += step1;
                cpt2 += step2;
                ++_hliney;
                if (_hliney > 199) return;
            }
        }
    }
}

auto Video::drawChar(char character, uint16_t x, uint16_t y, uint8_t color, uint8_t *buf) -> void
{
    if (x <= 39 && y <= 192) {

        const uint8_t *ft = Font::data + (character - ' ') * 8;

        uint8_t *p = buf + x * 4 + y * 160;

        for (int j = 0; j < 8; ++j) {
            uint8_t ch = *(ft + j);
            for (int i = 0; i < 4; ++i) {
                uint8_t b = *(p + i);
                uint8_t cmask = 0xff;
                uint8_t colb = 0;
                if (ch & 0x80) {
                    colb |= color << 4;
                    cmask &= 0x0F;
                }
                ch <<= 1;
                if (ch & 0x80) {
                    colb |= color;
                    cmask &= 0xF0;
                }
                ch <<= 1;
                *(p + i) = (b & cmask) | colb;
            }
            p += 160;
        }
    }
}

auto Video::drawPoint(uint8_t color, int16_t x, int16_t y) -> void
{
    if (x >= 0 && x <= 319 && y >= 0 && y <= 199) {
        uint16_t off = y * 160 + x / 2;

        uint8_t cmasko, cmaskn;
        if (x & 1) {
            cmaskn = 0x0F;
            cmasko = 0xF0;
        } else {
            cmaskn = 0xF0;
            cmasko = 0x0F;
        }

        uint8_t colb = (color << 4) | color;
        if (color == 0x10) {
            cmaskn &= 0x88;
            cmasko = ~cmaskn;
            colb = 0x88;
        } else if (color == 0x11) {
            colb = *(_pages[VIDEO_PAGE0].data + off);
        }
        uint8_t b = *(_page0->data + off);
        *(_page0->data + off) = (b & cmasko) | (colb & cmaskn);
    }
}

auto Video::drawLineBlend(int16_t x1, int16_t x2, uint8_t color) -> void
{
    int16_t xmax = std::max(x1, x2);
    int16_t xmin = std::min(x1, x2);
    uint8_t *p = _page0->data + _hliney * 160 + xmin / 2;

    uint16_t w = xmax / 2 - xmin / 2 + 1;
    uint8_t cmaske = 0;
    uint8_t cmasks = 0;
    if (xmin & 1) {
        --w;
        cmasks = 0xF7;
    }
    if (!(xmax & 1)) {
        --w;
        cmaske = 0x7F;
    }

    if (cmasks != 0) {
        *p = (*p & cmasks) | 0x08;
        ++p;
    }
    while (w--) {
        *p = (*p & 0x77) | 0x88;
        ++p;
    }
    if (cmaske != 0) {
        *p = (*p & cmaske) | 0x80;
        ++p;
    }
}

auto Video::drawLineN(int16_t x1, int16_t x2, uint8_t color) -> void
{
    int16_t xmax = std::max(x1, x2);
    int16_t xmin = std::min(x1, x2);
    uint8_t *p = _page0->data + _hliney * 160 + xmin / 2;

    uint16_t w = xmax / 2 - xmin / 2 + 1;
    uint8_t cmaske = 0;
    uint8_t cmasks = 0;
    if (xmin & 1) {
        --w;
        cmasks = 0xF0;
    }
    if (!(xmax & 1)) {
        --w;
        cmaske = 0x0F;
    }

    uint8_t colb = ((color & 0xF) << 4) | (color & 0xF);
    if (cmasks != 0) {
        *p = (*p & cmasks) | (colb & 0x0F);
        ++p;
    }
    while (w--) {
        *p++ = colb;
    }
    if (cmaske != 0) {
        *p = (*p & cmaske) | (colb & 0xF0);
        ++p;
    }
}

auto Video::drawLineP(int16_t x1, int16_t x2, uint8_t color) -> void
{
    int16_t xmax = std::max(x1, x2);
    int16_t xmin = std::min(x1, x2);
    uint16_t off = _hliney * 160 + xmin / 2;
    uint8_t *p = _page0->data + off;
    uint8_t *q = _pages[VIDEO_PAGE0].data + off;

    uint8_t w = xmax / 2 - xmin / 2 + 1;
    uint8_t cmaske = 0;
    uint8_t cmasks = 0;
    if (xmin & 1) {
        --w;
        cmasks = 0xF0;
    }
    if (!(xmax & 1)) {
        --w;
        cmaske = 0x0F;
    }

    if (cmasks != 0) {
        *p = (*p & cmasks) | (*q & 0x0F);
        ++p;
        ++q;
    }
    while (w--) {
        *p++ = *q++;
    }
    if (cmaske != 0) {
        *p = (*p & cmaske) | (*q & 0xF0);
        ++p;
        ++q;
    }
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

/*
 * A shape can be given in two different ways:
 *
 *  - A list of screenspace vertices.
 *  - A list of objectspace vertices, based on a delta from the first vertex.
 *
 *   This is a recursive function.
 */
auto Video::readAndDrawPolygon(uint8_t color, uint16_t zoom, const Point& point) -> void
{
    auto read_polygon = [&]() -> void
    {
        _polygon.bbw   = static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;
        _polygon.bbh   = static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;
        _polygon.count = static_cast<uint8_t>(_polygonData.fetchByte());

        assert(((_polygon.count & 1) == 0) && (_polygon.count < countof(_polygon.points)));

        int       index = -1;
        const int count = _polygon.count;
        for(auto& point : _polygon.points) {
            if(++index >= count) {
                break;
            }
            point.x = static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;
            point.y = static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;
        }
    };

    auto read_polygon_hierarchy = [&]()
    {
        Point ph(point);
        ph.x -= static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;
        ph.y -= static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;

        int children = _polygonData.fetchByte() + 1;

        while(children-- > 0) {
            Point pc(ph);
            uint16_t off = _polygonData.fetchWordBE();
            pc.x += static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;
            pc.y += static_cast<uint16_t>(_polygonData.fetchByte()) * zoom / 64;

            uint16_t color = 0xff;
            uint16_t _bp = off;
            off &= 0x7fff;

            if(_bp & 0x8000) {
                color = *_polygonData.get() & 0x7F;
                _polygonData.advance(2);
            }

            const Ptr polygonData(_polygonData);
            _polygonData.seek(off * 2);
            readAndDrawPolygon(color, zoom, pc);
            _polygonData = polygonData;
        }
    };

    auto draw_polygon = [&]() -> void
    {
        uint8_t i = _polygonData.fetchByte();
        if(i >= 0xc0) { // 0xc0 = 192
            if(color & 0x80) { // 0x80 = 128 (1000 0000)
                color = (i & 0x3f); // 0x3f =  63 (0011 1111)
            }
            read_polygon();
            fillPolygon(color, zoom, point);
        }
        else {
            i &= 0x3f; // 0x3f = 63
            if(i == 1) {
                log_alert("Video::readAndDrawPolygon() ec=0x%X (i != 2)", 0xF80);
            }
            else if(i == 2) {
                read_polygon_hierarchy();
            }
            else {
                log_alert("Video::readAndDrawPolygon() ec=0x%X (i != 2)", 0xFBB);
            }
        }
    };

    return draw_polygon();
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------
