/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "FileSystem.h"
#include <RmlUi/Core/StringUtilities.h>
#include <cstdlib>
#include <cstdio>

#ifdef WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

struct FileSystemNode;

typedef Rml::Core::UnorderedMap< Rml::Core::String, FileSystemNode* > NodeMap;

FileSystemNode* file_system_root = nullptr;
NodeMap node_map;


/**
	Stores a single node (file or directory) in the file system. This is only used to generate
	interesting data for the data source.
 */

struct FileSystemNode
{
	FileSystemNode(const Rml::Core::String _name, bool _directory, int _depth = -1) : name(_name)
	{
		id = Rml::Core::CreateString(16, "%x", this);

		directory = _directory;
		depth = _depth;

		node_map[id] = this;
	}

	~FileSystemNode()
	{
		for (size_t i = 0; i < child_nodes.size(); ++i)
			delete child_nodes[i];
	}

	// Build the list of files and directories within this directory.
	void BuildTree(const Rml::Core::String& root = "")
	{
#ifdef WIN32
		_finddata_t find_data;
		intptr_t find_handle = _findfirst((root + name + "/*.*").c_str(), &find_data);
		if (find_handle != -1)
		{
			do
			{
				if (strcmp(find_data.name, ".") == 0 ||
					strcmp(find_data.name, "..") == 0)
					continue;

				child_nodes.push_back(new FileSystemNode(find_data.name, (find_data.attrib & _A_SUBDIR) == _A_SUBDIR, depth + 1));

			} while (_findnext(find_handle, &find_data) == 0);

			_findclose(find_handle);
		}
#else
			struct dirent** file_list = nullptr;
			int file_count = -1;
			file_count = scandir((root + name).c_str(), &file_list, 0, alphasort);
			if (file_count == -1)
				return;

			while (file_count--)
			{
				if (strcmp(file_list[file_count]->d_name, ".") == 0 ||
					strcmp(file_list[file_count]->d_name, "..") == 0)
					continue;

				child_nodes.push_back(new FileSystemNode(file_list[file_count]->d_name, (file_list[file_count]->d_type & DT_DIR) == DT_DIR, depth + 1));

				free(file_list[file_count]);
			}
			free(file_list);
#endif

		// Generate the trees of all of our subdirectories.
		for (size_t i = 0; i < child_nodes.size(); ++i)
		{
			if (child_nodes[i]->directory)
				child_nodes[i]->BuildTree(root + name + "/");
		}
	}

	typedef std::vector< FileSystemNode* > NodeList;

	Rml::Core::String id;
	Rml::Core::String name;
	bool directory;
	int depth;

	NodeList child_nodes;
};


FileSystem::FileSystem(const Rml::Core::String& root) : Rml::Controls::DataSource("file")
{
	// Generate the file system nodes starting at the RmlUi's root directory.
	file_system_root = new FileSystemNode(".", true);
	file_system_root->BuildTree(root);
}

FileSystem::~FileSystem()
{
	delete file_system_root;
	file_system_root = nullptr;
}

void FileSystem::GetRow(Rml::Core::StringList& row, const Rml::Core::String& table, int row_index, const Rml::Core::StringList& columns)
{
	// Get the node that data is being queried from; one of its children (as indexed by row_index)
	// is the actual node the data will be read from.
	FileSystemNode* node = GetNode(table);
	if (node == nullptr)
		return;

	for (size_t i = 0; i < columns.size(); i++)
	{
		if (columns[i] == "name")
		{
			// Returns the node's name.
			row.push_back(node->child_nodes[row_index]->name);
		}
		else if (columns[i] == "depth")
		{
			// Returns the depth of the node (ie, how far down the directory structure it is).
			row.push_back(Rml::Core::CreateString(8, "%d", node->child_nodes[row_index]->depth));
		}
		else if (columns[i] == Rml::Controls::DataSource::CHILD_SOURCE)
		{
			// Returns the name of the data source that this node's children can be queried from.
			row.push_back("file." + node->child_nodes[row_index]->id);
		}
	}
}

int FileSystem::GetNumRows(const Rml::Core::String& table)
{
	FileSystemNode* node = GetNode(table);

	if (node != nullptr)
		return (int) node->child_nodes.size();

	return 0;
}

FileSystemNode* FileSystem::GetNode(const Rml::Core::String& table)
{
	// Determine which node the row is being requested from.
	if (table == "root")
		return file_system_root;
	else
	{
		NodeMap::iterator i = node_map.find(table);
		if (i != node_map.end())
			return i->second;
	}

	return nullptr;
}
