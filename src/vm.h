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
// XXX
// ---------------------------------------------------------------------------

#define VM_NUM_THREADS 64
#define VM_NUM_VARIABLES 256
#define VM_NO_SETVEC_REQUESTED 0xFFFF
#define VM_INACTIVE_THREAD    0xFFFF

enum ScriptVars {
    VM_VARIABLE_RANDOM_SEED          = 0x3C,
    VM_VARIABLE_LAST_KEYCHAR         = 0xDA,
    VM_VARIABLE_HERO_POS_UP_DOWN     = 0xE5,
    VM_VARIABLE_MUS_MARK             = 0xF4,
    VM_VARIABLE_SCROLL_Y             = 0xF9, // = 239
    VM_VARIABLE_HERO_ACTION          = 0xFA,
    VM_VARIABLE_HERO_POS_JUMP_DOWN   = 0xFB,
    VM_VARIABLE_HERO_POS_LEFT_RIGHT  = 0xFC,
    VM_VARIABLE_HERO_POS_MASK        = 0xFD,
    VM_VARIABLE_HERO_ACTION_POS_MASK = 0xFE,
    VM_VARIABLE_PAUSE_SLICES         = 0xFF
};

//For threadsData navigation
#define PC_OFFSET 0
#define REQUESTED_PC_OFFSET 1
#define NUM_DATA_FIELDS 2

//For vmIsChannelActive navigation
#define CURR_STATE 0
#define REQUESTED_STATE 1
#define NUM_THREAD_FIELDS 2

class VirtualMachine
{
public:
    //This table is used to play a sound
    static const uint16_t frequencyTable[];

    Engine& engine;
    uint32_t _currTimeStamp;
    uint32_t _nextTimeStamp;
    uint32_t _prevTimeStamp;
    ByteCode _bytecode;


    int16_t vmVariables[VM_NUM_VARIABLES];
    uint16_t _scriptStackCalls[VM_NUM_THREADS];
    uint16_t threadsData[NUM_DATA_FIELDS][VM_NUM_THREADS];

    // This array is used:
    //     0 to save the channel's instruction pointer
    //     when the channel release control (this happens on a break).
    //     1 When a setVec is requested for the next vm frame.
    uint8_t vmIsChannelActive[NUM_THREAD_FIELDS][VM_NUM_THREADS];

    uint8_t _stackPtr;

    VirtualMachine(Engine& engine);
    void init();
    void fini();

    auto getNextTimeStamp() -> uint32_t
    {
        return _nextTimeStamp;
    }

    void op_movi();
    void op_movw();
    void op_addw();
    void op_addi();
    void op_call();
    void op_ret();
    void op_yield();
    void op_jmp();
    void op_setvec();
    void op_jnz();
    void op_cjmp();
    void op_setpal();
    void op_reset();
    void op_setpage();
    void op_fill();
    void op_copy();
    void op_blit();
    void op_kill();
    void op_print();
    void op_subw();
    void op_andi();
    void op_iori();
    void op_shli();
    void op_shri();
    void op_sound();
    void op_loadres();
    void op_music();
    void op_trap(uint8_t opcode);
    void op_poly1(uint8_t opcode);
    void op_poly2(uint8_t opcode);

    void initForPart(uint16_t partId);
    void checkThreadRequests();
    void schedule();
    void hostFrame();
    void executeThread();

    void inp_updatePlayer();
    void inp_handleSpecialKeys();

    void snd_playSound(uint16_t resNum, uint8_t freq, uint8_t vol, uint8_t channel);
    void snd_playMusic(uint16_t resNum, uint16_t delay, uint8_t pos);
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_VIRTUALMACHINE_H__ */
