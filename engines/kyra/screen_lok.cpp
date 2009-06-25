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

#include "kyra/kyra_lok.h"
#include "kyra/screen_lok.h"

namespace Kyra {

Screen_LoK::Screen_LoK(KyraEngine_LoK *vm, OSystem *system)
	: Screen(vm, system) {
	_vm = vm;
}

Screen_LoK::~Screen_LoK() {
	for (int i = 0; i < ARRAYSIZE(_saveLoadPage); ++i) {
		delete[] _saveLoadPage[i];
		_saveLoadPage[i] = 0;
	}

	for (int i = 0; i < ARRAYSIZE(_saveLoadPageOvl); ++i) {
		delete[] _saveLoadPageOvl[i];
		_saveLoadPageOvl[i] = 0;
	}

	delete[] _unkPtr1;
	delete[] _unkPtr2;
}

bool Screen_LoK::init() {
	if (!Screen::init())
		return false;

	memset(_bitBlitRects, 0, sizeof(_bitBlitRects));
	_bitBlitNum = 0;
	memset(_saveLoadPage, 0, sizeof(_saveLoadPage));
	memset(_saveLoadPageOvl, 0, sizeof(_saveLoadPageOvl));

	_unkPtr1 = new uint8[getRectSize(1, 144)];
	assert(_unkPtr1);
	memset(_unkPtr1, 0, getRectSize(1, 144));
	_unkPtr2 = new uint8[getRectSize(1, 144)];
	assert(_unkPtr2);
	memset(_unkPtr2, 0, getRectSize(1, 144));

	return true;
}

void Screen_LoK::setScreenDim(int dim) {
	assert(dim < _screenDimTableCount);
	_curDim = &_screenDimTable[dim];
}

const ScreenDim *Screen_LoK::getScreenDim(int dim) {
	assert(dim < _screenDimTableCount);
	return &_screenDimTable[dim];
}

void Screen_LoK::fadeSpecialPalette(int palIndex, int startIndex, int size, int fadeTime) {
	assert(_vm->palTable1()[palIndex]);

	Palette tempPal(getPalette(0).getNumColors());
	tempPal.copy(getPalette(0));
	tempPal.copy(_vm->palTable1()[palIndex], 0, size, startIndex);

	fadePalette(tempPal, fadeTime*18);

	getPalette(0).copy(tempPal, startIndex, size);
	setScreenPalette(getPalette(0));
	_system->updateScreen();
}

void Screen_LoK::addBitBlitRect(int x, int y, int w, int h) {
	if (_bitBlitNum >= kNumBitBlitRects)
		error("too many bit blit rects");

	_bitBlitRects[_bitBlitNum].left = x;
	_bitBlitRects[_bitBlitNum].top = y;
	_bitBlitRects[_bitBlitNum].right = x + w;
	_bitBlitRects[_bitBlitNum].bottom = y + h;
	++_bitBlitNum;
}

void Screen_LoK::bitBlitRects() {
	Common::Rect *cur = _bitBlitRects;
	while (_bitBlitNum) {
		_bitBlitNum--;
		copyRegion(cur->left, cur->top, cur->left, cur->top, cur->width(), cur->height(), 2, 0);
		++cur;
	}
}

void Screen_LoK::savePageToDisk(const char *file, int page) {
	if (!_saveLoadPage[page/2]) {
		_saveLoadPage[page/2] = new uint8[SCREEN_W * SCREEN_H];
		assert(_saveLoadPage[page/2]);
	}
	memcpy(_saveLoadPage[page/2], getPagePtr(page), SCREEN_W * SCREEN_H);

	if (_useOverlays) {
		if (!_saveLoadPageOvl[page/2]) {
			_saveLoadPageOvl[page/2] = new uint8[SCREEN_OVL_SJIS_SIZE];
			assert(_saveLoadPageOvl[page/2]);
		}

		uint8 *srcPage = getOverlayPtr(page);
		if (!srcPage) {
			warning("trying to save unsupported overlay page %d", page);
			return;
		}

		memcpy(_saveLoadPageOvl[page/2], srcPage, SCREEN_OVL_SJIS_SIZE);
	}
}

void Screen_LoK::loadPageFromDisk(const char *file, int page) {
	if (!_saveLoadPage[page/2]) {
		warning("trying to restore page %d, but no backup found", page);
		return;
	}

	copyBlockToPage(page, 0, 0, SCREEN_W, SCREEN_H, _saveLoadPage[page/2]);
	delete[] _saveLoadPage[page/2];
	_saveLoadPage[page/2] = 0;

	if (_saveLoadPageOvl[page/2]) {
		uint8 *dstPage = getOverlayPtr(page);
		if (!dstPage) {
			warning("trying to restore unsupported overlay page %d", page);
			return;
		}

		memcpy(dstPage, _saveLoadPageOvl[page/2], SCREEN_OVL_SJIS_SIZE);
		delete[] _saveLoadPageOvl[page/2];
		_saveLoadPageOvl[page/2] = 0;
	}
}

void Screen_LoK::queryPageFromDisk(const char *file, int page, uint8 *buffer) {
	if (!_saveLoadPage[page/2]) {
		warning("trying to query page %d, but no backup found", page);
		return;
	}

	memcpy(buffer, _saveLoadPage[page/2], SCREEN_W*SCREEN_H);
}

void Screen_LoK::deletePageFromDisk(int page) {
	delete[] _saveLoadPage[page/2];
	_saveLoadPage[page/2] = 0;

	if (_saveLoadPageOvl[page/2]) {
		delete[] _saveLoadPageOvl[page/2];
		_saveLoadPageOvl[page/2] = 0;
	}
}

void Screen_LoK::copyBackgroundBlock(int x, int page, int flag) {
	if (x < 1)
		return;

	int height = 128;
	if (flag)
		height += 8;
	if (!(x & 1))
		++x;
	if (x == 19)
		x = 17;

	uint8 *ptr1 = _unkPtr1;
	uint8 *ptr2 = _unkPtr2;
	int oldVideoPage = _curPage;
	_curPage = page;

	int curX = x;
	hideMouse();
	copyRegionToBuffer(_curPage, 8, 8, 8, height, ptr2);
	for (int i = 0; i < 19; ++i) {
		int tempX = curX + 1;
		copyRegionToBuffer(_curPage, tempX<<3, 8, 8, height, ptr1);
		copyBlockToPage(_curPage, tempX<<3, 8, 8, height, ptr2);
		int newXPos = curX + x;
		if (newXPos > 37)
			newXPos = newXPos % 38;

		tempX = newXPos + 1;
		copyRegionToBuffer(_curPage, tempX<<3, 8, 8, height, ptr2);
		copyBlockToPage(_curPage, tempX<<3, 8, 8, height, ptr1);
		curX += x*2;
		if (curX > 37) {
			curX = curX % 38;
		}
	}
	showMouse();
	_curPage = oldVideoPage;
}

void Screen_LoK::copyBackgroundBlock2(int x) {
	copyBackgroundBlock(x, 4, 1);
}

void Screen_LoK::setTextColorMap(const uint8 *cmap) {
	setTextColor(cmap, 0, 11);
}

int Screen_LoK::getRectSize(int x, int y) {
	if (x < 1)
		x = 1;
	else if (x > 40)
		x = 40;

	if (y < 1)
		y = 1;
	else if (y > 200)
		y = 200;

	return ((x*y) << 3);
}

#pragma mark -

Screen_LoK_16::Screen_LoK_16(KyraEngine_LoK *vm, OSystem *system) : Screen_LoK(vm, system) {
}

void Screen_LoK_16::setScreenPalette(const Palette &pal) {
	_screenPalette->copy(pal);

	for (int i = 0; i < 256; ++i)
		paletteMap(i, pal[i * 3 + 0] << 2, pal[i * 3 + 1] << 2, pal[i * 3 + 2] << 2);

	uint8 palette[16 * 4];
	for (int i = 0; i < 16; ++i) {
		palette[i * 4 + 0] = (_palette16[i * 3 + 0] * 0xFF) / 0x0F;
		palette[i * 4 + 1] = (_palette16[i * 3 + 1] * 0xFF) / 0x0F;
		palette[i * 4 + 2] = (_palette16[i * 3 + 2] * 0xFF) / 0x0F;
		palette[i * 4 + 3] = 0;
	}

	_system->setPalette(palette, 0, 16);
}

void Screen_LoK_16::paletteMap(uint8 idx, int r, int g, int b) {
	const int red = r;
	const int green = g;
	const int blue = b;

	uint16 rgbDiff = 1000;
	int rDiff = 0, gDiff = 0, bDiff = 0;

	int index1 = -1;

	for (int i = 0; i < 16; ++i) {
		const int realR = _palette16[i * 3 + 0] << 4;
		const int realG = _palette16[i * 3 + 1] << 4;
		const int realB = _palette16[i * 3 + 2] << 4;

		uint16 diff = ABS(r - realR) + ABS(g - realG) + ABS(b - realB);

		if (diff < rgbDiff) {
			rgbDiff = diff;
			index1 = i;

			rDiff = r - realR;
			gDiff = g - realG;
			bDiff = b - realB;
		}
	}

	r = rDiff / 4 + red;
	g = gDiff / 4 + green;
	b = bDiff / 4 + blue;

	rgbDiff = 1000;
	int index2 = -1;

	for (int i = 0; i < 16; ++i) {
		const int realR = _palette16[i * 3 + 0] << 4;
		const int realG = _palette16[i * 3 + 1] << 4;
		const int realB = _palette16[i * 3 + 2] << 4;

		uint16 diff = ABS(r - realR) + ABS(g - realG) + ABS(b - realB);

		if (diff < rgbDiff) {
			rgbDiff = diff;
			index2 = i;
		}
	}

	_paletteMap[idx * 4 + 0] = index2;
	_paletteMap[idx * 4 + 3] = index2;

	_paletteMap[idx * 4 + 1] = index1;
	_paletteMap[idx * 4 + 2] = index1;
}

void Screen_LoK_16::convertTo16Colors(uint8 *page, int w, int h) {
	const int rowAdd = 1280 - w;

	uint8 *row1 = page;
	uint8 *row2 = page + 640;

	for (int i = 0; i < h; i += 2) {
		for (int k = 0; k < w; k += 2) {
			*row1 = _paletteMap[*row1 * 4 + 0]; ++row1;
			*row1 = _paletteMap[*row1 * 4 + 1]; ++row1;

			*row2 = _paletteMap[*row2 * 4 + 2]; ++row2;
			*row2 = _paletteMap[*row2 * 4 + 3]; ++row2;
		}

		row1 += rowAdd;
		row2 += rowAdd;
	}
}

void Screen_LoK_16::mergeOverlay(int x, int y, int w, int h) {
	Screen_LoK::mergeOverlay(x, y, w, h);

	convertTo16Colors(_sjisOverlayPtrs[0] + y * 640 + x, w, h);
}

} // end of namespace Kyra
