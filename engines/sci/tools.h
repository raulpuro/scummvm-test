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

#ifndef SCI_TOOLS_H
#define SCI_TOOLS_H

#include "common/scummsys.h"
#include "common/endian.h"

namespace Sci {

int sciprintf(const char *fmt, ...) GCC_PRINTF(1, 2);
/* Prints a string to the console stack
** Parameters: fmt: a printf-style format string
**             ...: Additional parameters as defined in fmt
** Returns   : (int) 1
** Implementation is in src/scicore/console.c
*/

/** Find first set bit in bits and return its index. Returns 0 if bits is 0. */
int sci_ffs(int bits);


#  define BREAKPOINT() { error("Breakpoint in %s, line %d\n", __FILE__, __LINE__); }

} // End of namespace Sci

#endif // SCI_TOOLS_H
