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

#ifndef KYRA_SCREEN_LOL_H
#define KYRA_SCREEN_LOL_H

#include "kyra/screen_v2.h"

namespace Kyra {

class LoLEngine;

class Screen_LoL : public Screen_v2 {
public:
	Screen_LoL(LoLEngine *vm, OSystem *system);
	
	void setScreenDim(int dim);
	const ScreenDim *getScreenDim(int dim);

	void fprintString(const char *format, int x, int y, uint8 col1, uint8 col2, uint16 flags, ...);
	void fprintStringIntro(const char *format, int x, int y, uint8 c1, uint8 c2, uint8 c3, uint16 flags, ...);
private:
	LoLEngine *_vm;

	static const ScreenDim _screenDimTable[];
	static const int _screenDimTableCount;
};

} // end of namespace Kyra

#endif

