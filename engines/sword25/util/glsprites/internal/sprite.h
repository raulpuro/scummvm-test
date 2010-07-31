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

#ifndef GLS_SPRITE_H
#define GLS_SPRITE_H

#include "../glsprites.h"

typedef enum
{
	ST_POW2,
	ST_TILED,
	ST_RECTANGLE,
} GLS_spriteType;

typedef struct 
{
	GLS_spriteType type;
	GLS_Result (*Delete)(GLS_Sprite *);
	GLS_Result (*Blit)(GLS_Sprite *, const GLS_Rect *);
	GLS_Result (*SetData)(GLS_Sprite *, GLS_UInt32, GLS_UInt32, const void *, GLS_UInt32);
} GLS_spriteFunctionTable;

#endif
