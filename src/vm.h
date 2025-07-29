/*
 * vm.h - Copyright (c) 2004-2025
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
#ifndef __AW_VIRTUALMACHINE_H__
#define __AW_VIRTUALMACHINE_H__

#include "intern.h"

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

enum Variables
{
    VM_VARIABLE_RANDOM_SEED          = 0x3c,
    VM_VARIABLE_INPUT_KEY            = 0xda,
    VM_VARIABLE_HERO_POS_UP_DOWN     = 0xe5,
    VM_VARIABLE_MUSIC_MARK           = 0xf4,
    VM_VARIABLE_SCROLL_Y             = 0xf9,
    VM_VARIABLE_HERO_ACTION          = 0xfa,
    VM_VARIABLE_HERO_POS_JUMP_DOWN   = 0xfb,
    VM_VARIABLE_HERO_POS_LEFT_RIGHT  = 0xfc,
    VM_VARIABLE_HERO_POS_MASK        = 0xfd,
    VM_VARIABLE_HERO_ACTION_POS_MASK = 0xfe,
    VM_VARIABLE_PAUSE_SLICES         = 0xff
};

// ---------------------------------------------------------------------------
// VirtualMachine
// ---------------------------------------------------------------------------

class VirtualMachine final
    : public SubSystem
{
public: // public interface
    VirtualMachine(Engine& engine);

    VirtualMachine(VirtualMachine&&) = delete;

    VirtualMachine(const VirtualMachine&) = delete;

    VirtualMachine& operator=(VirtualMachine&&) = delete;

    VirtualMachine& operator=(const VirtualMachine&) = delete;

    virtual ~VirtualMachine() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public vm interface
    auto run(Controls& controls) -> void;

    auto setByteCode(const uint8_t* bytecode) -> void;

    auto getRegister(uint8_t index) const -> uint16_t
    {
        return _registers[index].u;
    }

    auto setRegister(uint8_t index, uint16_t value)
    {
        _registers[index].u = value;
    }

private: // private interface
    struct Thread
    {
        uint32_t thread_id       = 0;
        uint16_t current_pc      = 0;
        uint16_t requested_pc    = 0;
        uint8_t  current_state   = 0;
        uint8_t  requested_state = 0;
        uint8_t  opcode          = 0x3f;
        bool     yield           = false;
    };

    struct Register
    {
        union {
            int16_t  s;
            uint16_t u;
        };
    };

    struct Stack
    {
        uint32_t array[256];
        uint32_t index;
    };

    auto op_movr(Thread& thread) -> void;

    auto op_movi(Thread& thread) -> void;

    auto op_addr(Thread& thread) -> void;

    auto op_addi(Thread& thread) -> void;

    auto op_subr(Thread& thread) -> void;

    auto op_andi(Thread& thread) -> void;

    auto op_iori(Thread& thread) -> void;

    auto op_shli(Thread& thread) -> void;

    auto op_shri(Thread& thread) -> void;

    auto op_jump(Thread& thread) -> void;

    auto op_cjmp(Thread& thread) -> void;

    auto op_djnz(Thread& thread) -> void;

    auto op_call(Thread& thread) -> void;

    auto op_ret(Thread& thread) -> void;

    auto op_init(Thread& thread) -> void;

    auto op_kill(Thread& thread) -> void;

    auto op_yield(Thread& thread) -> void;

    auto op_reset(Thread& thread) -> void;

    auto op_loadres(Thread& thread) -> void;

    auto op_palette(Thread& thread) -> void;

    auto op_page(Thread& thread) -> void;

    auto op_fill(Thread& thread) -> void;

    auto op_copy(Thread& thread) -> void;

    auto op_blit(Thread& thread) -> void;

    auto op_print(Thread& thread) -> void;

    auto op_sound(Thread& thread) -> void;

    auto op_music(Thread& thread) -> void;

    auto op_poly1(Thread& thread) -> void;

    auto op_poly2(Thread& thread) -> void;

    auto op_invalid(Thread& thread) -> void;

private: // private data
    ByteCode _bytecode;
    Thread   _threads[64];
    Register _registers[256];
    Stack    _stack;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_VIRTUALMACHINE_H__ */
