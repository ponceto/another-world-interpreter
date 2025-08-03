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

	engine->vm.hostFrame();

#ifdef __EMSCRIPTEN__
	if (engine->sys->input.quit) {
		engine = (delete engine, nullptr);
		emscripten_cancel_main_loop();
		emscripten_force_exit(EXIT_SUCCESS);
	}
#endif

}

Engine::Engine(System *paramSys, const char *dataDir, const char *dumpDir)
	: sys(paramSys), vm(&mixer, &res, &player, &video, sys), mixer(sys), res(&video, dataDir, dumpDir), 
	player(&mixer, &res, sys), video(&res, sys), _dataDir(dataDir), _dumpDir(dumpDir) {
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
