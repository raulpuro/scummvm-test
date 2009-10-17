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

#ifndef SCI_GUI_TRANSITIONS_H
#define SCI_GUI_TRANSITIONS_H

#include "sci/gui/gui_helpers.h"

namespace Sci {

struct GuiTransitionTranslateEntry {
	int16 orgId;
	int16 newId;
	bool blackoutFlag;
};

enum {
	SCI_TRANSITIONS_VERTICALROLL_FROMCENTER		= 0,
	SCI_TRANSITIONS_HORIZONTALROLL_FROMCENTER	= 1,
	SCI_TRANSITIONS_STRAIGHT_FROM_RIGHT			= 2,
	SCI_TRANSITIONS_STRAIGHT_FROM_LEFT			= 3,
	SCI_TRANSITIONS_STRAIGHT_FROM_BOTTOM		= 4,
	SCI_TRANSITIONS_STRAIGHT_FROM_TOP			= 5,
	SCI_TRANSITIONS_DIAGONALROLL_FROMCENTER		= 6,
	SCI_TRANSITIONS_DIAGONALROLL_TOCENTER		= 7,
	SCI_TRANSITIONS_BLOCKS						= 8,
	SCI_TRANSITIONS_PIXELATION					= 9,
	SCI_TRANSITIONS_FADEPALETTE					= 10,
	SCI_TRANSITIONS_SCROLL_RIGHT				= 11,
	SCI_TRANSITIONS_SCROLL_LEFT					= 12,
	SCI_TRANSITIONS_SCROLL_UP					= 13,
	SCI_TRANSITIONS_SCROLL_DOWN					= 14,
	SCI_TRANSITIONS_NONE_LONGBOW				= 15,
	SCI_TRANSITIONS_NONE						= 100,
	// here are transitions that are used by the old tableset, but are not included anymore in the new tableset
	SCI_TRANSITIONS_VERTICALROLL_TOCENTER		= 300,
	SCI_TRANSITIONS_HORIZONTALROLL_TOCENTER		= 301
};

class SciGuiScreen;
class SciGuiTransitions {
public:
	SciGuiTransitions(SciGui *gui, SciGuiScreen *screen, SciGuiPalette *palette, bool isVGA);
	~SciGuiTransitions();

	void setup(int16 number, bool blackoutFlag);
	void doit(Common::Rect picRect);

private:
	void init(void);
	const GuiTransitionTranslateEntry *translateNumber(int16 number, const GuiTransitionTranslateEntry *tablePtr);
	void doTransition(int16 number, bool blackout);
	void setNewPalette(bool blackoutFlag);
	void setNewScreen(bool blackoutFlag);
	void copyRectToScreen(const Common::Rect rect, bool blackoutFlag);
	void fadeOut();
	void fadeIn();
	void pixelation(bool blackoutFlag);
	void blocks(bool blackoutFlag);
	void straight(int16 number, bool blackoutFlag);
	void scroll(int16 number);
	void verticalRollFromCenter(bool blackoutFlag);
	void verticalRollToCenter(bool blackoutFlag);
	void horizontalRollFromCenter(bool blackoutFlag);
	void horizontalRollToCenter(bool blackoutFlag);
	void diagonalRollFromCenter(bool blackoutFlag);
	void diagonalRollToCenter(bool blackoutFlag);

	SciGui *_gui;
	SciGuiScreen *_screen;
	SciGuiPalette *_palette;

	bool _isVGA;
	const GuiTransitionTranslateEntry *_translationTable;
	int16 _number;
	bool _blackoutFlag;
	Common::Rect _picRect;
	byte *_oldScreen; // buffer for saving current active screen data to, has dimenions of _screen->_displayScreen
};

} // End of namespace Sci

#endif
