/******************************************************************************/
/* This file is part of Broken Sword 2.5                                      */
/* Copyright (c) Malte Thiesen, Daniel Queteschiner and Michael Elsd�rfer     */
/*                                                                            */
/* Broken Sword 2.5 is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation; either version 2 of the License, or          */
/* (at your option) any later version.                                        */
/*                                                                            */
/* Broken Sword 2.5 is distributed in the hope that it will be useful,        */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               */
/* GNU General Public License for more details.                               */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with Broken Sword 2.5; if not, write to the Free Software            */
/* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA   */
/******************************************************************************/

#ifndef GLS_SPRITE_TILED_H
#define GLS_SPRITE_TILED_H

#include "../glsprites.h"

GLS_Result GLS_NewSpriteTiled(GLS_UInt32 width, GLS_UInt32 height, GLS_Bool useAlphachannel, void * data, GLS_Sprite ** pSprite);
GLS_Result GLS_DeleteSpriteTiled(GLS_Sprite * sprite);

GLS_Result GLS_BlitTiled(GLS_Sprite * sprite,
						GLS_SInt32 x, GLS_SInt32 y,
						const GLS_Rect * subImage,
						const GLS_Color * color,
						GLS_Bool flipH, GLS_Bool flipV,
						GLS_Float scaleX, GLS_Float scaleY);

GLS_Result GLS_SetSpriteDataTiled(GLS_Sprite * sprite,
								  GLS_UInt32 width, GLS_UInt32 height,
								  GLS_Bool useAlphachannel,
								  const void * data,
								  GLS_UInt32 stride);

#endif