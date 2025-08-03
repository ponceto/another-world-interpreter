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

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "intern.h"


#define MEMENTRY_STATE_END_OF_MEMLIST 0xFF
#define MEMENTRY_STATE_NOT_NEEDED 0 
#define MEMENTRY_STATE_LOADED 1
#define MEMENTRY_STATE_LOAD_ME 2

/*
    This is a directory entry. When the game starts, it loads memlist.bin and 
	populate and array of MemEntry
*/
struct MemEntry {
	uint8_t state;         // 0x0
	uint8_t type;          // 0x1, Resource::ResType
	uint8_t *bufPtr;       // 0x2
	uint16_t unk4;         // 0x4, unused
	uint8_t rankNum;       // 0x6
	uint8_t bankId;       // 0x7
	uint32_t bankOffset;      // 0x8 0xA
	uint16_t unkC;         // 0xC, unused
	uint16_t packedSize;   // 0xE
	                     // All ressources are packed (for a gain of 28% according to Chahi)

	uint16_t unk10;        // 0x10, unused
	uint16_t size; // 0x12
};
/*
     Note: state is not a boolean, it can have value 0, 1, 2 or 255, respectively meaning:
      0:NOT_NEEDED
      1:LOADED
      2:LOAD_ME
      255:END_OF_MEMLIST

    See MEMENTRY_STATE_* #defines above.
*/

struct Video;

struct Resource {

	enum ResType {
		RT_SOUND  = 0,
		RT_MUSIC  = 1,
		RT_BITMAP = 2, // full screen video buffer, size=0x7D00 

		// FCS: 0x7D00=32000...but 320x200 = 64000 ??
		// Since the game is 16 colors, two pixels palette indices can be stored in one byte
		// that's why we can store two pixels palette indice in one byte and we only need 320*200/2 bytes for
		// an entire screen.

		RT_PALETTE  = 3, // palette (1024=vga + 1024=ega), size=2048
		RT_BYTECODE = 4,
		RT_POLYGON  = 5
	};
	
	enum {
		MEM_BLOCK_SIZE = 600 * 1024   //600kb total memory consumed (not taking into account stack and static heap)
	};
	
	
	Video *video;
	const char *_dataDir;
	const char *_dumpDir;
	MemEntry _memList[150];
	uint16_t _numMemList;
	uint16_t currentPartId;
	uint16_t requestedNextPart;
	uint8_t *_memPtrStart;
	uint8_t *_scriptBakPtr;
	uint8_t *_scriptCurPtr;
	uint8_t *_vidBakPtr;
	uint8_t *_vidCurPtr;
	bool _useSegVideo2;
	uint8_t *segPalettes;
	uint8_t *segBytecode;
	uint8_t *segCinematic;
	uint8_t *_segVideo2;

	Resource(Video *vid, const char *dataDir, const char *dumpDir);
	
	void readBank(const MemEntry *me, uint8_t *dstBuf);
	void readEntries();
	void loadMarkedAsNeeded();
	void invalidateAll();
	void invalidateRes();	
	void loadPartsOrMemoryEntry(uint16_t num);
	void dumpMemList();
	void dumpMemEntry(uint16_t index, const MemEntry* entry);
	void setupPart(uint16_t ptrId);
	void allocMemBlock();
	void freeMemBlock();
};

#endif
