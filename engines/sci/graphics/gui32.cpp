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

#include "common/timer.h"
#include "common/util.h"

#include "sci/sci.h"
#include "sci/debug.h"	// for g_debug_sleeptime_factor
#include "sci/event.h"
#include "sci/engine/state.h"
#include "sci/engine/selector.h"
#include "sci/graphics/gui32.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/palette.h"
#include "sci/graphics/cursor.h"
#include "sci/graphics/cache.h"
#include "sci/graphics/compare.h"
#include "sci/graphics/coordadjuster.h"
#include "sci/graphics/frameout.h"
#include "sci/graphics/paint32.h"
#include "sci/graphics/picture.h"
#include "sci/graphics/robot.h"
#include "sci/graphics/view.h"

namespace Sci {

SciGui32::SciGui32(EngineState *state, GfxScreen *screen, GfxPalette *palette, GfxCache *cache, GfxCursor *cursor)
	: _s(state), _screen(screen), _palette(palette), _cache(cache), _cursor(cursor) {

	_coordAdjuster = new GfxCoordAdjuster32(_s->_segMan);
	_s->_gfxCoordAdjuster = _coordAdjuster;
	_compare = new GfxCompare(_s->_segMan, _s->_kernel, _cache, _screen, _coordAdjuster);
	_s->_gfxCompare = _compare;
	_paint32 = new GfxPaint32(_s->resMan, _s->_segMan, _s->_kernel, _cache, _screen, _palette);
	_s->_gfxPaint = _paint32;
	_frameout = new GfxFrameout(_s->_segMan, _s->resMan, _cache, _screen, _palette, _paint32);
	_s->_gfxFrameout = _frameout;
}

SciGui32::~SciGui32() {
	delete _frameout;
	delete _paint32;
	delete _compare;
	delete _coordAdjuster;
}

void SciGui32::resetEngineState(EngineState *s) {
	_s = s;
}

void SciGui32::init() {
}

void SciGui32::textSize(const char *text, int16 font, int16 maxWidth, int16 *textWidth, int16 *textHeight) {
	*textWidth = 0;
	*textHeight = 0;
}

void SciGui32::shakeScreen(uint16 shakeCount, uint16 directions) {
	while (shakeCount--) {
		if (directions & SCI_SHAKE_DIRECTION_VERTICAL)
			_screen->setVerticalShakePos(10);
		// TODO: horizontal shakes
		g_system->updateScreen();
		g_system->delayMillis(50);
		if (directions & SCI_SHAKE_DIRECTION_VERTICAL)
			_screen->setVerticalShakePos(0);
		g_system->updateScreen();
		g_system->delayMillis(50);
	}
}

void SciGui32::setCursorPos(Common::Point pos) {
	//pos.y += _gfx->GetPort()->top;
	//pos.x += _gfx->GetPort()->left;
	moveCursor(pos);
}

void SciGui32::moveCursor(Common::Point pos) {
	// pos.y += _windowMgr->_picWind->rect.top;
	// pos.x += _windowMgr->_picWind->rect.left;

	// pos.y = CLIP<int16>(pos.y, _windowMgr->_picWind->rect.top, _windowMgr->_picWind->rect.bottom - 1);
	// pos.x = CLIP<int16>(pos.x, _windowMgr->_picWind->rect.left, _windowMgr->_picWind->rect.right - 1);

	if (pos.x > _screen->getWidth() || pos.y > _screen->getHeight()) {
		warning("attempt to place cursor at invalid coordinates (%d, %d)", pos.y, pos.x);
		return;
	}

	_cursor->setPosition(pos);

	// Trigger event reading to make sure the mouse coordinates will
	// actually have changed the next time we read them.
	_s->_event->get(SCI_EVENT_PEEK);
}

void SciGui32::setCursorZone(Common::Rect zone) {
	_cursor->setMoveZone(zone);
}

void SciGui32::drawRobot(GuiResourceId robotId) {
	Robot *test = new Robot(_s->resMan, _screen, robotId);
	test->draw();
	delete test;
}

} // End of namespace Sci
