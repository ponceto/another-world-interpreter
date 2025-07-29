/*
 * vm.cc - Copyright (c) 2004-2025
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
#include "vm.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_VM, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// <anonymous>::condition
// ---------------------------------------------------------------------------

#ifndef NDEBUG
namespace {

const char* const condition[] = {
    "eq", "ne",
    "gt", "ge",
    "lt", "le",
    "??", "??",
};

}
#endif

// ---------------------------------------------------------------------------
// VirtualMachine
// ---------------------------------------------------------------------------

VirtualMachine::VirtualMachine(Engine& engine)
    : SubSystem(engine, "VirtualMachine")
    , _bytecode()
    , _threads()
    , _registers()
    , _stack()
{
    uint32_t index = 0;
    for(auto& thread : _threads) {
        thread.thread_id = index++;
    }
}

auto VirtualMachine::start() -> void
{
    LOG_DEBUG("starting...");
    LOG_DEBUG("started!");
}

auto VirtualMachine::reset() -> void
{
    LOG_DEBUG("resetting...");
    for(auto& _register : _registers) {
        _register.u = 0x0000;
    }
    _registers[VM_VARIABLE_RANDOM_SEED].u = static_cast<uint16_t>(::time(nullptr));
    _registers[0xe4].u = 0x0014; // DOS Version
#ifdef OUT_OF_THIS_WORLD
    _registers[0x54].u = 0x0081; // Out Of This World
#else
    _registers[0x54].u = 0x0001; // Another World
#endif
#ifdef BYPASS_PROTECTION
    _registers[0xbc].u = 0x0010; // set by the game protection code
    _registers[0xc6].u = 0x0080; // set by the game protection code
    _registers[0xdc].u = 0x0021; // set by the game protection code
    _registers[0xf2].u = 0x0fa0; // set by the game protection code
#endif
    LOG_DEBUG("reset!");
}

auto VirtualMachine::stop() -> void
{
    LOG_DEBUG("stopping...");
    LOG_DEBUG("stopped!");
}

auto VirtualMachine::setByteCode(const uint8_t* bytecode) -> void
{
    auto set_bytecode = [&]() -> void
    {
        _bytecode.reset(bytecode);
    };

    auto reset_threads = [&]() -> void
    {
        auto& thread0(_threads[0]);
        for(auto& thread : _threads) {
            thread.current_pc      = 0xffff;
            thread.requested_pc    = 0xffff;
            thread.current_state   = 0;
            thread.requested_state = 0;
            thread.opcode          = 0x3f;
            thread.yield           = false;
        }
        thread0.current_pc   = 0x0000;
        thread0.requested_pc = 0xffff;
    };

    auto reset_registers = [&]() -> void
    {
        constexpr uint16_t zero = 0;
        for(auto& _register : _registers) {
            _register.u |= zero;
        }
    };

    auto reset_stack = [&]() -> void
    {
        constexpr uint16_t zero = 0;
        for(auto& value : _stack.array) {
            value &= zero;
        }
        _stack.index = 0;
    };

    set_bytecode();
    reset_threads();
    reset_registers();
    reset_stack();
}

auto VirtualMachine::run(Controls& controls) -> void
{
    auto is_ready = [&]() -> bool
    {
        _currTicks = _engine.getTicks();
        if(_engine.isStopped() || _engine.isPaused()) {
            _nextTicks = _currTicks + 100;
            return false;
        }
        if(_currTicks >= _nextTicks) {
            _prevTicks = _nextTicks;
            return true;
        }
        return false;
    };

    auto process_inputs = [&]() -> void
    {
        _registers[VM_VARIABLE_INPUT_KEY].s            = 0;
        _registers[VM_VARIABLE_HERO_POS_LEFT_RIGHT].s  = controls.horz;
        _registers[VM_VARIABLE_HERO_POS_UP_DOWN].s     = controls.vert;
        _registers[VM_VARIABLE_HERO_POS_JUMP_DOWN].s   = controls.vert;
        _registers[VM_VARIABLE_HERO_POS_MASK].u        = controls.mask;
        _registers[VM_VARIABLE_HERO_ACTION].s          = controls.btns;
        _registers[VM_VARIABLE_HERO_ACTION_POS_MASK].u = controls.mask;

        if(controls.input != '\0') {
            _registers[VM_VARIABLE_INPUT_KEY].s = controls.input;
            controls.input = '\0';
        }
    };

    auto run_one_thread = [&](Thread& thread) -> void
    {
        thread.opcode = 0x3f;
        thread.yield  = false;
        if((thread.current_pc >= 0xffff) || (thread.current_state != 0)) {
            thread.yield = true;
        }
        if(thread.yield == false) {
            _bytecode.seek(thread.current_pc);
            do {
                thread.opcode = _bytecode.fetchByte();
                switch(thread.opcode) {
                    case 0x00: // movi [$x],u16
                        op_movi(thread);
                        break;
                    case 0x01: // movr [$x],[$y]
                        op_movr(thread);
                        break;
                    case 0x02: // addr [$x],[$y]
                        op_addr(thread);
                        break;
                    case 0x03: // addi [$x],u16
                        op_addi(thread);
                        break;
                    case 0x04: // call u16
                        op_call(thread);
                        break;
                    case 0x05: // ret
                        op_ret(thread);
                        break;
                    case 0x06: // yield
                        op_yield(thread);
                        break;
                    case 0x07: // jump u16
                        op_jump(thread);
                        break;
                    case 0x08: // init thread:u08,u16
                        op_init(thread);
                        break;
                    case 0x09: // djnz $[x],u16
                        op_djnz(thread);
                        break;
                    case 0x0a: // cjmp cond:u08,$[x],y
                        op_cjmp(thread);
                        break;
                    case 0x0b: // palette pal:u16
                        op_palette(thread);
                        break;
                    case 0x0c: // reset begin:u08,end:u08,state:u08
                        op_reset(thread);
                        break;
                    case 0x0d: // page src:u08
                        op_page(thread);
                        break;
                    case 0x0e: // fill dst:u08,col:u08
                        op_fill(thread);
                        break;
                    case 0x0f: // copy dst:u08,src:u08
                        op_copy(thread);
                        break;
                    case 0x10: // blit src:u08
                        op_blit(thread);
                        break;
                    case 0x11: // kill
                        op_kill(thread);
                        break;
                    case 0x12: // print string:u16,x:u08,y:u08,color:u08
                        op_print(thread);
                        break;
                    case 0x13: // subr [$x],[$y]
                        op_subr(thread);
                        break;
                    case 0x14: // andi [$x],u16
                        op_andi(thread);
                        break;
                    case 0x15: // iori [$x],u16
                        op_iori(thread);
                        break;
                    case 0x16: // shli [$x],u16
                        op_shli(thread);
                        break;
                    case 0x17: // shri [$x],u16
                        op_shri(thread);
                        break;
                    case 0x18: // sound id:u16,frequency:u08,volume:u08,channel:u08
                        op_sound(thread);
                        break;
                    case 0x19: // loadres id:u16
                        op_loadres(thread);
                        break;
                    case 0x1a: // music id:u16,delay:u16,position:u08
                        op_music(thread);
                        break;
                    case 0x1b ... 0x3f:
                        op_invalid(thread);
                        break;
                    case 0x40 ... 0x7f:
                        op_poly1(thread);
                        break;
                    case 0x80 ... 0xff:
                        op_poly2(thread);
                        break;
                    default:
                        op_invalid(thread);
                        break;
                }
                thread.current_pc = _bytecode.offset();
            } while(thread.yield == false);
        }
        thread.opcode = 0x3f;
        thread.yield  = false;
    };

    auto run_all_threads = [&]() -> void
    {
        LOG_DEBUG("+---+-----+\t");
        LOG_DEBUG("|tid|where|\t");
        LOG_DEBUG("+---+-----+\t");
        for(auto& thread : _threads) {
            thread.current_state = thread.requested_state;
            if(thread.requested_pc != 0xffff) {
                if(thread.requested_pc != 0xfffe) {
                    thread.current_pc   = thread.requested_pc;
                    thread.requested_pc = 0xffff;
                }
                else {
                    thread.current_pc   = 0xffff;
                    thread.requested_pc = 0xffff;
                }
            }
        }
        for(auto& thread : _threads) {
            run_one_thread(thread);
        }
        LOG_DEBUG("+---+-----+\t");
        LOG_DEBUG("");
    };

    auto schedule = [&]() -> void
    {
        const uint32_t pause = _registers[VM_VARIABLE_PAUSE_SLICES].u;
        const uint32_t delay = pause * 20;

        _currTicks = _engine.getTicks();
        _nextTicks = _prevTicks + delay;

        if(_nextTicks <= _currTicks) {
            _nextTicks = _currTicks + 1;
        }
    };

    if(is_ready()) {
        process_inputs();
        run_all_threads();
        schedule();
    }
}

// ---------------------------------------------------------------------------
// VirtualMachine: load/store instructions
// ---------------------------------------------------------------------------

auto VirtualMachine::op_movr(Thread& thread) -> void
{
    const uint8_t dst = _bytecode.fetchByte();
    const uint8_t src = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],[$%02x]", thread.thread_id, thread.current_pc, "movr", dst, src);

    _registers[dst].u = _registers[src].u;
}

auto VirtualMachine::op_movi(Thread& thread) -> void
{
    const uint8_t  dst = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],$%04x", thread.thread_id, thread.current_pc, "movi", dst, imm);

    _registers[dst].u = imm;
}

// ---------------------------------------------------------------------------
// VirtualMachine: arithmetic and logic instructions
// ---------------------------------------------------------------------------

auto VirtualMachine::op_addr(Thread& thread) -> void
{
    const uint8_t dst = _bytecode.fetchByte();
    const uint8_t src = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],[$%02x]", thread.thread_id, thread.current_pc, "addr", dst, src);

    _registers[dst].u += _registers[src].u;
}

auto VirtualMachine::op_addi(Thread& thread) -> void
{
    const uint8_t  dst = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],$%04x", thread.thread_id, thread.current_pc, "addi", dst, imm);

    _registers[dst].u += imm;
}

auto VirtualMachine::op_subr(Thread& thread) -> void
{
    const uint8_t dst = _bytecode.fetchByte();
    const uint8_t src = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],[$%02x]", thread.thread_id, thread.current_pc, "subr", dst, src);

    _registers[dst].u -= _registers[src].u;
}

auto VirtualMachine::op_andi(Thread& thread) -> void
{
    const uint8_t  dst = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],$%02x", thread.thread_id, thread.current_pc, "andi", dst, imm);

    _registers[dst].u &= imm;
}

auto VirtualMachine::op_iori(Thread& thread) -> void
{
    const uint8_t  dst = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],$%02x", thread.thread_id, thread.current_pc, "iori", dst, imm);

    _registers[dst].u |= imm;
}

auto VirtualMachine::op_shli(Thread& thread) -> void
{
    const uint8_t  dst = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],%d", thread.thread_id, thread.current_pc, "shli", dst, imm);

    _registers[dst].u <<= imm;
}

auto VirtualMachine::op_shri(Thread& thread) -> void
{
    const uint8_t  dst = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],%d", thread.thread_id, thread.current_pc, "shri", dst, imm);

    _registers[dst].u >>= imm;
}

// ---------------------------------------------------------------------------
// VirtualMachine: jump and call instructions
// ---------------------------------------------------------------------------

auto VirtualMachine::op_jump(Thread& thread) -> void
{
    const uint16_t loc = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%04x", thread.thread_id, thread.current_pc, "jump", loc);

    _bytecode.seek(loc);
}

auto VirtualMachine::op_cjmp(Thread& thread) -> void
{
    const uint8_t  variant = _bytecode.fetchByte();
    const uint8_t  compare = variant & 0x07;
    const uint8_t  rg1     = _bytecode.fetchByte();
    const uint16_t op1     = _registers[rg1].u;
    uint8_t        rg2     = 0;
    uint16_t       op2     = 0;
    uint16_t       loc     = 0;

    if(variant & 0x80) {
        rg2 = _bytecode.fetchByte();
        op2 = _registers[rg2].u;
        loc = _bytecode.fetchWord();
        LOG_DEBUG("|$%02x|$%04x|\t%-7s %s,[$%02x],[$%02x],$%04x", thread.thread_id, thread.current_pc, "jump", condition[compare], rg1, rg2, loc);
    }
    else if(variant & 0x40) {
        op2 = _bytecode.fetchWord();
        loc = _bytecode.fetchWord();
        LOG_DEBUG("|$%02x|$%04x|\t%-7s %s,[$%02x],$%04x,$%04x", thread.thread_id, thread.current_pc, "jump", condition[compare], rg1, op2, loc);
    }
    else {
        op2 = _bytecode.fetchByte();
        loc = _bytecode.fetchWord();
        LOG_DEBUG("|$%02x|$%04x|\t%-7s %s,[$%02x],$%02x,$%04x", thread.thread_id, thread.current_pc, "jump", condition[compare], rg1, op2, loc);
    }
#ifdef BYPASS_PROTECTION
    if((compare == 0) && (rg1 == 0x29) && (rg2 == 0x1e)) {
        if(_engine.getCurPartId() == GAME_PART0) {
            LOG_DEBUG("bypassing protection...");
            // 4 symbols
            _registers[0x29].u = _registers[0x1e].u;
            _registers[0x2a].u = _registers[0x1f].u;
            _registers[0x2b].u = _registers[0x20].u;
            _registers[0x2c].u = _registers[0x21].u;
            // counters
            _registers[0x32].u = 0x06;
            _registers[0x64].u = 0x14;
            // seek
            _bytecode.seek(loc);
            return;
        }
    }
#endif
    switch(compare) {
        case 0: // eq: equal
            if(static_cast<int16_t>(op1) == static_cast<int16_t>(op2)) {
                _bytecode.seek(loc);
            }
            break;
        case 1: // ne: not equal
            if(static_cast<int16_t>(op1) != static_cast<int16_t>(op2)) {
                _bytecode.seek(loc);
            }
            break;
        case 2: // gt: greater than
            if(static_cast<int16_t>(op1) > static_cast<int16_t>(op2)) {
                _bytecode.seek(loc);
            }
            break;
        case 3: // ge: greater or equal
            if(static_cast<int16_t>(op1) >= static_cast<int16_t>(op2)) {
                _bytecode.seek(loc);
            }
            break;
        case 4: // lt: less than
            if(static_cast<int16_t>(op1) < static_cast<int16_t>(op2)) {
                _bytecode.seek(loc);
            }
            break;
        case 5: // le: less or equal
            if(static_cast<int16_t>(op1) <= static_cast<int16_t>(op2)) {
                _bytecode.seek(loc);
            }
            break;
        default:
            log_fatal("invalid conditional jump 0x%02x", compare);
            break;
    }
}

auto VirtualMachine::op_djnz(Thread& thread) -> void
{
    const uint8_t  dst = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s [$%02x],$%04x", thread.thread_id, thread.current_pc, "djnz", dst, imm);

    if(--_registers[dst].u != 0) {
        _bytecode.seek(imm);
    }
}

auto VirtualMachine::op_call(Thread& thread) -> void
{
    if(_stack.index >= countof(_stack.array)) {
        log_fatal("virtual machine had a stack overflow while executing 'call'");
    }
    const uint16_t loc = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%04x", thread.thread_id, thread.current_pc, "call", loc);

    _stack.array[_stack.index++] = _bytecode.offset();
    _bytecode.seek(loc);
}

auto VirtualMachine::op_ret(Thread& thread) -> void
{
    if(_stack.index == 0) {
        log_fatal("virtual machine had a stack underflow while executing 'ret'");
    }
    const uint16_t loc = _stack.array[--_stack.index];

    LOG_DEBUG("|$%02x|$%04x|\t%-7s (return to caller)", thread.thread_id, thread.current_pc, "ret");

    _bytecode.seek(loc);
}

// ---------------------------------------------------------------------------
// VirtualMachine: thread management instructions
// ---------------------------------------------------------------------------

auto VirtualMachine::op_init(Thread& thread) -> void
{
    const uint8_t  tid = _bytecode.fetchByte();
    const uint16_t imm = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x,$%04x", thread.thread_id, thread.current_pc, "init", tid, imm);

    _threads[tid].requested_pc = imm;
}

auto VirtualMachine::op_kill(Thread& thread) -> void
{
    LOG_DEBUG("|$%02x|$%04x|\t%-7s (kill this thread)", thread.thread_id, thread.current_pc, "kill");

    _bytecode.seek(0xffff);
    thread.yield = true;
}

auto VirtualMachine::op_yield(Thread& thread) -> void
{
    LOG_DEBUG("|$%02x|$%04x|\t%-7s (goto next thread)", thread.thread_id, thread.current_pc, "yield");

    thread.yield = true;
}

auto VirtualMachine::op_reset(Thread& thread) -> void
{
    const uint8_t begin = _bytecode.fetchByte();
    const uint8_t end   = _bytecode.fetchByte();
    const uint8_t state = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x,$%02x,$%02x", thread.thread_id, thread.current_pc, "reset", begin, end, state);

    switch(state) {
        case 0:
        case 1:
            for(int index = begin; index <= end; ++index) {
                _threads[index].requested_state = state;
            }
            break;
        case 2:
            for(int index = begin; index <= end; ++index) {
                _threads[index].requested_pc = 0xfffe;
            }
            break;
        default:
            log_fatal("op_reset has failed [state: 0x%02x]", state);
            break;
    }
}

// ---------------------------------------------------------------------------
// VirtualMachine: resources instructions
// ---------------------------------------------------------------------------

auto VirtualMachine::op_loadres(Thread& thread) -> void
{
    const uint16_t id = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x", thread.thread_id, thread.current_pc, "loadres", id);

    _engine.loadResource(id);
}

auto VirtualMachine::op_palette(Thread& thread) -> void
{
    const uint16_t palette = _bytecode.fetchWord();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%04x", thread.thread_id, thread.current_pc, "palette", palette);

    _engine.selectPalette(palette >> 8);
}

auto VirtualMachine::op_page(Thread& thread) -> void
{
    const uint8_t dst = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x", thread.thread_id, thread.current_pc, "page", dst);

    _engine.selectPage(dst);
}

auto VirtualMachine::op_fill(Thread& thread) -> void
{
    const uint8_t dst = _bytecode.fetchByte();
    const uint8_t col = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x,$%02x", thread.thread_id, thread.current_pc, "fill", dst, col);

    _engine.fillPage(dst, col);
}

auto VirtualMachine::op_copy(Thread& thread) -> void
{
    const uint8_t src = _bytecode.fetchByte();
    const uint8_t dst = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x,$%02x", thread.thread_id, thread.current_pc, "copy", dst, src);

    _engine.copyPage(dst, src, _registers[VM_VARIABLE_SCROLL_Y].u);
}

auto VirtualMachine::op_blit(Thread& thread) -> void
{
    const uint8_t src = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x", thread.thread_id, thread.current_pc, "blit", src);

    _engine.blitPage(src);
}

auto VirtualMachine::op_print(Thread& thread) -> void
{
    const uint16_t string = _bytecode.fetchWord();
    const uint8_t  text_x = _bytecode.fetchByte();
    const uint8_t  text_y = _bytecode.fetchByte();
    const uint8_t  color  = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%03x,%d,%d,%d", thread.thread_id, thread.current_pc, "print", string, text_x, text_y, color);

    _engine.drawString(string, text_x, text_y, color);
}

auto VirtualMachine::op_sound(Thread& thread) -> void
{
    const uint16_t sound_id  = _bytecode.fetchWord();
    const uint8_t  frequency = _bytecode.fetchByte();
    const uint8_t  volume    = _bytecode.fetchByte();
    const uint8_t  channel   = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x,%d,%d,%d", thread.thread_id, thread.current_pc, "sound", sound_id, frequency, volume, channel);

    _engine.playSound(sound_id, channel, volume, frequency);
}

auto VirtualMachine::op_music(Thread& thread) -> void
{
    const uint16_t music_id = _bytecode.fetchWord();
    const uint16_t delay    = _bytecode.fetchWord();
    const uint8_t  position = _bytecode.fetchByte();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x,%d,%d", thread.thread_id, thread.current_pc, "music", music_id, delay, position);

    _engine.playMusic(music_id, position, delay);
}

auto VirtualMachine::op_poly1(Thread& thread) -> void
{
    int polygonDataIndex = 1;

    auto get_offset = [&]() -> uint16_t
    {
        const uint16_t imm = _bytecode.fetchWord();

        return imm * 2;
    };

    auto get_poly_x = [&]() -> int16_t
    {
        uint16_t imm = static_cast<uint16_t>(_bytecode.fetchByte());

        if(thread.opcode & 0x20) {
            if(thread.opcode & 0x10) {
                imm += 0x100;
            }
        }
        else {
            if(thread.opcode & 0x10) {
                imm = _registers[imm].u;
            }
            else {
                imm = (imm << 8) | static_cast<uint16_t>(_bytecode.fetchByte());
            }
        }
        return imm;
    };

    auto get_poly_y = [&]() -> int16_t
    {
        uint16_t imm = static_cast<uint16_t>(_bytecode.fetchByte());

        if(thread.opcode & 0x08) {
            if(thread.opcode & 0x04) {
                imm += 0x000;
            }
        }
        else {
            if(thread.opcode & 0x04) {
                imm = _registers[imm].u;
            }
            else {
                imm = (imm << 8) | static_cast<uint16_t>(_bytecode.fetchByte());
            }
        }
        return imm;
    };

    auto get_zoom = [&]() -> uint16_t
    {
        uint16_t imm = 0x40;

        if(thread.opcode & 0x02) {
            if(thread.opcode & 0x01) {
                polygonDataIndex = 2;
                imm = 0x40;
            }
            else {
                imm = _bytecode.fetchByte();
            }
        }
        else {
            if(thread.opcode & 0x01) {
                imm = _registers[_bytecode.fetchByte()].u;
            }
            else {
                imm = 0x40;
            }
        }
        return imm;
    };

    auto get_point = [&](int16_t x, int16_t y) -> Point
    {
        return Point(x, y);
    };

    auto get_buffer = [&]() -> const uint8_t*
    {
        return _engine.getPolygonData(polygonDataIndex);
    };

    const uint16_t offset = get_offset();
    const int16_t  poly_x = get_poly_x();
    const int16_t  poly_y = get_poly_y();
    const uint16_t zoom   = get_zoom();
    const uint8_t* buffer = get_buffer();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s (buffer: %d, offset: %d, x: %d, y: %d, zoom: %d)", thread.thread_id, thread.current_pc, "poly1", polygonDataIndex, offset, poly_x, poly_y, zoom);

    _engine.drawPolygons(buffer, offset, get_point(poly_x, poly_y), zoom);
}

auto VirtualMachine::op_poly2(Thread& thread) -> void
{
    int polygonDataIndex = 1;

    auto get_offset = [&]() -> uint16_t
    {
        const uint16_t msb = static_cast<uint16_t>(thread.opcode) << 8;
        const uint16_t lsb = static_cast<uint16_t>(_bytecode.fetchByte());
        const uint16_t imm = (msb | lsb);

        return imm * 2;
    };

    auto get_poly_x = [&]() -> int16_t
    {
        const uint16_t imm = static_cast<uint16_t>(_bytecode.fetchByte());

        return imm;
    };

    auto get_poly_y = [&]() -> int16_t
    {
        const uint16_t imm = static_cast<uint16_t>(_bytecode.fetchByte());

        return imm;
    };

    auto get_zoom = [&]() -> uint16_t
    {
        const uint16_t imm = 0x40;

        return imm;
    };

    auto get_point = [&](int16_t x, int16_t y) -> Point
    {
        const int16_t h = (y - 199);
        if(h > 0) {
            y = 199;
            x += h;
        }
        return Point(x, y);
    };

    auto get_buffer = [&]() -> const uint8_t*
    {
        return _engine.getPolygonData(polygonDataIndex);
    };

    const uint16_t offset = get_offset();
    const int16_t  poly_x = get_poly_x();
    const int16_t  poly_y = get_poly_y();
    const uint16_t zoom   = get_zoom();
    const uint8_t* buffer = get_buffer();

    LOG_DEBUG("|$%02x|$%04x|\t%-7s (buffer: %d, offset: %d, x: %d, y: %d, zoom: %d)", thread.thread_id, thread.current_pc, "poly2", polygonDataIndex, offset, poly_x, poly_y, zoom);

    _engine.drawPolygons(buffer, offset, get_point(poly_x, poly_y), zoom);
}

auto VirtualMachine::op_invalid(Thread& thread) -> void
{
    LOG_DEBUG("|$%02x|$%04x|\t%-7s $%02x", thread.thread_id, thread.current_pc, "invalid", thread.opcode);

    log_fatal("invalid opcode 0x%02x at 0x%04x", thread.opcode, thread.current_pc);
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------
