/*
 * input.h - Copyright (c) 2004-2025
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
#ifndef __AW_INPUT_H__
#define __AW_INPUT_H__

#include "intern.h"

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

class Input final
    : public SubSystem
{
public: // public interface
    Input(Engine& engine);

    Input(Input&&) = delete;

    Input(const Input&) = delete;

    Input& operator=(Input&&) = delete;

    Input& operator=(const Input&) = delete;

    virtual ~Input() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public input interface
    auto getControls() -> Controls&
    {
        return _controls;
    }

    auto getControls() const -> const Controls&
    {
        return _controls;
    }

private: // private data
    Controls _controls;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_INPUT_H__ */
