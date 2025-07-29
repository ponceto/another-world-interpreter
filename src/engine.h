/*
 * engine.h - Copyright (c) 2004-2025
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
#ifndef __AW_ENGINE_H__
#define __AW_ENGINE_H__

#include "intern.h"
#include "backend.h"
#include "resources.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "vm.h"

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

class Engine final
    : public SubSystem
{
public: // public interface
    Engine(const std::string& datadir, const std::string& dumpdir);

    Engine(Engine&&) = delete;

    Engine(const Engine&) = delete;

    Engine& operator=(Engine&&) = delete;

    Engine& operator=(const Engine&) = delete;

    virtual ~Engine() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public interface
    auto main() -> void;

    auto getDataDir() const -> const std::string&
    {
        return _datadir;
    }

    auto getDumpDir() const -> const std::string&
    {
        return _dumpdir;
    }

    auto initPart(uint16_t id) -> void;

    auto switchPalettes() -> void;

public: // public backend interface
    auto getTicks() -> uint32_t
    {
        return _backend->getTicks();
    }

    auto sleepFor(uint32_t delay) -> void
    {
        return _backend->sleepFor(delay);
    }

    auto sleepUntil(uint32_t ticks) -> void
    {
        return _backend->sleepUntil(ticks);
    }

    auto processEvents() -> void
    {
        return _backend->processEvents(_input.getControls());
    }

    auto updateScreen(const Page& page, const Palette& palette) -> void
    {
        return _backend->updateScreen(page, palette);
    }

    auto startAudio(AudioCallback callback, void* data) -> void
    {
        return _backend->startAudio(callback, data);
    }

    auto stopAudio() -> void
    {
        return _backend->stopAudio();
    }

    auto getAudioSampleRate() -> uint32_t
    {
        return _backend->getAudioSampleRate();
    }

    auto addTimer(TimerCallback callback, uint32_t delay, void* data) -> int
    {
        return _backend->addTimer(delay, callback, data);
    }

    auto removeTimer(int timer) -> void
    {
        return _backend->removeTimer(timer);
    }

public: // public resource interface
    auto getResource(uint16_t resourceId) -> Resource*
    {
        return _resources.getResource(resourceId);
    }

    auto loadResource(uint16_t resourceId) -> void
    {
        return _resources.loadResource(resourceId);
    }

    auto getCurPartId() const -> uint16_t
    {
        return _resources.getCurPartId();
    }

    auto getReqPartId() const -> uint16_t
    {
        return _resources.getReqPartId();
    }

    auto requestPartId(uint16_t partId) -> void
    {
        return _resources.requestPartId(partId);
    }

    auto getPolygonData(int index) -> const uint8_t*
    {
        return _resources.getPolygonData(index);
    }

    auto getString(uint16_t id) -> const char*
    {
        const String* string = _resources.getString(id);

        if(string != nullptr) {
            return string->value;
        }
        return nullptr;
    }

public: // public video interface
    auto selectPalette(uint8_t palette) -> void
    {
        return _video.selectPalette(palette);
    }

    auto selectPage(uint8_t dst) -> void
    {
        return _video.selectPage(dst);
    }

    auto fillPage(uint8_t dst, uint8_t col) -> void
    {
        return _video.fillPage(dst, col);
    }

    auto drawBitmap(const uint8_t* bitmap) -> void
    {
        return _video.drawBitmap(bitmap);
    }

    auto copyPage(uint8_t dst, uint8_t src, int16_t vscroll) -> void
    {
        return _video.copyPage(dst, src, vscroll);
    }

    auto blitPage(uint8_t src) -> void
    {
        return _video.blitPage(src);
    }

    auto drawString(uint16_t id, uint16_t x, uint16_t y, uint8_t color) -> void
    {
        return _video.drawString(id, x, y, color);
    }

    auto drawPolygons(const uint8_t* buffer, uint16_t offset, const Point& point, uint16_t zoom) -> void
    {
        return _video.drawPolygons(buffer, offset, point, zoom);
    }

public: // public audio interface
    auto playSound(uint16_t id, uint8_t channel, uint8_t volume, uint8_t frequency) -> void
    {
        return _audio.playSound(id, channel, volume, frequency);
    }

    auto playMusic(uint16_t id, uint8_t position, uint16_t delay) -> void
    {
        return _audio.playMusic(id, position, delay);
    }

public: // public input interface
    auto getControls() -> Controls&
    {
        return _input.getControls();
    }

    auto getControls() const -> const Controls&
    {
        return _input.getControls();
    }

    auto isRunning() const -> bool
    {
        const Controls& controls(_input.getControls());

        return controls.quit == false;
    }

    auto isStopped() const -> bool
    {
        const Controls& controls(_input.getControls());

        return controls.quit != false;
    }

    auto isPaused() const -> bool
    {
        const Controls& controls(_input.getControls());

        return controls.pause != false;
    }

public: // public vm interface
    auto setMusicMark(uint16_t value) -> void
    {
        return _vm.setRegister(VM_VARIABLE_MUSIC_MARK, value);
    }

    auto processVirtualMachine() -> void
    {
        return _vm.run(_input.getControls());
    }

private: // private interface
    struct Emscripten;

private: // private data
    const std::string _datadir;
    const std::string _dumpdir;
    uint8_t           _palmode;
    const BackendPtr  _backend;
    Resources         _resources;
    Video             _video;
    Audio             _audio;
    Input             _input;
    VirtualMachine    _vm;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_ENGINE_H__ */
