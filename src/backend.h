/*
 * backend.h - Copyright (c) 2004-2025
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
#ifndef __AW_BACKEND_H__
#define __AW_BACKEND_H__

#include "intern.h"

// ---------------------------------------------------------------------------
// Backend
// ---------------------------------------------------------------------------

class Backend
    : public SubSystem
{
public: // public interface
    Backend(Engine& engine);

    Backend(Backend&&) = delete;

    Backend(const Backend&) = delete;

    Backend& operator=(Backend&&) = delete;

    Backend& operator=(const Backend&) = delete;

    virtual ~Backend() = default;

public: // public backend interface
    static auto create(Engine& engine) -> Backend*;

    virtual auto getTicks() -> uint32_t = 0;

    virtual auto sleepFor(uint32_t delay) -> void = 0;

    virtual auto sleepUntil(uint32_t ticks) -> void = 0;

    virtual auto processEvents(Controls& controls) -> void = 0;

    virtual auto updateScreen(const Page& page, const Palette& palette) -> void = 0;

    virtual auto startAudio(AudioCallback callback, void* userdata) -> void = 0;

    virtual auto stopAudio() -> void = 0;

    virtual auto getAudioSampleRate() -> uint32_t = 0;

    virtual auto addTimer(uint32_t delay, TimerCallback callback, void* data) -> int = 0;

    virtual auto removeTimer(int timerId) -> void = 0;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_BACKEND_H__ */
