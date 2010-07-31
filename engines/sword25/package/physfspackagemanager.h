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

/* 
 * This code is based on Broken Sword 2.5 engine
 *
 * Copyright (c) Malte Thiesen, Daniel Queteschiner and Michael Elsdoerfer
 *
 * Licensed under GNU GPL v2
 *
 */

#ifndef SWORD25_PHYSFS_PACKAGE_MANAGER_H
#define SWORD25_PHYSFS_PACKAGE_MANAGER_H

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include "sword25/kernel/common.h"
#include "sword25/package/packagemanager.h"

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------

class BS_Kernel;

// -----------------------------------------------------------------------------
// Klassendefinition
// -----------------------------------------------------------------------------

class BS_PhysfsPackageManager : public BS_PackageManager
{
public:
	BS_PhysfsPackageManager(BS_Kernel * KernelPtr);
	virtual ~BS_PhysfsPackageManager();

	virtual bool LoadPackage(const std::string & FileName, const std::string& MountPosition);
	virtual bool LoadDirectoryAsPackage(const std::string & DirectoryName, const std::string& MountPosition);
	virtual void* GetFile(const std::string& FileName, unsigned int * FileSizePtr = 0);
	virtual	std::string GetCurrentDirectory();
	virtual bool ChangeDirectory(const std::string & Directory);
	virtual std::string GetAbsolutePath(const std::string & FileName);
	virtual FileSearch* CreateSearch(const std::string & Filter, const std::string& Path, unsigned int TypeFilter = FT_DIRECTORY | FT_FILE);
	virtual unsigned int GetFileSize(const std::string & FileName);
	virtual unsigned int GetFileType(const std::string & FileName);
	virtual bool FileExists(const std::string & FileName);

private:
	std::string m_CurrentDirectory;
};

#endif
