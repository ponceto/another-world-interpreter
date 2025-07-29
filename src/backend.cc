/*
 * backend.cc - Copyright (c) 2004-2025
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
#include <SDL2/SDL.h>
#include "logger.h"
#include "engine.h"
#include "backend.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_BACKEND, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// some aliases
// ---------------------------------------------------------------------------

using WindowType                  = SDL_Window;
using RendererType                = SDL_Renderer;
using SurfaceType                 = SDL_Surface;
using TextureType                 = SDL_Texture;
using ColorType                   = SDL_Color;
using AudioSpecType               = SDL_AudioSpec;
using EventType                   = SDL_Event;
using CommonEventType             = SDL_CommonEvent;
using DisplayEventType            = SDL_DisplayEvent;
using WindowEventType             = SDL_WindowEvent;
using KeyboardEventType           = SDL_KeyboardEvent;
using TextEditingEventType        = SDL_TextEditingEvent;
using TextEditingExtEventType     = SDL_TextEditingExtEvent;
using TextInputEventType          = SDL_TextInputEvent;
using MouseMotionEventType        = SDL_MouseMotionEvent;
using MouseButtonEventType        = SDL_MouseButtonEvent;
using MouseWheelEventType         = SDL_MouseWheelEvent;
using JoyAxisEventType            = SDL_JoyAxisEvent;
using JoyBallEventType            = SDL_JoyBallEvent;
using JoyHatEventType             = SDL_JoyHatEvent;
using JoyButtonEventType          = SDL_JoyButtonEvent;
using JoyDeviceEventType          = SDL_JoyDeviceEvent;
using JoyBatteryEventType         = SDL_JoyBatteryEvent;
using ControllerAxisEventType     = SDL_ControllerAxisEvent;
using ControllerButtonEventType   = SDL_ControllerButtonEvent;
using ControllerDeviceEventType   = SDL_ControllerDeviceEvent;
using ControllerTouchpadEventType = SDL_ControllerTouchpadEvent;
using ControllerSensorEventType   = SDL_ControllerSensorEvent;
using AudioDeviceEventType        = SDL_AudioDeviceEvent;
using SensorEventType             = SDL_SensorEvent;
using QuitEventType               = SDL_QuitEvent;
using UserEventType               = SDL_UserEvent;
using SysWMEventType              = SDL_SysWMEvent;
using TouchFingerEventType        = SDL_TouchFingerEvent;
using MultiGestureEventType       = SDL_MultiGestureEvent;
using DollarGestureEventType      = SDL_DollarGestureEvent;
using DropEventType               = SDL_DropEvent;

// ---------------------------------------------------------------------------
// std::default_delete<WindowType>
// ---------------------------------------------------------------------------

template <>
struct std::default_delete<WindowType>
{
    auto operator()(WindowType* window) const -> void
    {
        if(window != nullptr) {
            ::SDL_DestroyWindow(window);
        }
    }
};

// ---------------------------------------------------------------------------
// std::default_delete<RendererType>
// ---------------------------------------------------------------------------

template <>
struct std::default_delete<RendererType>
{
    auto operator()(RendererType* renderer) const -> void
    {
        if(renderer != nullptr) {
            ::SDL_DestroyRenderer(renderer);
        }
    }
};

// ---------------------------------------------------------------------------
// std::default_delete<SurfaceType>
// ---------------------------------------------------------------------------

template <>
struct std::default_delete<SurfaceType>
{
    auto operator()(SurfaceType* surface) const -> void
    {
        if(surface != nullptr) {
            ::SDL_FreeSurface(surface);
        }
    }
};

// ---------------------------------------------------------------------------
// std::default_delete<TextureType>
// ---------------------------------------------------------------------------

template <>
struct std::default_delete<TextureType>
{
    auto operator()(TextureType* texture) const -> void
    {
        if(texture != nullptr) {
            ::SDL_DestroyTexture(texture);
        }
    }
};

// ---------------------------------------------------------------------------
// <anonymous>::TextureLocker
// ---------------------------------------------------------------------------

namespace {

class TextureLocker
{
public: // public interface
    TextureLocker(TextureType* texture)
        : _texture(texture)
        , _pixels(nullptr)
        , _pitch(0)
    {
        if(::SDL_LockTexture(_texture, nullptr, &_pixels, &_pitch) != 0) {
            log_fatal("unable to lock texture");
        }
    }

   ~TextureLocker()
    {
        ::SDL_UnlockTexture(_texture);
    }

    auto pixels() const -> uint8_t*
    {
        return reinterpret_cast<uint8_t*>(_pixels);
    }

    auto pitch() const -> int
    {
        return _pitch;
    }

private: // private data
    TextureType* _texture;
    void*        _pixels;
    int          _pitch;
};

}

// ---------------------------------------------------------------------------
// <anonymous>::SDL
// ---------------------------------------------------------------------------

namespace {

class SDL final
    : public Library
{
public: // public interface
    SDL();

    SDL(SDL&&) = delete;

    SDL(const SDL&) = delete;

    SDL& operator=(SDL&&) = delete;

    SDL& operator=(const SDL&) = delete;

    virtual ~SDL();

private: // private data
    int      _status;
    uint32_t _flags;
};

}

// ---------------------------------------------------------------------------
// <anonymous>::BackendImpl
// ---------------------------------------------------------------------------

namespace {

class BackendImpl final
    : public Backend
{
public: // public interface
    BackendImpl(Engine& engine);

    BackendImpl(BackendImpl&&) = delete;

    BackendImpl(const BackendImpl&) = delete;

    BackendImpl& operator=(BackendImpl&&) = delete;

    BackendImpl& operator=(const BackendImpl&) = delete;

    virtual ~BackendImpl() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public backend interface
    virtual auto getTicks() -> uint32_t override final;

    virtual auto sleepFor(uint32_t delay) -> void override final;

    virtual auto sleepUntil(uint32_t ticks) -> void override final;

    virtual auto processEvents(Controls& controls) -> void override final;

    virtual auto updateScreen(const Page& page, const Palette& palette) -> void override final;

    virtual auto startAudio(AudioCallback callback, void* data) -> void override final;

    virtual auto stopAudio() -> void override final;

    virtual auto getAudioSampleRate() -> uint32_t override final;

    virtual auto addTimer(uint32_t delay, TimerCallback callback, void* data) -> int override final;

    virtual auto removeTimer(int timerId) -> void override final;

private: // private interface
    auto realize() -> void;

    auto unrealize() -> void;

private: // private stuff
    static constexpr int SCREEN_W  = 320; // width
    static constexpr int SCREEN_H  = 200; // height
    static constexpr int SCREEN_P  = 160; // pitch
    static constexpr int MIN_SCALE = 1;
    static constexpr int DFL_SCALE = 3;
    static constexpr int MAX_SCALE = 5;

    using LibraryPtr  = std::unique_ptr<SDL>;
    using WindowPtr   = std::unique_ptr<WindowType>;
    using RendererPtr = std::unique_ptr<RendererType>;
    using SurfacePtr  = std::unique_ptr<SurfaceType>;
    using TexturePtr  = std::unique_ptr<TextureType>;

private: // private data
    const std::string _title;
    LibraryPtr        _sdl;
    WindowPtr         _window;
    RendererPtr       _renderer;
    TexturePtr        _texture;
    AudioSpecType     _audiospec;
    int               _frames;
    int               _scale;
    bool              _crt;
};

}

// ---------------------------------------------------------------------------
// Backend
// ---------------------------------------------------------------------------

Backend::Backend(Engine& engine)
    : SubSystem(engine, "Backend")
{
}

auto Backend::create(Engine& engine) -> Backend*
{
    return new BackendImpl(engine);
}

// ---------------------------------------------------------------------------
// <anonymous>::SDL
// ---------------------------------------------------------------------------

namespace {

SDL::SDL()
    : _status(0)
    , _flags(0)
{
    LOG_DEBUG("starting sdl...");
    if(_flags == 0) {
        _flags |= SDL_INIT_TIMER;
        _flags |= SDL_INIT_AUDIO;
        _flags |= SDL_INIT_VIDEO;
        _flags |= SDL_INIT_JOYSTICK;
        _flags |= SDL_INIT_GAMECONTROLLER;
        _flags |= SDL_INIT_EVENTS;
    }
    if((_status = ::SDL_Init(_flags)) != 0) {
        log_fatal("SDL_Init() has failed");
    }
    LOG_DEBUG("sdl is started!");
}

SDL::~SDL()
{
    LOG_DEBUG("stopping sdl...");
    if(_status == 0) {
        ::SDL_Quit();
    }
    LOG_DEBUG("sdl is stopped!");
}

}

// ---------------------------------------------------------------------------
// <anonymous>::BackendImpl
// ---------------------------------------------------------------------------

namespace {

BackendImpl::BackendImpl(Engine& engine)
    : Backend(engine)
    , _title("Another World Interpreter")
    , _sdl(nullptr)
    , _window(nullptr)
    , _renderer(nullptr)
    , _texture(nullptr)
    , _audiospec()
    , _frames(0)
    , _scale(DFL_SCALE)
    , _crt(false)
{
    _audiospec.freq     = AUDIO_SAMPLE_RATE;
    _audiospec.format   = AUDIO_F32SYS;
    _audiospec.channels = 1;
    _audiospec.samples  = _audiospec.freq / 25;
    _audiospec.callback = nullptr;
    _audiospec.userdata = nullptr;
}

auto BackendImpl::start() -> void
{
    LOG_DEBUG("starting...");
    if(bool(_sdl) == false) {
        _sdl = std::make_unique<SDL>();
    }
    realize();
    LOG_DEBUG("started!");
}

auto BackendImpl::reset() -> void
{
    LOG_DEBUG("resetting...");
    LOG_DEBUG("reset!");
}

auto BackendImpl::stop() -> void
{
    LOG_DEBUG("stopping...");
    unrealize();
    if(bool(_sdl) != false) {
        _sdl.reset();
    }
    LOG_DEBUG("stopped!");
}

auto BackendImpl::getTicks() -> uint32_t
{
    return ::SDL_GetTicks();
}

auto BackendImpl::sleepFor(uint32_t delay) -> void
{
    ::SDL_Delay(delay);
}

auto BackendImpl::sleepUntil(uint32_t ticks) -> void
{
    const uint32_t currTicks = ::SDL_GetTicks();
    const uint32_t nextTicks = ticks;

    if(nextTicks > currTicks) {
        ::SDL_Delay(nextTicks - currTicks);
    }
}

auto BackendImpl::processEvents(Controls& controls) -> void
{
    auto updateControls = [&]() -> void
    {
        controls.horz = 0;
        controls.vert = 0;
        controls.btns = 0;
        if((controls.mask & Controls::DPAD_RIGHT) != 0) {
            controls.horz = +1;
        }
        if((controls.mask & Controls::DPAD_LEFT) != 0) {
            controls.horz = -1;
        }
        if((controls.mask & Controls::DPAD_DOWN) != 0) {
            controls.vert = +1;
        }
        if((controls.mask & Controls::DPAD_UP) != 0) {
            controls.vert = -1;
        }
        if((controls.mask & Controls::DPAD_BUTTON) != 0) {
            controls.btns = +1;
        }
    };

    auto onQuit = [&](const QuitEventType& event) -> void
    {
        controls.quit = true;
    };

    auto onWindowEvent = [&](const WindowEventType& event) -> void
    {
        switch(event.event) {
            case SDL_WINDOWEVENT_EXPOSED:
                ::SDL_RenderCopy(_renderer.get(), _texture.get(), nullptr, nullptr);
                ::SDL_RenderPresent(_renderer.get());
                break;
            default:
                break;
        }
    };

    auto onKeyPress = [&](const KeyboardEventType& event) -> void
    {
        const auto mods = ::SDL_GetModState();

        switch(event.keysym.sym) {
            case SDLK_UP:
                controls.mask |= Controls::DPAD_UP;
                break;
            case SDLK_DOWN:
                controls.mask |= Controls::DPAD_DOWN;
                break;
            case SDLK_LEFT:
                controls.mask |= Controls::DPAD_LEFT;
                break;
            case SDLK_RIGHT:
                controls.mask |= Controls::DPAD_RIGHT;
                break;
            case SDLK_SPACE:
                controls.mask |= Controls::DPAD_BUTTON;
                break;
            case SDLK_0:
                _engine.requestPartId(GAME_PART0);
                break;
            case SDLK_1:
                _engine.requestPartId(GAME_PART1);
                break;
            case SDLK_2:
                _engine.requestPartId(GAME_PART2);
                break;
            case SDLK_3:
                _engine.requestPartId(GAME_PART3);
                break;
            case SDLK_4:
                _engine.requestPartId(GAME_PART4);
                break;
            case SDLK_5:
                _engine.requestPartId(GAME_PART5);
                break;
            case SDLK_6:
                _engine.requestPartId(GAME_PART6);
                break;
            case SDLK_7:
                _engine.requestPartId(GAME_PART7);
                break;
            case SDLK_8:
                _engine.requestPartId(GAME_PART8);
                break;
            case SDLK_9:
                _engine.requestPartId(GAME_PART9);
                break;
            case SDLK_a...SDLK_z:
                if(_engine.getCurPartId() != GAME_PART_LAST) {
                    if(event.keysym.sym == SDLK_c) {
                        _engine.requestPartId(GAME_PART_LAST);
                        break;
                    }
                    if(event.keysym.sym == SDLK_m) {
                        _crt ^= true;
                        break;
                    }
                    if(event.keysym.sym == SDLK_p) {
                        controls.pause ^= true;
                        break;
                    }
                    if(event.keysym.sym == SDLK_r) {
                        _engine.reset();
                        break;
                    }
                    if(event.keysym.sym == SDLK_v) {
                        _engine.switchPalettes();
                        break;
                    }
                }
                controls.input = event.keysym.sym & ~0x20;
                break;
            case SDLK_BACKSPACE:
                controls.input = '\b';
                break;
            case SDLK_RETURN:
                controls.input = '\r';
                break;
            case SDLK_TAB :
                unrealize();
                if(mods & (KMOD_LSHIFT | KMOD_RSHIFT)) {
                    if(--_scale < MIN_SCALE) {
                        _scale = MAX_SCALE;
                    }
                }
                else {
                    if(++_scale > MAX_SCALE) {
                        _scale = MIN_SCALE;
                    }
                }
                realize();
                break;
            case SDLK_ESCAPE:
                controls.quit = true;
                break;
            default:
                break;
        }
    };

    auto onKeyRelease = [&](const KeyboardEventType& event) -> void
    {
        switch(event.keysym.sym) {
            case SDLK_UP:
                controls.mask &= ~Controls::DPAD_UP;
                break;
            case SDLK_DOWN:
                controls.mask &= ~Controls::DPAD_DOWN;
                break;
            case SDLK_LEFT:
                controls.mask &= ~Controls::DPAD_LEFT;
                break;
            case SDLK_RIGHT:
                controls.mask &= ~Controls::DPAD_RIGHT;
                break;
            case SDLK_SPACE:
                controls.mask &= ~Controls::DPAD_BUTTON;
                break;
            case SDLK_RETURN:
                controls.mask &= ~Controls::DPAD_BUTTON;
                break;
            default:
                break;
        }
    };

    auto processAllEvents = [&]() -> void
    {
        EventType event;
        while(::SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    onQuit(event.quit);
                    break;
                case SDL_WINDOWEVENT:
                    onWindowEvent(event.window);
                    break;
                case SDL_KEYDOWN:
                    onKeyPress(event.key);
                    break;
                case SDL_KEYUP:
                    onKeyRelease(event.key);
                    break;
                default:
                    break;
            }
        }
        updateControls();
    };

    return processAllEvents();
}

auto BackendImpl::updateScreen(const Page& page, const Palette& palette) -> void
{
    auto mul_clamp = [](uint8_t value, float factor) -> uint8_t
    {
        float result = static_cast<float>(value) * factor;
        if(result >= 255.0f) {
            result = 255.0f;
        }
        return static_cast<uint8_t>(result);
    };

    auto render_std = [&]() -> void
    {
        const TextureLocker texture(_texture.get());
        const uint16_t screen_w = SCREEN_P;
        const uint16_t screen_h = SCREEN_H;
        const uint8_t* srcbuf   = page.data;
        uint8_t*       dstbuf   = texture.pixels();
        int            pitch    = texture.pitch();
        for(int y = screen_h; y != 0; --y) {
            /* render even line */ {
                const uint8_t* srcptr = srcbuf;
                uint8_t*       dstptr = dstbuf;
                for(int x = screen_w; x != 0; --x) {
                    const uint8_t colors = *srcptr++;
                    const auto c1(palette.data[(colors >> 4) & 0xf]);
                    const auto c2(palette.data[(colors >> 0) & 0xf]);
                    *dstptr++ = c1.r; *dstptr++ = c1.g; *dstptr++ = c1.b;
                    *dstptr++ = c1.r; *dstptr++ = c1.g; *dstptr++ = c1.b;
                    *dstptr++ = c2.r; *dstptr++ = c2.g; *dstptr++ = c2.b;
                    *dstptr++ = c2.r; *dstptr++ = c2.g; *dstptr++ = c2.b;
                }
                dstbuf += pitch;
            }
            /* render odd line */ {
                const uint8_t* srcptr = srcbuf;
                uint8_t*       dstptr = dstbuf;
                for(int x = screen_w; x != 0; --x) {
                    const uint8_t colors = *srcptr++;
                    const auto c1(palette.data[(colors >> 4) & 0xf]);
                    const auto c2(palette.data[(colors >> 0) & 0xf]);
                    *dstptr++ = c1.r; *dstptr++ = c1.g; *dstptr++ = c1.b;
                    *dstptr++ = c1.r; *dstptr++ = c1.g; *dstptr++ = c1.b;
                    *dstptr++ = c2.r; *dstptr++ = c2.g; *dstptr++ = c2.b;
                    *dstptr++ = c2.r; *dstptr++ = c2.g; *dstptr++ = c2.b;
                }
                dstbuf += pitch;
            }
            srcbuf += SCREEN_P;
        }
    };

    auto render_crt = [&]() -> void
    {
        const TextureLocker texture(_texture.get());
        const uint16_t screen_w = SCREEN_P;
        const uint16_t screen_h = SCREEN_H;
        const uint8_t* srcbuf   = page.data;
        uint8_t*       dstbuf   = texture.pixels();
        int            pitch    = texture.pitch();
        for(int y = screen_h; y != 0; --y) {
            /* render bright line */ {
                const uint8_t* srcptr = srcbuf;
                uint8_t*       dstptr = dstbuf;
                for(int x = screen_w; x != 0; --x) {
                    const uint8_t colors = *srcptr++;
                    const auto c1(palette.data[(colors >> 4) & 0xf]);
                    const auto c2(palette.data[(colors >> 0) & 0xf]);
                    constexpr float factor1 = 1.00f * 1.15f;
                    constexpr float factor2 = 0.97f * 1.15f;
                    *dstptr++ = mul_clamp(c1.r, factor1);
                    *dstptr++ = mul_clamp(c1.g, factor1);
                    *dstptr++ = mul_clamp(c1.b, factor1);
                    *dstptr++ = mul_clamp(c1.r, factor2);
                    *dstptr++ = mul_clamp(c1.g, factor2);
                    *dstptr++ = mul_clamp(c1.b, factor2);
                    *dstptr++ = mul_clamp(c2.r, factor1);
                    *dstptr++ = mul_clamp(c2.g, factor1);
                    *dstptr++ = mul_clamp(c2.b, factor1);
                    *dstptr++ = mul_clamp(c2.r, factor2);
                    *dstptr++ = mul_clamp(c2.g, factor2);
                    *dstptr++ = mul_clamp(c2.b, factor2);
                }
                dstbuf += pitch;
            }
            /* render dark line */ {
                const uint8_t* srcptr = srcbuf;
                uint8_t*       dstptr = dstbuf;
                for(int x = screen_w; x != 0; --x) {
                    const uint8_t colors = *srcptr++;
                    const auto c1(palette.data[(colors >> 4) & 0xf]);
                    const auto c2(palette.data[(colors >> 0) & 0xf]);
                    constexpr float factor1 = 0.97f * 0.85f;
                    constexpr float factor2 = 1.00f * 0.85f;
                    *dstptr++ = mul_clamp(c1.r, factor1);
                    *dstptr++ = mul_clamp(c1.g, factor1);
                    *dstptr++ = mul_clamp(c1.b, factor1);
                    *dstptr++ = mul_clamp(c1.r, factor2);
                    *dstptr++ = mul_clamp(c1.g, factor2);
                    *dstptr++ = mul_clamp(c1.b, factor2);
                    *dstptr++ = mul_clamp(c2.r, factor1);
                    *dstptr++ = mul_clamp(c2.g, factor1);
                    *dstptr++ = mul_clamp(c2.b, factor1);
                    *dstptr++ = mul_clamp(c2.r, factor2);
                    *dstptr++ = mul_clamp(c2.g, factor2);
                    *dstptr++ = mul_clamp(c2.b, factor2);
                }
                dstbuf += pitch;
            }
            srcbuf += SCREEN_P;
        }
    };

    /* render */ {
        if(_crt != false) {
            render_crt();
        }
        else {
            render_std();
        }
    }
    /* blit to screen */ {
        ::SDL_RenderCopy(_renderer.get(), _texture.get(), nullptr, nullptr);
        ::SDL_RenderPresent(_renderer.get());
    }
    /* compute fps */ {
        ++_frames;
        _currTicks = getTicks();
        if(_currTicks >= _nextTicks) {
            const uint32_t dt = (_currTicks - _prevTicks);
            if(dt != 0) {
                LOG_DEBUG("framerate: %dfps", ((_frames * 1000) / dt));
            }
            _prevTicks = _currTicks;
            _nextTicks = _currTicks + 1000;
            _frames = 0;
        }
    }
}

auto BackendImpl::startAudio(AudioCallback callback, void* userdata) -> void
{
    LOG_DEBUG("starting audio...");
    _audiospec.callback = callback;
    _audiospec.userdata = userdata;

    if(::SDL_OpenAudio(&_audiospec, nullptr) == 0) {
        ::SDL_PauseAudio(0);
    }
    else {
        log_fatal("startAudio() has failed");
    }
    LOG_DEBUG("audio started!");
}

auto BackendImpl::stopAudio() -> void
{
    LOG_DEBUG("stopping audio...");
    ::SDL_CloseAudio();
    LOG_DEBUG("audio stopped!");
}

auto BackendImpl::getAudioSampleRate() -> uint32_t
{
    return _audiospec.freq;
}

auto BackendImpl::addTimer(uint32_t delay, TimerCallback callback, void* data) -> int
{
    return ::SDL_AddTimer(delay, reinterpret_cast<SDL_TimerCallback>(callback), data);
}

auto BackendImpl::removeTimer(int timerId) -> void
{
    ::SDL_RemoveTimer(timerId);
}

auto BackendImpl::realize() -> void
{
    auto set_hints = [&]() -> void
    {
        static_cast<void>(::SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"));
    };

    auto create_window = [&]() -> void
    {
        const int      pos_x = SDL_WINDOWPOS_UNDEFINED;
        const int      pos_y = SDL_WINDOWPOS_UNDEFINED;
        const int      dim_w = SCREEN_W * _scale;
        const int      dim_h = SCREEN_H * _scale;
        const uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

        if(bool(_window) == false) {
            _window.reset(::SDL_CreateWindow(_title.c_str(), pos_x, pos_y, dim_w, dim_h, flags));
        }
        if(bool(_window) == false) {
            log_fatal("SDL_CreateWindow() has failed");
        }
    };

    auto create_renderer = [&]() -> void
    {
        const int      index = -1;
        const uint32_t flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

        if(bool(_renderer) == false) {
            _renderer.reset(::SDL_CreateRenderer(_window.get(), index, flags));
        }
        if(bool(_renderer) == false) {
            log_fatal("SDL_CreateRenderer() has failed");
        }
    };

    auto create_texture = [&]() -> void
    {
        const uint32_t format = SDL_PIXELFORMAT_RGB24;
        const int      access = SDL_TEXTUREACCESS_STREAMING;
        const int      dim_w  = SCREEN_W * 2;
        const int      dim_h  = SCREEN_H * 2;

        if(bool(_texture) == false) {
            _texture.reset(::SDL_CreateTexture(_renderer.get(), format, access, dim_w, dim_h));
        }
        if(bool(_texture) == false) {
            log_fatal("SDL_CreateTexture() has failed");
        }
    };

    auto create_all = [&]() -> void
    {
        LOG_DEBUG("realizing...");
        set_hints();
        create_window();
        create_renderer();
        create_texture();
        LOG_DEBUG("realized!");
    };

    return create_all();
}

auto BackendImpl::unrealize() -> void
{
    auto destroy_texture = [&]() -> void
    {
        _texture.reset();
    };

    auto destroy_renderer = [&]() -> void
    {
        _renderer.reset();
    };

    auto destroy_window = [&]() -> void
    {
        _window.reset();
    };

    auto destroy_all = [&]() -> void
    {
        LOG_DEBUG("unrealizing...");
        destroy_texture();
        destroy_renderer();
        destroy_window();
        LOG_DEBUG("unrealized!");
    };

    return destroy_all();
}

}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------
