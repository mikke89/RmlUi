/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Rocket/Core/Types.h>
#include <Rocket/Controls/DataSource.h>

struct FileSystemNode;

/**
	Reads the directory structure of the current directory and fills in a data source.
	@author Peter Curry
 */

class FileSystem : public Rocket::Controls::DataSource
{
public:
	FileSystem(const Rocket::Core::String& root);
	virtual ~FileSystem();

	virtual void GetRow(Rocket::Core::StringList& row, const Rocket::Core::String& table, int row_index, const Rocket::Core::StringList& columns);
	virtual int GetNumRows(const Rocket::Core::String& table);

private:
	FileSystemNode* GetNode(const Rocket::Core::String& table);
};

#endif
