/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2006 The ScummVM project
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
 * $Id: map.h 21480 2006-03-28 15:15:45Z fingolfin $
 */

#ifndef COMMON_FUNC_H
#define COMMON_FUNC_H

#include "common/scummsys.h"

namespace Common {

template <class T>
struct EqualTo {
  bool operator()(const T& x, const T& y) const { return x == y; }
};

template <class T>
struct Less {
  bool operator()(const T& x, const T& y) const { return x < y; }
};

/**
 * Base template for hash functor objects, used by HashMap.
 * This needs to be specialized for every type that you need to hash.
 */
template <typename T> struct Hash;


#define GENERATE_TRIVIAL_HASH_FUNCTOR(T) \
    template <> struct Hash<T> { \
      uint operator()(T val) const { return (uint)val; } \
    }

GENERATE_TRIVIAL_HASH_FUNCTOR(bool);
GENERATE_TRIVIAL_HASH_FUNCTOR(char);
GENERATE_TRIVIAL_HASH_FUNCTOR(signed char);
GENERATE_TRIVIAL_HASH_FUNCTOR(unsigned char);
GENERATE_TRIVIAL_HASH_FUNCTOR(wchar_t);
GENERATE_TRIVIAL_HASH_FUNCTOR(short);
GENERATE_TRIVIAL_HASH_FUNCTOR(int);
GENERATE_TRIVIAL_HASH_FUNCTOR(long);
GENERATE_TRIVIAL_HASH_FUNCTOR(unsigned short);
GENERATE_TRIVIAL_HASH_FUNCTOR(unsigned int);
GENERATE_TRIVIAL_HASH_FUNCTOR(unsigned long);

#undef GENERATE_TRIVIAL_HASH_FUNCTOR



}	// End of namespace Common

#endif
