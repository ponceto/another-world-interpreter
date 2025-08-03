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

#define COLOR_BLACK 0xFF
#define DEFAULT_ZOOM 0x40

// ---------------------------------------------------------------------------
// VirtualMachine
// ---------------------------------------------------------------------------

const uint16_t VirtualMachine::frequencyTable[] = {
    0x0CFF, 0x0DC3, 0x0E91, 0x0F6F, 0x1056, 0x114E, 0x1259, 0x136C,
    0x149F, 0x15D9, 0x1726, 0x1888, 0x19FD, 0x1B86, 0x1D21, 0x1EDE,
    0x20AB, 0x229C, 0x24B3, 0x26D7, 0x293F, 0x2BB2, 0x2E4C, 0x3110,
    0x33FB, 0x370D, 0x3A43, 0x3DDF, 0x4157, 0x4538, 0x4998, 0x4DAE,
    0x5240, 0x5764, 0x5C9A, 0x61C8, 0x6793, 0x6E19, 0x7485, 0x7BBD
};

VirtualMachine::VirtualMachine(Engine& eng)
	: engine(eng)
	, _currTimeStamp(0)
	, _nextTimeStamp(0)
	, _prevTimeStamp(0)
	, _bytecode()
{
}

void VirtualMachine::init() {

	memset(vmVariables, 0, sizeof(vmVariables));
	vmVariables[VM_VARIABLE_RANDOM_SEED] = ::time(nullptr);
#ifdef BYPASS_PROTECTION
	// these 3 variables are set by the game code
	vmVariables[0xBC] = 0x0010;
	vmVariables[0xC6] = 0x0080;
	vmVariables[0xF2] = 0x0FA0;
	// these 2 variables are set by the engine executable
	vmVariables[0xDC] = 0x0021;
#endif

#ifdef OUT_OF_THIS_WORLD
	vmVariables[0x54] = 0x0081; // Out Of This World
#else
	vmVariables[0x54] = 0x0001; // Another World
#endif
	vmVariables[0xE4] = 0x0014; // DOS Version

    initForPart(0x0000);
}

void VirtualMachine::fini() {
}

void VirtualMachine::op_trap(uint8_t opcode) {
    log_debug(SYS_VM, "VirtualMachine::op_trap()");

    log_fatal("invalid opcode 0x%02x", opcode);
}

void VirtualMachine::op_poly1(uint8_t opcode) {
    log_debug(SYS_VM, "VirtualMachine::op_poly1()");
    int16_t x, y;
    uint16_t off = _bytecode.fetchWord() * 2;
    x = _bytecode.fetchByte();

    engine.resources.useSegVideo2 = false;

    if (!(opcode & 0x20))
    {
        if (!(opcode & 0x10))  // 0001 0000 is set
        {
            x = (x << 8) | _bytecode.fetchByte();
        } else {
            x = vmVariables[x];
        }
    }
    else
    {
        if (opcode & 0x10) { // 0001 0000 is set
            x += 0x100;
        }
    }

    y = _bytecode.fetchByte();

    if (!(opcode & 8))  // 0000 1000 is set
    {
        if (!(opcode & 4)) { // 0000 0100 is set
            y = (y << 8) | _bytecode.fetchByte();
        } else {
            y = vmVariables[y];
        }
    }

    uint16_t zoom = _bytecode.fetchByte();

    if (!(opcode & 2))  // 0000 0010 is set
    {
        if (!(opcode & 1)) // 0000 0001 is set
        {
            _bytecode.rewind(1);
            zoom = 0x40;
        }
        else
        {
            zoom = vmVariables[zoom];
        }
    }
    else
    {

        if (opcode & 1) { // 0000 0001 is set
            engine.resources.useSegVideo2 = true;
            _bytecode.rewind(1);
            zoom = 0x40;
        }
    }
    log_debug(SYS_VM, "vid_opcd_0x40 : off=0x%x x=%d y=%d", off, x, y);
    engine.video.setDataBuffer(engine.resources.useSegVideo2 ? engine.resources.segVideo2 : engine.resources.segPolygon, off);
    engine.video.readAndDrawPolygon(0xFF, zoom, Point(x, y));
}

void VirtualMachine::op_poly2(uint8_t opcode) {
    log_debug(SYS_VM, "VirtualMachine::op_poly2()");
    uint16_t off = ((opcode << 8) | _bytecode.fetchByte()) * 2;
    engine.resources.useSegVideo2 = false;
    int16_t x = _bytecode.fetchByte();
    int16_t y = _bytecode.fetchByte();
    int16_t h = y - 199;
    if (h > 0) {
        y = 199;
        x += h;
    }
    log_debug(SYS_VM, "vid_opcd_0x80 : opcode=0x%x off=0x%x x=%d y=%d", opcode, off, x, y);

    // This switch the polygon database to "cinematic" and probably draws a black polygon
    // over all the screen.
    engine.video.setDataBuffer(engine.resources.segPolygon, off);
    engine.video.readAndDrawPolygon(COLOR_BLACK, DEFAULT_ZOOM, Point(x,y));
}

void VirtualMachine::op_movi() {
	uint8_t variableId = _bytecode.fetchByte();
	int16_t value = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_movi(0x%02x, %d)", variableId, value);
	vmVariables[variableId] = value;
}

void VirtualMachine::op_movw() {
	uint8_t dstVariableId = _bytecode.fetchByte();
	uint8_t srcVariableId = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_movw(0x%02x, 0x%02x)", dstVariableId, srcVariableId);
	vmVariables[dstVariableId] = vmVariables[srcVariableId];
}

void VirtualMachine::op_addw() {
	uint8_t dstVariableId = _bytecode.fetchByte();
	uint8_t srcVariableId = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_addw(0x%02x, 0x%02x)", dstVariableId, srcVariableId);
	vmVariables[dstVariableId] += vmVariables[srcVariableId];
}

void VirtualMachine::op_addi() {
	if (engine.resources.currentPartId == 0x3E86 && _bytecode.get() == engine.resources.segByteCode + 0x6D48) {
		log_alert("VirtualMachine::op_addi() hack for non-stop looping gun sound bug");
		// the script 0x27 slot 0x17 doesn't stop the gun sound from looping, I
		// don't really know why ; for now, let's play the 'stopping sound' like
		// the other scripts do
		//  (0x6D43) jmp(0x6CE5)
		//  (0x6D46) break
		//  (0x6D47) VAR(6) += -50
		snd_playSound(0x5B, 1, 64, 1);
	}
	uint8_t variableId = _bytecode.fetchByte();
	int16_t value = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_addi(0x%02x, %d)", variableId, value);
	vmVariables[variableId] += value;
}

void VirtualMachine::op_call() {

	uint16_t offset = _bytecode.fetchWord();
	uint8_t sp = _stackPtr;

	log_debug(SYS_VM, "VirtualMachine::op_call(0x%x)", offset);
	_scriptStackCalls[sp] = _bytecode.get() - engine.resources.segByteCode;
	if (_stackPtr == 0xFF) {
		log_fatal("VirtualMachine::op_call() ec=0x%x stack overflow", 0x8F);
	}
	++_stackPtr;
	_bytecode.set(engine.resources.segByteCode + offset);
}

void VirtualMachine::op_ret() {
	log_debug(SYS_VM, "VirtualMachine::op_ret()");
	if (_stackPtr == 0) {
		log_fatal("VirtualMachine::op_ret() ec=0x%x stack underflow", 0x8F);
	}
	--_stackPtr;
	uint8_t sp = _stackPtr;
	_bytecode.set(engine.resources.segByteCode + _scriptStackCalls[sp]);
}

void VirtualMachine::op_yield() {
	log_debug(SYS_VM, "VirtualMachine::op_yield()");
	_bytecode.yield(true);
}

void VirtualMachine::op_jmp() {
	uint16_t pcOffset = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_jmp(0x%02x)", pcOffset);
	_bytecode.set(engine.resources.segByteCode + pcOffset);
}

void VirtualMachine::op_setvec() {
	uint8_t threadId = _bytecode.fetchByte();
	uint16_t pcOffsetRequested = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_setvec(0x%x, 0x%x)", threadId,pcOffsetRequested);
	threadsData[REQUESTED_PC_OFFSET][threadId] = pcOffsetRequested;
}

void VirtualMachine::op_jnz() {
	uint8_t i = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_jnz(0x%02x)", i);
	--vmVariables[i];
	if (vmVariables[i] != 0) {
		op_jmp();
	} else {
		_bytecode.fetchWord();
	}
}

void VirtualMachine::op_cjmp() {
	uint8_t opcode = _bytecode.fetchByte();
	const uint8_t var = _bytecode.fetchByte();
	int16_t b = vmVariables[var];
	int16_t a;

	if (opcode & 0x80) {
		a = vmVariables[_bytecode.fetchByte()];
	} else if (opcode & 0x40) {
		a = _bytecode.fetchWord();
	} else {
		a = _bytecode.fetchByte();
	}
	log_debug(SYS_VM, "VirtualMachine::op_cjmp(%d, 0x%02x, 0x%02x)", opcode, b, a);

	// Check if the conditional value is met.
	bool expr = false;
	switch (opcode & 7) {
	case 0:	// jz
		expr = (b == a);
#ifdef BYPASS_PROTECTION
      if (engine.resources.currentPartId == 16000) {
        //
        // 0CB8: jmpIf(VAR(0x29) == VAR(0x1E), @0CD3)
        // ...
        //
        if (var == 0x29 && (opcode & 0x80) != 0) {
          // 4 symbols
          vmVariables[0x29] = vmVariables[0x1E];
          vmVariables[0x2A] = vmVariables[0x1F];
          vmVariables[0x2B] = vmVariables[0x20];
          vmVariables[0x2C] = vmVariables[0x21];
          // counters
          vmVariables[0x32] = 6;
          vmVariables[0x64] = 20;
          log_alert("Script::op_cjmp() bypassing protection");
          expr = true;
        }
      }
#endif
		break;
	case 1: // jnz
		expr = (b != a);
		break;
	case 2: // jg
		expr = (b > a);
		break;
	case 3: // jge
		expr = (b >= a);
		break;
	case 4: // jl
		expr = (b < a);
		break;
	case 5: // jle
		expr = (b <= a);
		break;
	default:
		log_alert("VirtualMachine::op_cjmp() invalid condition %d", (opcode & 7));
		break;
	}

	if (expr) {
		op_jmp();
	} else {
		_bytecode.fetchWord();
	}

}

void VirtualMachine::op_setpal() {
	uint16_t paletteId = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_changePalette(%d)", paletteId);
	engine.video.paletteIdRequested = paletteId >> 8;
}

void VirtualMachine::op_reset() {

	uint8_t threadId = _bytecode.fetchByte();
	uint8_t i =        _bytecode.fetchByte();

	// FCS: WTF, this is cryptic as hell !!
	//int8_t n = (i & 0x3F) - threadId;  //0x3F = 0011 1111
	// The following is so much clearer

	//Make sure i within [0-VM_NUM_THREADS-1]
	i = i & (VM_NUM_THREADS-1) ;
	int8_t n = i - threadId;

	if (n < 0) {
		log_alert("VirtualMachine::op_reset() ec=0x%x (n < 0)", 0x880);
		return;
	}
	++n;
	uint8_t a = _bytecode.fetchByte();

	log_debug(SYS_VM, "VirtualMachine::op_reset(%d, %d, %d)", threadId, i, a);

	if (a == 2) {
		uint16_t *p = &threadsData[REQUESTED_PC_OFFSET][threadId];
		while (n--) {
			*p++ = 0xFFFE;
		}
	} else if (a < 2) {
		uint8_t *p = &vmIsChannelActive[REQUESTED_STATE][threadId];
		while (n--) {
			*p++ = a;
		}
	}
}

void VirtualMachine::op_setpage() {
	uint8_t frameBufferId = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_setpage(%d)", frameBufferId);
	engine.video.changePagePtr1(frameBufferId);
}

void VirtualMachine::op_fill() {
	uint8_t pageId = _bytecode.fetchByte();
	uint8_t color = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_fill(%d, %d)", pageId, color);
	engine.video.fillPage(pageId, color);
}

void VirtualMachine::op_copy() {
	uint8_t srcPageId = _bytecode.fetchByte();
	uint8_t dstPageId = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_copy(%d, %d)", srcPageId, dstPageId);
	engine.video.copyPage(srcPageId, dstPageId, vmVariables[VM_VARIABLE_SCROLL_Y]);
}


void VirtualMachine::op_blit() {

    uint8_t pageId = _bytecode.fetchByte();
    log_debug(SYS_VM, "VirtualMachine::op_blit(%d)", pageId);
    inp_handleSpecialKeys();

    // The bytecode will set vmVariables[VM_VARIABLE_PAUSE_SLICES] from 1 to 5
    // The virtual machine hence indicate how long the image should be displayed.

    //WTF ?
    vmVariables[0xF7] = 0;

    engine.video.updateDisplay(pageId);
}

void VirtualMachine::op_kill() {
	log_debug(SYS_VM, "VirtualMachine::op_kill()");
	_bytecode.set(engine.resources.segByteCode + 0xFFFF);
	_bytecode.yield(true);
}

void VirtualMachine::op_print() {
	uint16_t stringId = _bytecode.fetchWord();
	uint16_t x = _bytecode.fetchByte();
	uint16_t y = _bytecode.fetchByte();
	uint16_t color = _bytecode.fetchByte();

	log_debug(SYS_VM, "VirtualMachine::op_print(0x%03X, %d, %d, %d)", stringId, x, y, color);

	engine.video.drawString(color, x, y, stringId);
}

void VirtualMachine::op_subw() {
	uint8_t i = _bytecode.fetchByte();
	uint8_t j = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_subw(0x%02x, 0x%02x)", i, j);
	vmVariables[i] -= vmVariables[j];
}

void VirtualMachine::op_andi() {
	uint8_t variableId = _bytecode.fetchByte();
	uint16_t n = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_andi(0x%02x, %d)", variableId, n);
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] & n;
}

void VirtualMachine::op_iori() {
	uint8_t variableId = _bytecode.fetchByte();
	uint16_t value = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_iori(0x%02x, %d)", variableId, value);
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] | value;
}

void VirtualMachine::op_shli() {
	uint8_t variableId = _bytecode.fetchByte();
	uint16_t leftShiftValue = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_shli(0x%02x, %d)", variableId, leftShiftValue);
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] << leftShiftValue;
}

void VirtualMachine::op_shri() {
	uint8_t variableId = _bytecode.fetchByte();
	uint16_t rightShiftValue = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_shri(0x%02x, %d)", variableId, rightShiftValue);
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] >> rightShiftValue;
}

void VirtualMachine::op_sound() {
	uint16_t resourceId = _bytecode.fetchWord();
	uint8_t freq = _bytecode.fetchByte();
	uint8_t vol = _bytecode.fetchByte();
	uint8_t channel = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_sound(0x%x, %d, %d, %d)", resourceId, freq, vol, channel);
	snd_playSound(resourceId, freq, vol, channel);
}

void VirtualMachine::op_loadres() {

	uint16_t resourceId = _bytecode.fetchWord();
	log_debug(SYS_VM, "VirtualMachine::op_loadres(%d)", resourceId);

    engine.resources.loadPartOrResource(resourceId);
}

void VirtualMachine::op_music() {
	uint16_t resNum = _bytecode.fetchWord();
	uint16_t delay = _bytecode.fetchWord();
	uint8_t pos = _bytecode.fetchByte();
	log_debug(SYS_VM, "VirtualMachine::op_music(0x%x, %d, %d)", resNum, delay, pos);
	snd_playMusic(resNum, delay, pos);
}

void VirtualMachine::initForPart(uint16_t partId) {

	engine.stopMusicModule();
	engine.stopAllAudioChannels();

	//WTF is that ?
	vmVariables[0xE4] = 0x14;

	if(partId == 0x0000) {
		partId = GAME_PART1;
	}
#ifdef BYPASS_PROTECTION
	if(partId == GAME_PART1) {
		partId = GAME_PART2;
	}
#endif
	engine.resources.setupPart(partId);

	//Set all thread to inactive (pc at 0xFFFF or 0xFFFE )
	memset((uint8_t *)threadsData, 0xFF, sizeof(threadsData));


	memset((uint8_t *)vmIsChannelActive, 0, sizeof(vmIsChannelActive));

	int firstThreadId = 0;
	threadsData[PC_OFFSET][firstThreadId] = 0;
}

/*
     This is called every frames in the infinite loop.
*/
void VirtualMachine::checkThreadRequests() {

	//Check if a part switch has been requested.
	if (engine.resources.requestedPartId != 0) {
		initForPart(engine.resources.requestedPartId);
		engine.resources.requestedPartId = 0;
	}


	// Check if a state update has been requested for any thread during the previous VM execution:
	//      - Pause
	//      - Jump

	// JUMP:
	// Note: If a jump has been requested, the jump destination is stored
	// in threadsData[REQUESTED_PC_OFFSET]. Otherwise threadsData[REQUESTED_PC_OFFSET] == 0xFFFF

	// PAUSE:
	// Note: If a pause has been requested it is stored in  vmIsChannelActive[REQUESTED_STATE][i]

	for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

		vmIsChannelActive[CURR_STATE][threadId] = vmIsChannelActive[REQUESTED_STATE][threadId];

		uint16_t n = threadsData[REQUESTED_PC_OFFSET][threadId];

		if (n != VM_NO_SETVEC_REQUESTED) {

			threadsData[PC_OFFSET][threadId] = (n == 0xFFFE) ? VM_INACTIVE_THREAD : n;
			threadsData[REQUESTED_PC_OFFSET][threadId] = VM_NO_SETVEC_REQUESTED;
		}
	}
}

void VirtualMachine::schedule() {
    _currTimeStamp = engine.getTimeStamp();
    const uint32_t elapsed = _currTimeStamp - _prevTimeStamp;
    const uint32_t delay   = vmVariables[VM_VARIABLE_PAUSE_SLICES] * 20;

    if(delay > elapsed) {
        _nextTimeStamp = _currTimeStamp + delay - elapsed;
    }
    else {
        _nextTimeStamp = _currTimeStamp + 1;
    }
}

void VirtualMachine::hostFrame() {
    _currTimeStamp = engine.getTimeStamp();
    if(_currTimeStamp >= _nextTimeStamp) {
        _prevTimeStamp = _nextTimeStamp;
        if(engine.paused()) {
            _nextTimeStamp = _currTimeStamp + 100;
            return;
        }
    }
    else {
        return;
    }

	// Run the Virtual Machine for every active threads (one vm frame).
	// Inactive threads are marked with a thread instruction pointer set to 0xFFFF (VM_INACTIVE_THREAD).
	// A thread must feature a break opcode so the interpreter can move to the next thread.

	for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {
        _bytecode.yield(false);

		if (vmIsChannelActive[CURR_STATE][threadId])
			continue;

		uint16_t n = threadsData[PC_OFFSET][threadId];

		if (n != VM_INACTIVE_THREAD) {

			// Set the script pointer to the right location.
			// script pc is used in executeThread in order
			// to get the next opcode.
			_bytecode.set(engine.resources.segByteCode + n);
			_stackPtr = 0;

			log_debug(SYS_VM, "VirtualMachine::hostFrame() i=0x%02x n=0x%02x *p=0x%02x", threadId, n, *_bytecode.get());
			executeThread();

			//Since .pc is going to be modified by this next loop iteration, we need to save it.
			threadsData[PC_OFFSET][threadId] = _bytecode.get() - engine.resources.segByteCode;


			log_debug(SYS_VM, "VirtualMachine::hostFrame() i=0x%02x pos=0x%x", threadId, threadsData[PC_OFFSET][threadId]);
			if (engine.system.input.quit) {
				break;
			}
		}

	}
    checkThreadRequests();
    schedule();
}


void VirtualMachine::executeThread() {
    while(!_bytecode.yield()) {
        uint8_t opcode = _bytecode.fetchByte();
        switch(opcode) {
            case 0x00: // op_movi
                op_movi();
                break;
            case 0x01: // op_movw
                op_movw();
                break;
            case 0x02: // op_addw
                op_addw();
                break;
            case 0x03: // op_addi
                op_addi();
                break;
            case 0x04: // op_call
                op_call();
                break;
            case 0x05: // op_ret
                op_ret();
                break;
            case 0x06: // op_yield
                op_yield();
                break;
            case 0x07: // op_jmp
                op_jmp();
                break;
            case 0x08: // op_setvec
                op_setvec();
                break;
            case 0x09: // op_jnz
                op_jnz();
                break;
            case 0x0a: // op_cjmp
                op_cjmp();
                break;
            case 0x0b: // op_setpal
                op_setpal();
                break;
            case 0x0c: // op_reset
                op_reset();
                break;
            case 0x0d: // op_setpage
                op_setpage();
                break;
            case 0x0e: // op_fill
                op_fill();
                break;
            case 0x0f: // op_copy
                op_copy();
                break;
            case 0x10: // op_blit
                op_blit();
                break;
            case 0x11: // op_kill
                op_kill();
                break;
            case 0x12: // op_print
                op_print();
                break;
            case 0x13: // op_subw
                op_subw();
                break;
            case 0x14: // op_andi
                op_andi();
                break;
            case 0x15: // op_iori
                op_iori();
                break;
            case 0x16: // op_shli
                op_shli();
                break;
            case 0x17: // op_shri
                op_shri();
                break;
            case 0x18: // op_sound
                op_sound();
                break;
            case 0x19: // op_loadres
                op_loadres();
                break;
            case 0x1a: // op_music
                op_music();
                break;
            case 0x1b ... 0x3f:
                op_trap(opcode);
                break;
            case 0x40 ... 0x7f:
                op_poly1(opcode);
                break;
            case 0x80 ... 0xff:
                op_poly2(opcode);
                break;
            default:
                op_trap(opcode);
                break;
        }
    }
}

void VirtualMachine::inp_updatePlayer() {

	int16_t lr = 0;
	int16_t ud = 0;
	int16_t bt = 0;
	int16_t mask = 0;

	if (engine.system.input.dpad & PlayerInput::DIR_RIGHT) {
		lr = +1;
		mask |= 0x01;
	}
	if (engine.system.input.dpad & PlayerInput::DIR_LEFT) {
		lr = -1;
		mask |= 0x02;
	}
	if (engine.system.input.dpad & PlayerInput::DIR_DOWN) {
		ud = +1;
		mask |= 0x04;
	}
	if (engine.system.input.dpad & PlayerInput::DIR_UP) {
		ud = -1;
		mask |= 0x08;
	}
	if (engine.system.input.dpad & PlayerInput::DIR_BUTTON) {
		bt = +1;
		mask |= 0x80;
	}

	vmVariables[VM_VARIABLE_HERO_POS_LEFT_RIGHT]  = lr;
	vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN]     = ud;
	vmVariables[VM_VARIABLE_HERO_POS_JUMP_DOWN]   = ud;
	vmVariables[VM_VARIABLE_HERO_POS_MASK]        = mask;
	vmVariables[VM_VARIABLE_HERO_ACTION]          = bt;
	vmVariables[VM_VARIABLE_HERO_ACTION_POS_MASK] = mask;
}

void VirtualMachine::inp_handleSpecialKeys() {

	if (engine.system.input.code) {
		engine.system.input.code = false;
		if (engine.resources.currentPartId != GAME_PART_LAST && engine.resources.currentPartId != GAME_PART_FIRST) {
			engine.resources.requestedPartId = GAME_PART_LAST;
		}
	}

}

void VirtualMachine::snd_playSound(uint16_t resNum, uint8_t freq, uint8_t vol, uint8_t channel) {

	log_debug(SYS_VM, "snd_playSound(0x%x, %d, %d, %d)", resNum, freq, vol, channel);

	Resource *me = engine.getResource(resNum);

	if (me->state != RS_LOADED)
		return;


	if (vol == 0) {
		engine.stopAudioChannel(channel);
	} else {
        AudioSample sample;
		sample.resId   = resNum;
		sample.bufPtr  = me->data + 8; // skip header
		sample.bufLen  = Utils::fetchWordBE(me->data) * 2;
		sample.loopLen = Utils::fetchWordBE(me->data + 2) * 2;
		if (sample.loopLen != 0) {
			sample.loopPos = sample.bufLen;
		}
		assert(freq < 40);
		engine.playAudioChannel(channel, sample, frequencyTable[freq], (vol < 0x3F ? vol : 0x3F));
	}

}

void VirtualMachine::snd_playMusic(uint16_t resNum, uint16_t delay, uint8_t pos) {

	log_debug(SYS_VM, "snd_playMusic(0x%x, %d, %d)", resNum, delay, pos);

	if (resNum != 0) {
		engine.loadMusicModule(resNum, delay, pos);
		engine.playMusicModule();
	} else if (delay != 0) {
		engine.setMusicModuleDelay(delay);
	} else {
		engine.stopMusicModule();
	}
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------
