/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "engine.h"
#include "file.h"
#include "serializer.h"
#include "sys.h"
#include "parts.h"

static constexpr uint32_t AWSV = (static_cast<uint32_t>('A') << 24)
		                       | (static_cast<uint32_t>('W') << 16)
		                       | (static_cast<uint32_t>('S') <<  8)
		                       | (static_cast<uint32_t>('V') <<  0)
		                       ;

static void engineMainLoop(Engine* engine) {

	engine->vm.checkThreadRequests();

	engine->vm.inp_updatePlayer();

	engine->processInput();

	engine->vm.hostFrame();

#ifdef __EMSCRIPTEN__
	if (engine->sys->input.quit) {
		engine = (delete engine, nullptr);
		emscripten_cancel_main_loop();
		emscripten_force_exit(EXIT_SUCCESS);
	}
#endif

}

Engine::Engine(System *paramSys, const char *dataDir, const char *saveDir, const char *dumpDir)
	: sys(paramSys), vm(&mixer, &res, &player, &video, sys), mixer(sys), res(&video, dataDir, dumpDir), 
	player(&mixer, &res, sys), video(&res, sys), _dataDir(dataDir), _saveDir(saveDir), _dumpDir(dumpDir), _stateSlot(0) {
	init();
}

void Engine::run() {

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(reinterpret_cast<em_arg_callback_func>(&engineMainLoop), this, 0, 1);
#else
	while (!sys->input.quit) {

		engineMainLoop(this);

	}
#endif


}

Engine::~Engine(){

	finish();
}


void Engine::init() {


	//Init system
	sys->init("Out Of This World");

	video.init();

	res.allocMemBlock();

	res.readEntries();

	vm.init();

	mixer.init();

	player.init();

	vm.initForPart(GAME_PART1);
}

void Engine::finish() {
	player.free();
	mixer.free();
	res.freeMemBlock();
	sys->destroy();
}

void Engine::processInput() {
	if (sys->input.load) {
		loadGameState(_stateSlot);
		sys->input.load = false;
	}
	if (sys->input.save) {
		saveGameState(_stateSlot, "quicksave");
		sys->input.save = false;
	}
	if (sys->input.stateSlot != 0) {
		int8_t slot = _stateSlot + sys->input.stateSlot;
		if (slot >= 0 && slot < MAX_SAVE_SLOTS) {
			_stateSlot = slot;
			debug(DBG_INFO, "Current game state slot is %d", _stateSlot);
		}
		sys->input.stateSlot = 0;
	}
}

void Engine::makeGameStateName(uint8_t slot, char *buf) {
	sprintf(buf, "raw.s%02d", slot);
}

void Engine::saveGameState(uint8_t slot, const char *desc) {
	char stateFile[20];
	makeGameStateName(slot, stateFile);
	File f(true);
	if (!f.open(stateFile, _saveDir, "wb")) {
		warning("Unable to save state file '%s'", stateFile);
	} else {
		// header
		f.writeUint32BE(AWSV);
		f.writeUint16BE(Serializer::CUR_VER);
		f.writeUint16BE(0);
		char hdrdesc[32];
		strncpy(hdrdesc, desc, sizeof(hdrdesc) - 1);
		f.write(hdrdesc, sizeof(hdrdesc));
		// contents
		Serializer s(&f, Serializer::SM_SAVE, res._memPtrStart);
		vm.saveOrLoad(s);
		res.saveOrLoad(s);
		video.saveOrLoad(s);
		player.saveOrLoad(s);
		mixer.saveOrLoad(s);
		if (f.ioErr()) {
			warning("I/O error when saving game state");
		} else {
			debug(DBG_INFO, "Saved state to slot %d", _stateSlot);
		}
	}
}

void Engine::loadGameState(uint8_t slot) {
	char stateFile[20];
	makeGameStateName(slot, stateFile);
	File f(true);
	if (!f.open(stateFile, _saveDir, "rb")) {
		warning("Unable to open state file '%s'", stateFile);
	} else {
		uint32_t id = f.readUint32BE();
		if (id != AWSV) {
			warning("Bad savegame format");
		} else {
			// mute
			player.stop();
			mixer.stopAll();
			// header
			uint16_t ver = f.readUint16BE();
			f.readUint16BE();
			char hdrdesc[32];
			f.read(hdrdesc, sizeof(hdrdesc));
			// contents
			Serializer s(&f, Serializer::SM_LOAD, res._memPtrStart, ver);
			vm.saveOrLoad(s);
			res.saveOrLoad(s);
			video.saveOrLoad(s);
			player.saveOrLoad(s);
			mixer.saveOrLoad(s);
		}
		if (f.ioErr()) {
			warning("I/O error when loading game state");
		} else {
			debug(DBG_INFO, "Loaded state from slot %d", _stateSlot);
		}
	}
}
