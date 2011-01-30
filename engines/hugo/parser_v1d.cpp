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

/*
 * This code is based on original Hugo Trilogy source code
 *
 * Copyright (c) 1989-1995 David P. Gray
 *
 */

// parser.c - handles all keyboard/command input

#include "common/system.h"

#include "hugo/hugo.h"
#include "hugo/parser.h"
#include "hugo/file.h"
#include "hugo/schedule.h"
#include "hugo/util.h"
#include "hugo/sound.h"
#include "hugo/object.h"
#include "hugo/text.h"

namespace Hugo {

Parser_v1d::Parser_v1d(HugoEngine *vm) : Parser(vm) {
}

Parser_v1d::~Parser_v1d() {
}

/**
* Locate word in list of nouns and return ptr to string in noun list
* If n is NULL, start at beginning of list, else with n
*/
char *Parser_v1d::findNextNoun(char *noun) {
	debugC(1, kDebugParser, "findNextNoun(%s)", noun);

	int currNounIndex = -1;
	if (noun) {                                        // If noun not NULL, find index
		for (currNounIndex = 0; _vm->_text->getNounArray(currNounIndex); currNounIndex++) {
			if (noun == _vm->_text->getNoun(currNounIndex, 0))
				break;
		}
	}
	for (int i = currNounIndex + 1; _vm->_text->getNounArray(i); i++) {
		for (int j = 0; strlen(_vm->_text->getNoun(i, j)); j++) {
			if (strstr(_vm->_line, _vm->_text->getNoun(i, j)))
				return _vm->_text->getNoun(i, 0);
		}
	}
	return 0;
}

/**
* Test whether hero is close to object.  Return TRUE or FALSE
* If no noun specified, check context flag in object before other tests.
* If object not near, return suitable string; may be similar object closer
* If radius is -1, treat radius as infinity
*/
bool Parser_v1d::isNear(char *verb, char *noun, object_t *obj, char *comment) {
	debugC(1, kDebugParser, "isNear(%s, %s, obj, %s)", verb, noun, comment);

	if (!noun && !obj->verbOnlyFl) {                // No noun specified & object not context senesitive
		return false;
	} else if (noun && (noun != _vm->_text->getNoun(obj->nounIndex, 0))) { // Noun specified & not same as object
		return false;
	} else if (obj->carriedFl) {                    // Object is being carried
		return true;
	} else if (obj->screenIndex != *_vm->_screen_p) { // Not in same screen
		if (obj->objValue)
			strcpy (comment, _vm->_text->getTextParser(kCmtAny4));
		return false;
	}

	if (obj->cycling == kCycleInvisible) {
		if (obj->seqNumb) {                         // There is an image
			strcpy(comment, _vm->_text->getTextParser(kCmtAny5));
			return false;
		} else {                                    // No image, assume visible
			if ((obj->radius < 0) ||
			   ((abs(obj->x - _vm->_hero->x) <= obj->radius) &&
			   (abs(obj->y - _vm->_hero->y - _vm->_hero->currImagePtr->y2) <= obj->radius))) {
			   return true;
			} else {
				// User is either not close enough (stationary, valueless objects)
				// or is not carrying it (small, portable objects of value)
				if (noun) {                         // Don't say unless object specified
					if (obj->objValue && (verb != _vm->_text->getVerb(_vm->_take, 0)))
						strcpy(comment, _vm->_text->getTextParser(kCmtAny4));
					else
						strcpy(comment, _vm->_text->getTextParser(kCmtClose));
					}
				return false;
			}
		}
	}

	if ((obj->radius < 0) ||
	    ((abs(obj->x - _vm->_hero->x) <= obj->radius) &&
	    (abs(obj->y + obj->currImagePtr->y2 - _vm->_hero->y - _vm->_hero->currImagePtr->y2) <= obj->radius))) {
	   return true;
	} else {
		// User is either not close enough (stationary, valueless objects)
		// or is not carrying it (small, portable objects of value)
		if (noun) {                                 // Don't say unless object specified
			if (obj->objValue && (verb != _vm->_text->getVerb(_vm->_take, 0)))
				strcpy(comment, _vm->_text->getTextParser(kCmtAny4));
			else
				strcpy(comment, _vm->_text->getTextParser(kCmtClose));
		}
		return false;
	}

	return true;
}

/**
* Test whether supplied verb is one of the common variety for this object
* say_ok needed for special case of take/drop which may be handled not only
* here but also in a cmd_list with a donestr string simultaneously
*/
bool Parser_v1d::isGenericVerb(char *word, object_t *obj) {
	debugC(1, kDebugParser, "isGenericVerb(%s, object_t *obj)", word);

	if (!obj->genericCmd)
		return false;

	// Following is equivalent to switch, but couldn't do one
	if (word == _vm->_text->getVerb(_vm->_look, 0)) {
		if ((LOOK & obj->genericCmd) == LOOK)
			Utils::Box(kBoxAny, "%s", _vm->_text->getTextData(obj->dataIndex));
		else
			Utils::Box(kBoxAny, "%s", _vm->_text->getTextParser(kTBUnusual_1d));
	} else if (word == _vm->_text->getVerb(_vm->_take, 0)) {
		if (obj->carriedFl)
			Utils::Box(kBoxAny, "%s", _vm->_text->getTextParser(kTBHave));
		else if ((TAKE & obj->genericCmd) == TAKE)
			takeObject(obj);
		else if (!obj->verbOnlyFl)                  // Make sure not taking object in context!
			Utils::Box(kBoxAny, "%s", _vm->_text->getTextParser(kTBNoUse));
		else
			return false;
	} else if (word == _vm->_text->getVerb(_vm->_drop, 0)) {
		if (!obj->carriedFl)
			Utils::Box(kBoxAny, "%s", _vm->_text->getTextParser(kTBDontHave));
		else if ((DROP & obj->genericCmd) == DROP)
			dropObject(obj);
		else
			Utils::Box(kBoxAny, "%s", _vm->_text->getTextParser(kTBNeed));
	} else {                                        // It was not a generic cmd
		return false;
	}

	return true;
}

/**
* Test whether supplied verb is included in the list of allowed verbs for
* this object.  If it is, then perform the tests on it from the cmd list
* and if it passes, perform the actions in the action list.  If the verb
* is catered for, return TRUE
*/
bool Parser_v1d::isObjectVerb(char *word, object_t *obj) {
	debugC(1, kDebugParser, "isObjectVerb(%s, object_t *obj)", word);

	// First, find matching verb in cmd list
	uint16 cmdIndex = obj->cmdIndex;                // ptr to list of commands
	if (!cmdIndex)                                  // No commands for this obj
		return false;

	int i;
	for (i = 0; _vm->_cmdList[cmdIndex][i].verbIndex != 0; i++) { // For each cmd
		if (!strcmp(word, _vm->_text->getVerb(_vm->_cmdList[cmdIndex][i].verbIndex, 0))) // Is this verb catered for?
			break;
	}

	if (_vm->_cmdList[cmdIndex][i].verbIndex == 0)  // No
		return false;

	// Verb match found, check all required objects are being carried
	cmd *cmnd = &_vm->_cmdList[cmdIndex][i];        // ptr to struct cmd
	if (cmnd->reqIndex) {                           // At least 1 thing in list
		uint16 *reqs = _vm->_arrayReqs[cmnd->reqIndex]; // ptr to list of required objects
		for (i = 0; reqs[i]; i++) {                 // for each obj
			if (!_vm->_object->isCarrying(reqs[i])) {
				Utils::Box(kBoxAny, "%s", _vm->_text->getTextData(cmnd->textDataNoCarryIndex));
				return true;
			}
		}
	}

	// Required objects are present, now check state is correct
	if ((obj->state != cmnd->reqState) && (cmnd->reqState != kStateDontCare)){
		Utils::Box(kBoxAny, "%s", _vm->_text->getTextData(cmnd->textDataWrongIndex));
		return true;
	}

	// Everything checked.  Change the state and carry out any actions
	if (cmnd->reqState != kStateDontCare)           // Don't change new state if required state didn't care
		obj->state = cmnd->newState;
	Utils::Box(kBoxAny, "%s", _vm->_text->getTextData(cmnd->textDataDoneIndex));
	_vm->_scheduler->insertActionList(cmnd->actIndex);
	// Special case if verb is Take or Drop.  Assume additional generic actions
	if ((word == _vm->_text->getVerb(_vm->_take, 0)) || (word == _vm->_text->getVerb(_vm->_drop, 0)))
		isGenericVerb(word, obj);
	return true;
}

/**
* Print text for possible background object.  Return TRUE if match found
* Only match if both verb and noun found.  Test_ca will match verb-only
*/
bool Parser_v1d::isBackgroundWord(char *noun, char *verb, objectList_t obj) {
	debugC(1, kDebugParser, "isBackgroundWord(%s, %s, object_list_t obj)", noun, verb);

	if (!noun)
		return false;

	for (int i = 0; obj[i].verbIndex; i++) {
		if ((verb == _vm->_text->getVerb(obj[i].verbIndex, 0)) && (noun == _vm->_text->getNoun(obj[i].nounIndex, 0))) {
			Utils::Box(kBoxAny, "%s", _vm->_file->fetchString(obj[i].commentIndex));
			return true;
		}
	}
	return false;
}

/**
* Do all things necessary to carry an object
*/
void Parser_v1d::takeObject(object_t *obj) {
	debugC(1, kDebugParser, "takeObject(object_t *obj)");

	obj->carriedFl = true;
	if (obj->seqNumb)                               // Don't change if no image to display
		obj->cycling = kCycleAlmostInvisible;

	_vm->adjustScore(obj->objValue);

	Utils::Box(kBoxAny, TAKE_TEXT, _vm->_text->getNoun(obj->nounIndex, TAKE_NAME));
}

/**
* Do all necessary things to drop an object
*/
void Parser_v1d::dropObject(object_t *obj) {
	debugC(1, kDebugParser, "dropObject(object_t *obj)");

	obj->carriedFl = false;
	obj->screenIndex = *_vm->_screen_p;
	if (obj->seqNumb)                               // Don't change if no image to display
		obj->cycling = kCycleNotCycling;
	obj->x = _vm->_hero->x - 1;
	obj->y = _vm->_hero->y + _vm->_hero->currImagePtr->y2 - 1;
	_vm->adjustScore(-obj->objValue);
	Utils::Box(kBoxAny, "%s", _vm->_text->getTextParser(kTBOk));
}

/**
* Print text for possible background object.  Return TRUE if match found
* If test_noun TRUE, must have a noun given
*/
bool Parser_v1d::isCatchallVerb(bool testNounFl, char *noun, char *verb, objectList_t obj) {
	debugC(1, kDebugParser, "isCatchallVerb(%d, %s, %s, object_list_t obj)", (testNounFl) ? 1 : 0, noun, verb);

	if (_maze.enabledFl)
		return false;

	if (testNounFl && !noun)
		return false;

	for (int i = 0; obj[i].verbIndex; i++) {
		if ((verb == _vm->_text->getVerb(obj[i].verbIndex, 0)) && ((noun == _vm->_text->getNoun(obj[i].nounIndex, 0)) || (obj[i].nounIndex == 0))) {
			Utils::Box(kBoxAny, "%s", _vm->_file->fetchString(obj[i].commentIndex));
			return true;
		}
	}
	return false;
}

/**
* Parse the user's line of text input.  Generate events as necessary
*/
void Parser_v1d::lineHandler() {
	debugC(1, kDebugParser, "lineHandler()");

	status_t &gameStatus = _vm->getGameStatus();

	// Toggle God Mode
	if (!strncmp(_vm->_line, "PPG", 3)) {
		_vm->_sound->playSound(!_vm->_soundTest, kSoundPriorityHigh);
		gameStatus.godModeFl = !gameStatus.godModeFl;
		return;
	}

	Utils::strlwr(_vm->_line);                      // Convert to lower case

	// God Mode cheat commands:
	// goto <screen>                                Takes hero to named screen
	// fetch <object name>                          Hero carries named object
	// fetch all                                    Hero carries all possible objects
	// find <object name>                           Takes hero to screen containing named object
	if (gameStatus.godModeFl) {
		// Special code to allow me to go straight to any screen
		if (strstr(_vm->_line, "goto")) {
			for (int i = 0; i < _vm->_numScreens; i++) {
				if (!scumm_stricmp(&_vm->_line[strlen("goto") + 1], _vm->_text->getScreenNames(i))) {
					_vm->_scheduler->newScreen(i);
					return;
				}
			}
		}

		// Special code to allow me to get objects from anywhere
		if (strstr(_vm->_line, "fetch all")) {
			for (int i = 0; i < _vm->_object->_numObj; i++) {
				if (_vm->_object->_objects[i].genericCmd & TAKE)
					takeObject(&_vm->_object->_objects[i]);
			}
			return;
		}

		if (strstr(_vm->_line, "fetch")) {
			for (int i = 0; i < _vm->_object->_numObj; i++) {
				if (!scumm_stricmp(&_vm->_line[strlen("fetch") + 1], _vm->_text->getNoun(_vm->_object->_objects[i].nounIndex, 0))) {
					takeObject(&_vm->_object->_objects[i]);
					return;
				}
			}
		}

		// Special code to allow me to goto objects
		if (strstr(_vm->_line, "find")) {
			for (int i = 0; i < _vm->_object->_numObj; i++) {
				if (!scumm_stricmp(&_vm->_line[strlen("find") + 1], _vm->_text->getNoun(_vm->_object->_objects[i].nounIndex, 0))) {
					_vm->_scheduler->newScreen(_vm->_object->_objects[i].screenIndex);
					return;
				}
			}
		}
	}

	if (!strcmp("exit", _vm->_line) || strstr(_vm->_line, "quit")) {
		if (Utils::Box(kBoxYesNo, "%s", _vm->_text->getTextParser(kTBExit_1d)) != 0)
			_vm->endGame();
		return;
	}

	// SAVE/RESTORE
	if (!strcmp("save", _vm->_line)) {
		if (gameStatus.gameOverFl)
			Utils::gameOverMsg();
		else
			_vm->_file->saveGame(-1, Common::String());
		return;
	}

	if (!strcmp("restore", _vm->_line)) {
		_vm->_file->restoreGame(-1);
		_vm->_scheduler->restoreScreen(*_vm->_screen_p);
		gameStatus.viewState = kViewPlay;
		return;
	}

	if (*_vm->_line == '\0')                        // Empty line
		return;

	if (strspn(_vm->_line, " ") == strlen(_vm->_line)) // Nothing but spaces!
		return;

	if (gameStatus.gameOverFl) {                    // No commands allowed!
		Utils::gameOverMsg();
		return;
	}

	// Find the first verb in the line
	char *verb = findVerb();
	char *noun = 0;                                 // Noun not found yet
	char farComment[kCompLineSize * 5] = "";        // hold 5 line comment if object not nearby

	if (verb) {                                     // OK, verb found.  Try to match with object
		do {
			noun = findNextNoun(noun);              // Find a noun in the line
			// Must try at least once for objects allowing verb-context
			for (int i = 0; i < _vm->_object->_numObj; i++) {
				object_t *obj = &_vm->_object->_objects[i];
				if (isNear(verb, noun, obj, farComment)) {
					if (isObjectVerb(verb, obj)     // Foreground object
					 || isGenericVerb(verb, obj))   // Common action type
						return;
				}
			}
			if ((*farComment == '\0') && isBackgroundWord(noun, verb, _vm->_backgroundObjects[*_vm->_screen_p]))
				return;
		} while (noun);
	}
	noun = findNextNoun(noun);
	if (*farComment != '\0')                        // An object matched but not near enough
		Utils::Box(kBoxAny, "%s", farComment);
	else if (!isCatchallVerb(true, noun, verb, _vm->_catchallList) &&
		     !isCatchallVerb(false, noun, verb, _vm->_backgroundObjects[*_vm->_screen_p])  &&
		     !isCatchallVerb(false, noun, verb, _vm->_catchallList))
		Utils::Box(kBoxAny, "%s", _vm->_text->getTextParser(kTBEh_1d));
}

void Parser_v1d::showInventory() {
	status_t &gameStatus = _vm->getGameStatus();
	if (gameStatus.viewState == kViewPlay) {
		if (gameStatus.gameOverFl)
			Utils::gameOverMsg();
		else
			showDosInventory();
	}
}
} // End of namespace Hugo
