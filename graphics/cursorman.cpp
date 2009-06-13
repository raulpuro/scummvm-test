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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 */

#include "graphics/cursorman.h"

#include "common/system.h"
#include "common/stack.h"

DECLARE_SINGLETON(Graphics::CursorManager);

namespace Graphics {

static bool g_initialized = false;

CursorManager::CursorManager() {
	if (!g_initialized) {
		g_initialized = true;
		_cursorStack.clear();
		_cursorPaletteStack.clear();
	}
}

bool CursorManager::isVisible() {
	if (_cursorStack.empty())
		return false;
	return _cursorStack.top()->_visible;
}

bool CursorManager::showMouse(bool visible) {
	if (_cursorStack.empty())
		return false;

	_cursorStack.top()->_visible = visible;

	// Should work, even if there's just a dummy cursor on the stack.
	return g_system->showMouse(visible);
}

#ifdef ENABLE_16BIT
void CursorManager::pushCursor(const byte *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, int targetScale) {
#else
void CursorManager::pushCursor(const byte *buf, uint w, uint h, int hotspotX, int hotspotY, byte keycolor, int targetScale) {
#endif
	Cursor *cur = new Cursor(buf, w, h, hotspotX, hotspotY, keycolor, targetScale);

	cur->_visible = isVisible();
	_cursorStack.push(cur);

	if (buf) {
		g_system->setMouseCursor(cur->_data, w, h, hotspotX, hotspotY, keycolor, targetScale);
	}
}

void CursorManager::popCursor() {
	if (_cursorStack.empty())
		return;

	Cursor *cur = _cursorStack.pop();
	delete cur;

	if (!_cursorStack.empty()) {
		cur = _cursorStack.top();
		g_system->setMouseCursor(cur->_data, cur->_width, cur->_height, cur->_hotspotX, cur->_hotspotY, cur->_keycolor, cur->_targetScale);
	}

	g_system->showMouse(isVisible());
}


void CursorManager::popAllCursors() {
	while (!_cursorStack.empty()) {
		Cursor *cur = _cursorStack.pop();
		delete cur;
	}

	if (g_system->hasFeature(OSystem::kFeatureCursorHasPalette)) {
		while (!_cursorPaletteStack.empty()) {
			Palette *pal = _cursorPaletteStack.pop();
			delete pal;
		}
	}

#ifdef ENABLE_16BIT
	while (!_cursorFormatStack.empty()) {
		PixelFormat *form = _cursorFormatStack.pop();
		delete form;
	}
#endif

	g_system->showMouse(isVisible());
}

#ifdef ENABLE_16BIT
void CursorManager::replaceCursor(const byte *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, int targetScale) {
#else
void CursorManager::replaceCursor(const byte *buf, uint w, uint h, int hotspotX, int hotspotY, byte keycolor, int targetScale) {
#endif

	if (_cursorStack.empty()) {
		pushCursor(buf, w, h, hotspotX, hotspotY, keycolor, targetScale);
		return;
	}

	Cursor *cur = _cursorStack.top();

#ifdef ENABLE_16BIT
	uint size;
	{	//limit the lifespan of the format variable to minimize memory impact
		Graphics::PixelFormat f = g_system->getScreenFormat();
		size = w * h * (f.bytesPerPixel);
	}
#else
	uint size = w * h;
#endif

	if (cur->_size < size) {
		delete[] cur->_data;
		cur->_data = new byte[size];
		cur->_size = size;
	}

	if (buf && cur->_data)
		memcpy(cur->_data, buf, size);

	cur->_width = w;
	cur->_height = h;
	cur->_hotspotX = hotspotX;
	cur->_hotspotY = hotspotY;
	cur->_keycolor = keycolor;
	cur->_targetScale = targetScale;

	g_system->setMouseCursor(cur->_data, w, h, hotspotX, hotspotY, keycolor, targetScale);
}

void CursorManager::disableCursorPalette(bool disable) {
	if (!g_system->hasFeature(OSystem::kFeatureCursorHasPalette))
		return;

	if (_cursorPaletteStack.empty())
		return;

	Palette *pal = _cursorPaletteStack.top();
	pal->_disabled = disable;

	g_system->disableCursorPalette(true);
}

void CursorManager::pushCursorPalette(const byte *colors, uint start, uint num) {
	if (!g_system->hasFeature(OSystem::kFeatureCursorHasPalette))
		return;

	Palette *pal = new Palette(colors, start, num);
	_cursorPaletteStack.push(pal);

	if (num)
		g_system->setCursorPalette(colors, start, num);
	else
		g_system->disableCursorPalette(true);
}

void CursorManager::popCursorPalette() {
	if (!g_system->hasFeature(OSystem::kFeatureCursorHasPalette))
		return;

	if (_cursorPaletteStack.empty())
		return;

	Palette *pal = _cursorPaletteStack.pop();
	delete pal;

	if (_cursorPaletteStack.empty()) {
		g_system->disableCursorPalette(true);
		return;
	}

	pal = _cursorPaletteStack.top();

	if (pal->_num && !pal->_disabled)
		g_system->setCursorPalette(pal->_data, pal->_start, pal->_num);
	else
		g_system->disableCursorPalette(true);
}

void CursorManager::replaceCursorPalette(const byte *colors, uint start, uint num) {
	if (!g_system->hasFeature(OSystem::kFeatureCursorHasPalette))
		return;

	if (_cursorPaletteStack.empty()) {
		pushCursorPalette(colors, start, num);
		return;
	}

	Palette *pal = _cursorPaletteStack.top();
	uint size = 4 * num;

	if (pal->_size < size) {
		// Could not re-use the old buffer. Create a new one.
		delete[] pal->_data;
		pal->_data = new byte[size];
		pal->_size = size;
	}

	pal->_start = start;
	pal->_num = num;

	if (num) {
		memcpy(pal->_data, colors, 4 * num);
		g_system->setCursorPalette(pal->_data, pal->_start, pal->_num);
	} else {
		g_system->disableCursorPalette(true);
	}
}

#ifdef ENABLE_16BIT
void CursorManager::pushCursorFormat(PixelFormat format) {
//	if (!g_system->hasFeature(OSystem::kFeatureCursorHasPalette))
//		return;
	PixelFormat *form = new PixelFormat(format);

	_cursorFormatStack.push(form);
	g_system->setCursorFormat(format);
}

void CursorManager::popCursorFormat() {

	if (_cursorFormatStack.empty())
		return;

	PixelFormat *form = _cursorFormatStack.pop();
	delete form;

	if (_cursorFormatStack.empty()) {
		g_system->setCursorFormat(g_system->getScreenFormat());
		return;
	}

	form = _cursorFormatStack.top();
	disableCursorPalette(form->bytesPerPixel != 1);

	g_system->setCursorFormat(*form);
}

void CursorManager::replaceCursorFormat(PixelFormat format) {
//	if (!g_system->hasFeature(OSystem::kFeatureCursorHasPalette))
//		return;

	if (_cursorFormatStack.empty()) {
		pushCursorFormat(format);
		return;
	}

	PixelFormat *form = _cursorFormatStack.top();

	g_system->setCursorFormat(*form);
}
#endif

} // End of namespace Graphics
