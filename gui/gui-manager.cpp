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

#include "common/events.h"
#include "common/system.h"
#include "common/util.h"
#include "common/config-manager.h"
#include "common/algorithm.h"
#include "common/timer.h"
#include "common/translation.h"

#include "backends/keymapper/keymapper.h"

#include "gui/gui-manager.h"
#include "gui/dialog.h"
#include "gui/ThemeEngine.h"
#include "gui/ThemeEval.h"
#include "gui/Tooltip.h"

#include "graphics/cursorman.h"

DECLARE_SINGLETON(GUI::GuiManager);

namespace GUI {

enum {
	kDoubleClickDelay = 500, // milliseconds
	kCursorAnimateDelay = 250,
	kTooltipDelay = 1250
};

// Constructor
GuiManager::GuiManager() : _redrawStatus(kRedrawDisabled), _stateIsSaved(false),
    _cursorAnimateCounter(0), _cursorAnimateTimer(0) {
	_theme = 0;
	_useStdCursor = false;

	_system = g_system;
	_lastScreenChangeID = _system->getScreenChangeID();
	_width = _system->getOverlayWidth();
	_height = _system->getOverlayHeight();

	// Clear the cursor
	memset(_cursor, 0xFF, sizeof(_cursor));

#ifdef USE_TRANSLATION
	// Enable translation
	TransMan.setLanguage(ConfMan.get("gui_language").c_str());
#endif // USE_TRANSLATION

	ConfMan.registerDefault("gui_theme", "scummmodern");
	Common::String themefile(ConfMan.get("gui_theme"));

	ConfMan.registerDefault("gui_renderer", ThemeEngine::findModeConfigName(ThemeEngine::_defaultRendererMode));
	ThemeEngine::GraphicsMode gfxMode = (ThemeEngine::GraphicsMode)ThemeEngine::findMode(ConfMan.get("gui_renderer"));

	// Try to load the theme
	if (!loadNewTheme(themefile, gfxMode)) {
		// Loading the theme failed, try to load the built-in theme
		if (!loadNewTheme("builtin", gfxMode)) {
			// Loading the built-in theme failed as well. Bail out
			error("Failed to load any GUI theme, aborting");
		}
	}
}

GuiManager::~GuiManager() {
	delete _theme;
}

#ifdef ENABLE_KEYMAPPER
void GuiManager::initKeymap() {
	using namespace Common;

	bool tmp;
	Keymapper *mapper = _system->getEventManager()->getKeymapper();

	// Do not try to recreate same keymap over again
	if (mapper->getKeymap("gui", tmp) != 0)
		return;

	Action *act;
	Keymap *guiMap = new Keymap("gui");

	act = new Action(guiMap, "CLOS", _("Close"), kGenericActionType, kStartKeyType);
	act->addKeyEvent(KeyState(KEYCODE_ESCAPE, ASCII_ESCAPE, 0));

	act = new Action(guiMap, "CLIK", _("Mouse click"));
	act->addLeftClickEvent();

	act = new Action(guiMap, "VIRT", _("Display keyboard"), kVirtualKeyboardActionType);
	act->addKeyEvent(KeyState(KEYCODE_F7, ASCII_F7, 0));

	act = new Action(guiMap, "REMP", _("Remap keys"), kKeyRemapActionType);
	act->addKeyEvent(KeyState(KEYCODE_F8, ASCII_F8, 0));

	mapper->addGlobalKeymap(guiMap);
}
#endif

bool GuiManager::loadNewTheme(Common::String id, ThemeEngine::GraphicsMode gfx, bool forced) {
	// If we are asked to reload the currently active theme, just do nothing
	// FIXME: Actually, why? It might be desirable at times to force a theme reload...
	if (!forced)
		if (_theme && id == _theme->getThemeId() && gfx == _theme->getGraphicsMode())
			return true;

	ThemeEngine *newTheme = 0;

	if (gfx == ThemeEngine::kGfxDisabled)
		gfx = ThemeEngine::_defaultRendererMode;

	// Try to load the new theme
	newTheme = new ThemeEngine(id, gfx);
	assert(newTheme);

	if (!newTheme->init())
		return false;

	//
	// Disable and delete the old theme
	//
	if (_theme)
		_theme->disable();
	delete _theme;

	if (_useStdCursor) {
		CursorMan.popCursorPalette();
		CursorMan.popCursor();
	}

	//
	// Enable the new theme
	//
	_theme = newTheme;
	_useStdCursor = !_theme->ownCursor();

	// If _stateIsSaved is set, we know that a Theme is already initialized,
	// thus we initialize the new theme properly
	if (_stateIsSaved) {
		_theme->enable();

		if (_useStdCursor)
			setupCursor();
	}

	// refresh all dialogs
	for (int i = 0; i < _dialogStack.size(); ++i)
		_dialogStack[i]->reflowLayout();

	// We need to redraw immediately. Otherwise
	// some other event may cause a widget to be
	// redrawn before redraw() has been called.
	_redrawStatus = kRedrawFull;
	redraw();
	_system->updateScreen();

	return true;
}

void GuiManager::redraw() {
	int i;
	ThemeEngine::ShadingStyle shading;

	if (_redrawStatus == kRedrawDisabled || _dialogStack.empty())
		return;

	shading = (ThemeEngine::ShadingStyle)xmlEval()->getVar("Dialog." + _dialogStack.top()->_name + ".Shading", 0);

	// Tanoku: Do not apply shading more than once when opening many dialogs
	// on top of each other. Screen ends up being too dark and it's a
	// performance hog.
	if (_redrawStatus == kRedrawOpenDialog && _dialogStack.size() > 2)
		shading = ThemeEngine::kShadingNone;

	switch (_redrawStatus) {
		case kRedrawCloseDialog:
		case kRedrawFull:
		case kRedrawTopDialog:
			_theme->clearAll();
			_theme->openDialog(true, ThemeEngine::kShadingNone);

			for (i = 0; i < _dialogStack.size() - 1; i++) {
				_dialogStack[i]->drawDialog();
			}

			_theme->finishBuffering();

		case kRedrawOpenDialog:
			_theme->updateScreen();
			_theme->openDialog(true, shading);
			_dialogStack.top()->drawDialog();
			_theme->finishBuffering();
			break;

		default:
			return;
	}

	_theme->updateScreen();
	_redrawStatus = kRedrawDisabled;
}

Dialog *GuiManager::getTopDialog() const {
	if (_dialogStack.empty())
		return 0;
	return _dialogStack.top();
}

void GuiManager::runLoop() {
	Dialog * const activeDialog = getTopDialog();
	bool didSaveState = false;
	int button;
	uint32 time;

	if (activeDialog == 0)
		return;

	if (!_stateIsSaved) {
		saveState();
		_theme->enable();
		didSaveState = true;

		_useStdCursor = !_theme->ownCursor();
		if (_useStdCursor)
			setupCursor();

//		_theme->refresh();

		_redrawStatus = kRedrawFull;
		redraw();
	}

	_lastMousePosition.x = _lastMousePosition.y = -1;
	_lastMousePosition.time = 0;

	Common::EventManager *eventMan = _system->getEventManager();
	uint32 lastRedraw = 0;
	const uint32 waitTime = 1000 / 45;

#ifdef ENABLE_KEYMAPPER
	// Due to circular reference with event manager and GUI
	// we cannot init keymap on the GUI creation. Thus, let's
	// try to do it on every launch, checking whether the
	// map is already existing
	initKeymap();

	eventMan->getKeymapper()->pushKeymap("gui");
#endif

	bool tooltipCheck = false;

	while (!_dialogStack.empty() && activeDialog == getTopDialog()) {
		redraw();

		// Don't "tickle" the dialog until the theme has had a chance
		// to re-allocate buffers in case of a scaler change.

		activeDialog->handleTickle();

		if (_useStdCursor)
			animateCursor();
//		_theme->updateScreen();
//		_system->updateScreen();

		if (lastRedraw + waitTime < _system->getMillis()) {
			_theme->updateScreen();
			_system->updateScreen();
			lastRedraw = _system->getMillis();
		}

		Common::Event event;

		while (eventMan->pollEvent(event)) {

			// The top dialog can change during the event loop. In that case, flush all the
			// dialog-related events since they were probably generated while the old dialog
			// was still visible, and therefore not intended for the new one.
			//
			// This hopefully fixes strange behaviour/crashes with pop-up widgets. (Most easily
			// triggered in 3x mode or when running ScummVM under Valgrind.)
			if (activeDialog != getTopDialog() && event.type != Common::EVENT_SCREEN_CHANGED)
				continue;

			Common::Point mouse(event.mouse.x - activeDialog->_x, event.mouse.y - activeDialog->_y);

			if (lastRedraw + waitTime < _system->getMillis()) {
				_theme->updateScreen();
				_system->updateScreen();
				lastRedraw = _system->getMillis();
			}

			switch (event.type) {
			case Common::EVENT_KEYDOWN:
				activeDialog->handleKeyDown(event.kbd);
				break;
			case Common::EVENT_KEYUP:
				activeDialog->handleKeyUp(event.kbd);
				break;
			case Common::EVENT_MOUSEMOVE:
				activeDialog->handleMouseMoved(mouse.x, mouse.y, 0);

				if (mouse.x != _lastMousePosition.x || mouse.y != _lastMousePosition.y) {
					_lastMousePosition.x = mouse.x;
					_lastMousePosition.y = mouse.y;
					_lastMousePosition.time = _system->getMillis();
				}

				tooltipCheck = true;
				break;
			// We don't distinguish between mousebuttons (for now at least)
			case Common::EVENT_LBUTTONDOWN:
			case Common::EVENT_RBUTTONDOWN:
				button = (event.type == Common::EVENT_LBUTTONDOWN ? 1 : 2);
				time = _system->getMillis();
				if (_lastClick.count && (time < _lastClick.time + kDoubleClickDelay)
							&& ABS(_lastClick.x - event.mouse.x) < 3
							&& ABS(_lastClick.y - event.mouse.y) < 3) {
					_lastClick.count++;
				} else {
					_lastClick.x = event.mouse.x;
					_lastClick.y = event.mouse.y;
					_lastClick.count = 1;
				}
				_lastClick.time = time;
				activeDialog->handleMouseDown(mouse.x, mouse.y, button, _lastClick.count);
				break;
			case Common::EVENT_LBUTTONUP:
			case Common::EVENT_RBUTTONUP:
				button = (event.type == Common::EVENT_LBUTTONUP ? 1 : 2);
				activeDialog->handleMouseUp(mouse.x, mouse.y, button, _lastClick.count);
				break;
			case Common::EVENT_WHEELUP:
				activeDialog->handleMouseWheel(mouse.x, mouse.y, -1);
				break;
			case Common::EVENT_WHEELDOWN:
				activeDialog->handleMouseWheel(mouse.x, mouse.y, 1);
				break;
			case Common::EVENT_QUIT:
				return;
			case Common::EVENT_SCREEN_CHANGED:
				screenChange();
				break;
			default:
				break;
			}
		}

		if (tooltipCheck && _lastMousePosition.time + kTooltipDelay < _system->getMillis()) {
			Widget *wdg = activeDialog->findWidget(_lastMousePosition.x, _lastMousePosition.y);
			if (wdg && wdg->getTooltip()) {
				Tooltip *tooltip = new Tooltip();
				tooltip->setup(activeDialog, wdg, _lastMousePosition.x, _lastMousePosition.y);
				tooltip->runModal();
				delete tooltip;
			}
		}

		// Delay for a moment
		_system->delayMillis(10);
	}

#ifdef ENABLE_KEYMAPPER
	eventMan->getKeymapper()->popKeymap();
#endif

	if (didSaveState) {
		_theme->disable();
		restoreState();
		_useStdCursor = false;
	}
}

#pragma mark -

void GuiManager::saveState() {
	// Backup old cursor
	_lastClick.x = _lastClick.y = 0;
	_lastClick.time = 0;
	_lastClick.count = 0;

	_stateIsSaved = true;
}

void GuiManager::restoreState() {
	if (_useStdCursor) {
		CursorMan.popCursor();
		CursorMan.popCursorPalette();
	}

	_system->updateScreen();

	_stateIsSaved = false;
}

void GuiManager::openDialog(Dialog *dialog) {
	_dialogStack.push(dialog);
	if (_redrawStatus != kRedrawFull)
		_redrawStatus = kRedrawOpenDialog;

	// We reflow the dialog just before opening it. If the screen changed
	// since the last time we looked, also refresh the loaded theme,
	// and reflow all other open dialogs, too.
	if (!checkScreenChange())
		dialog->reflowLayout();
}

void GuiManager::closeTopDialog() {
	// Don't do anything if no dialog is open
	if (_dialogStack.empty())
		return;

	// Remove the dialog from the stack
	_dialogStack.pop();
	if (_redrawStatus != kRedrawFull)
		_redrawStatus = kRedrawCloseDialog;

	redraw();
}

void GuiManager::setupCursor() {
	const byte palette[] = {
		255, 255, 255, 0,
		255, 255, 255, 0,
		171, 171, 171, 0,
		 87,  87,  87, 0
	};

	CursorMan.pushCursorPalette(palette, 0, 4);
	CursorMan.pushCursor(NULL, 0, 0, 0, 0, 0);
	CursorMan.showMouse(true);
}

// Draw the mouse cursor (animated). This is pretty much the same as in old
// SCUMM games, but the code no longer resembles what we have in cursor.cpp
// very much. We could plug in a different cursor here if we like to.

void GuiManager::animateCursor() {
	int time = _system->getMillis();
	if (time > _cursorAnimateTimer + kCursorAnimateDelay) {
		for (int i = 0; i < 15; i++) {
			if ((i < 6) || (i > 8)) {
				_cursor[16 * 7 + i] = _cursorAnimateCounter;
				_cursor[16 * i + 7] = _cursorAnimateCounter;
			}
		}

		CursorMan.replaceCursor(_cursor, 16, 16, 7, 7, 255);

		_cursorAnimateTimer = time;
		_cursorAnimateCounter = (_cursorAnimateCounter + 1) % 4;
	}
}

bool GuiManager::checkScreenChange() {
	int tmpScreenChangeID = _system->getScreenChangeID();
	if (_lastScreenChangeID != tmpScreenChangeID) {
		screenChange();
		return true;
	}
	return false;
}

void GuiManager::screenChange() {
	_lastScreenChangeID = _system->getScreenChangeID();
	_width = _system->getOverlayWidth();
	_height = _system->getOverlayHeight();

	// reinit the whole theme
	_theme->refresh();

	// refresh all dialogs
	for (int i = 0; i < _dialogStack.size(); ++i) {
		_dialogStack[i]->reflowLayout();
	}
	// We need to redraw immediately. Otherwise
	// some other event may cause a widget to be
	// redrawn before redraw() has been called.
	_redrawStatus = kRedrawFull;
	redraw();
	_system->updateScreen();
}

} // End of namespace GUI
