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

#include "common/endian.h"

#include "gob/gob.h"
#include "gob/scnplayer.h"
#include "gob/global.h"
#include "gob/util.h"
#include "gob/draw.h"
#include "gob/inter.h"
#include "gob/videoplayer.h"

namespace Gob {

SCNPlayer::SCNPlayer(GobEngine *vm) : DemoPlayer(vm) {
	_doubleMode = false;
}

SCNPlayer::~SCNPlayer() {
}

bool SCNPlayer::play(const char *fileName) {
	debugC(1, kDebugDemo, "Playing SCN \"%s\"", fileName);

	init();

	Common::File scn;

	if (!scn.open(fileName))
		return false;

	return play(scn);
}

bool SCNPlayer::play(Common::File &scn) {
	// Read labels
	LabelMap labels;
	if (!readLabels(scn, labels))
		return false;

	// Iterate over all lines
	while (!scn.err() && !scn.eos()) {
		Common::String line = scn.readLine();

		// Interpret
		if (line == "CLEAR") {
			clearScreen();
		} else if (lineStartsWith(line, "VIDEO:")) {
			evaluateVideoMode(line.c_str() + 6);
		} else if (lineStartsWith(line, "IMD_PRELOAD ")) {
			playVideo(line.c_str() + 12);
		} else if (lineStartsWith(line, "IMD ")) {
			playVideo(line.c_str() + 4);
		} else if (lineStartsWith(line, "GOTO ")) {
			gotoLabel(scn, labels, line.c_str() + 5);
		}

		// Mind user input
		_vm->_util->processInput();
		if (_vm->shouldQuit())
			return true;
	}

	if (scn.err())
		return false;

	return true;
}

bool SCNPlayer::readLabels(Common::File &scn, LabelMap &labels) {
	debugC(1, kDebugDemo, "Reading SCN labels");

	int32 startPos = scn.pos();

	// Iterate over all lines
	while (!scn.err() && !scn.eos()) {
		Common::String line = scn.readLine();

		if (lineStartsWith(line, "LABEL ")) {
			// Label => Add to the hashmap
			labels.setVal(line.c_str() + 6, scn.pos());
			debugC(2, kDebugDemo, "Found label \"%s\" (%d)", line.c_str() + 6, scn.pos());
		}
	}

	if (scn.err())
		return false;

	// Seek back
	if (!scn.seek(startPos))
		return false;

	return true;
}

void SCNPlayer::gotoLabel(Common::File &scn, const LabelMap &labels, const char *label) {
	debugC(2, kDebugDemo, "Jumping to label \"%s\"", label);

	if (!labels.contains(label))
		return;

	scn.seek(labels.getVal(label));
}

} // End of namespace Gob
