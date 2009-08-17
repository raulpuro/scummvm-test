/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef DRACI_GAME_H
#define DRACI_GAME_H

#include "common/str.h"
#include "draci/barchive.h"
#include "draci/script.h"
#include "draci/animation.h"
#include "draci/sprite.h"

namespace Draci {

class DraciEngine;

enum {
	kDragonObject = 0
};

enum StructSizes {
	personSize = sizeof(uint16) * 2 + sizeof(byte)
};


// Used as a return value for Game::getObjectWithAnimation() if no object
// owns the animation in question
enum {
	kObjectNotFound = -1
};

// Used as the value of the _escRoom field of the current room if there is 
// no escape room defined
enum {
	kNoEscRoom = -1
};

// Used as a value to Game::_currentIcon and means there is no item selected
// and a "real" cursor image is used
enum {
	kNoItem = -1
};

// Used as a default parameter in Game::loadWalkingMap() to specify that the default
// walking map to the room is to be loaded.
enum {
	kDefaultRoomMap = -1
};

enum {
	kNoDialogue = -1, kDialogueLines = 4
};

enum {
	kBlackPalette = -1
};

enum SpeechConstants {
	kBaseSpeechDuration = 200,
	kSpeechTimeUnit = 400
};

/** Inventory related magical constants */
enum InventoryConstants {
  kInventoryItemWidth = 25,
  kInventoryItemHeight = 25,
  kInventoryColumns = 7,
  kInventoryLines = 5,
  kInventoryX = 70, //!< Used for positioning of the inventory sprite on the X axis
  kInventoryY = 30, //!< Used for positioning of the inventory sprite on the Y axis
  kInventorySlots = kInventoryLines * kInventoryColumns
};

class WalkingMap {

public:	
	WalkingMap() {
		_realWidth = 0;
		_realHeight = 0;
		_mapWidth = 0;
		_mapHeight = 0;
		_byteWidth = 0;
		_data = NULL;
	}	

	void load(byte *data, uint length) {
		Common::MemoryReadStream mapReader(data, length);

		_realWidth = mapReader.readUint16LE();
		_realHeight = mapReader.readUint16LE();
		_deltaX = mapReader.readUint16LE();
		_deltaY = mapReader.readUint16LE();
		_mapWidth = mapReader.readUint16LE();
		_mapHeight = mapReader.readUint16LE();
		_byteWidth = mapReader.readUint16LE();

		// Set the data pointer to raw map data
		_data = data + mapReader.pos();
	}

	bool isWalkable(int x, int y);
	Common::Point findNearestWalkable(int x, int y, Common::Rect searchRect);

private:
	int _realWidth, _realHeight;
	int _deltaX, _deltaY;
	int _mapWidth, _mapHeight;
	int _byteWidth;
	byte *_data;
};

struct GameObject {
	
	uint _init, _look, _use, _canUse;
	bool _imInit, _imLook, _imUse;
	int _walkDir;
	byte _z;
	uint _lookX, _lookY, _useX, _useY;
	int _lookDir, _useDir;
	uint _absNum;
	Common::Array<int> _anims;
	GPL2Program _program;
	Common::String _title;
	int _location;
	bool _visible;
};

struct GameInfo {
	int _startRoom;
	int _mapRoom;
	uint _numObjects;
	uint _numItems;
	byte _numVariables;
	byte _numPersons;
	byte _numDialogues;
	uint _maxItemWidth, _maxItemHeight;
	uint _musicLength;
	uint _crc[4];
	uint _numDialogueBlocks;
};

struct GameItem {
	uint _init, _look, _use, _canUse;
	bool _imInit, _imLook, _imUse;
	GPL2Program _program;
	Common::String _title;
};

struct Person {
	uint _x, _y;
	byte _fontColour;
};

struct Dialogue {
	int _canLen;
	byte *_canBlock;
	Common::String _title;
	GPL2Program _program;
};

struct Room {
	int _roomNum;	
	byte _music;
	WalkingMap _walkingMap;
	byte _palette;
	int _numOverlays;
	int _init, _look, _use, _canUse;
	bool _imInit, _imLook, _imUse;
	bool _mouseOn, _heroOn;
	double _pers0, _persStep;
	int _escRoom;
	byte _numGates;
	Common::Array<int> _gates;
	GPL2Program _program;
};

enum LoopStatus {
	kStatusOrdinary, 
	kStatusGate, 
	kStatusInventory, 
	kStatusDialogue
};

enum LoopSubstatus {
	kSubstatusOrdinary, 
	kSubstatusTalk, 
	kSubstatusFade,
	kSubstatusStrange
};

/**
  * Enumerates the animations for the dragon's movement.
  */

enum Movement {
	kMoveUndefined, kMoveDown, kMoveUp, kMoveRight, kMoveLeft,
	kMoveRightDown, kMoveRightUp, kMoveLeftDown, kMoveLeftUp,
	kMoveDownRight, kMoveUpRight, kMoveDownLeft, kMoveUpLeft,
	kMoveLeftRight, kMoveRightLeft, kMoveUpStopLeft, kMoveUpStopRight,
	kSpeakRight, kSpeakLeft, kStopRight, kStopLeft
};

class Game {

public:

	Game(DraciEngine *vm);
	~Game();

	void init();
	void start();
	void loop();

	void changeRoom(uint roomNum);

	// HACK: this is only for testing
	int nextRoomNum() { 
		int n = _currentRoom._roomNum;
		n = n < 37 ? n+1 : n;

		// disable former distributor logo
		if (n == 30)
			++n;

		return n;
	}

	// HACK: same as above
	int prevRoomNum() { 
		int n = _currentRoom._roomNum;
		n = n > 0 ? n-1 : n;

		// disable former distributor logo
		if (n == 30)
			--n;

		return n;
	}

	void walkHero(int x, int y);

	void loadRoom(int roomNum);
	int loadAnimation(uint animNum, uint z);
	void loadOverlays();
	void loadObject(uint numObj);
	void loadWalkingMap(int mapID = kDefaultRoomMap);
	void loadItem(int itemID);

	uint getNumObjects();
	GameObject *getObject(uint objNum);
	int getObjectWithAnimation(int animID);

	int getVariable(int varNum);
	void setVariable(int varNum, int value);	

	Person *getPerson(int personID);

	int getRoomNum();
	void setRoomNum(int room);

	int getGateNum();
	void setGateNum(int gate);

	int getItemStatus(int itemID);
	void setItemStatus(int itemID, int status);
	int getCurrentItem();
	void setCurrentItem(int itemID);
	void removeItem(int itemID);
	void putItem(int itemID, int position);
	void addItem(int itemID);

	int getEscRoom();

	int getMarkedAnimationIndex();
	void setMarkedAnimationIndex(int index);

	void setLoopStatus(LoopStatus status);
	void setLoopSubstatus(LoopSubstatus status);
	LoopStatus getLoopStatus();
	LoopSubstatus getLoopSubstatus();

	bool shouldQuit() { return _shouldQuit; }
	void setQuit(bool quit) { _shouldQuit = quit; }

	bool shouldExitLoop() { return _shouldExitLoop; }
	void setExitLoop(bool exit) { _shouldExitLoop = exit; }

	void runGateProgram(int gate);

	void setSpeechTick(uint tick);

	void updateTitle();
	void updateCursor();

	void inventoryInit();
	void inventoryDraw();
	void inventoryDone();

	void dialogueMenu(int dialogueID);
	int dialogueDraw();
	void dialogueInit(int dialogID);
	void dialogueDone();
	void runDialogueProg(GPL2Program, int offset);

	void schedulePalette(int paletteID);
	int getScheduledPalette();

private:
	DraciEngine *_vm;

	GameInfo _info;

	int *_variables;
	Person *_persons;
	GameObject *_objects;

	byte *_itemStatus;
	GameItem *_items;
	int _currentItem;
	int _itemUnderCursor;

	int _inventory[kInventorySlots];
	bool _inventoryExit;


	Room _currentRoom;
	int _currentGate;	
	int _newRoom;
	int _newGate;

// HACK: remove public when tested and add getters instead
public:
	uint *_dialogueOffsets;
	int _currentDialogue;
	int *_dialogueVars;
	BArchive *_dialogueArchive;
	Dialogue *_dialogueBlocks;
	bool _dialogueBegin;
	bool _dialogueExit;
	int _currentBlock;
	int _lastBlock;
	int _dialogueLines;
	int _blockNum;
	int _lines[kDialogueLines];
	Animation *_dialogueAnims[kDialogueLines];
	
	LoopStatus _loopStatus;
	LoopSubstatus _loopSubstatus;

	bool _shouldQuit;
	bool _shouldExitLoop;

	uint _speechTick;

	int _objUnderCursor;
	int _oldObjUnderCursor;	
	int _animUnderCursor;

	int _markedAnimationIndex; //!< Used by the Mark GPL command

	int _scheduledPalette;
};

} // End of namespace Draci

#endif // DRACI_GAME_H
