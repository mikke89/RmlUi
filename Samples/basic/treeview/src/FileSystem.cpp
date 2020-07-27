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
#include <string.h>
#include <Shell.h>

struct FileSystemNode;

using NodeMap = Rml::UnorderedMap< Rml::String, FileSystemNode* >;

static Rml::UniquePtr<FileSystemNode> file_system_root;
static NodeMap node_map;


/**
	Stores a single node (file or directory) in the file system. This is only used to generate
	interesting data for the data source.
 */

struct FileSystemNode
{
	FileSystemNode(const Rml::String _name, bool _directory, int _depth = -1) : name(_name)
	{
		id = Rml::CreateString(16, "%x", this);

		directory = _directory;
		depth = _depth;

		node_map[id] = this;
	}

	~FileSystemNode()
	{}

	// Build the list of files and directories within this directory.
	void BuildTree(const Rml::String& root = "")
	{
		const Rml::String current_directory = root + name + '/';

		const Rml::StringList directories = Shell::ListDirectories(current_directory);

		for (const Rml::String& directory : directories)
		{
			child_nodes.push_back(Rml::MakeUnique<FileSystemNode>(directory, true, depth + 1));
			child_nodes.back()->BuildTree(current_directory);
		}

		const Rml::StringList files = Shell::ListFiles(current_directory);
		for (const Rml::String& file : files)
		{
			child_nodes.push_back(Rml::MakeUnique<FileSystemNode>(file, false, depth + 1));
		}
	}

	using NodeList = Rml::Vector< Rml::UniquePtr<FileSystemNode> >;

	Rml::String id;
	Rml::String name;
	bool directory;
	int depth;

	NodeList child_nodes;
};


FileSystem::FileSystem(const Rml::String& root) : Rml::DataSource("file")
{
	// Generate the file system nodes starting at the RmlUi's root directory.
	file_system_root = Rml::MakeUnique<FileSystemNode>(".", true);
	file_system_root->BuildTree(root);
}

FileSystem::~FileSystem()
{
	file_system_root.reset();
}

void FileSystem::GetRow(Rml::StringList& row, const Rml::String& table, int row_index, const Rml::StringList& columns)
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
			row.push_back(Rml::CreateString(8, "%d", node->child_nodes[row_index]->depth));
		}
		else if (columns[i] == Rml::DataSource::CHILD_SOURCE)
		{
			// Returns the name of the data source that this node's children can be queried from.
			row.push_back("file." + node->child_nodes[row_index]->id);
		}
	}
}

int FileSystem::GetNumRows(const Rml::String& table)
{
	FileSystemNode* node = GetNode(table);

	if (node != nullptr)
		return (int) node->child_nodes.size();

	return 0;
}

FileSystemNode* FileSystem::GetNode(const Rml::String& table)
{
	// Determine which node the row is being requested from.
	if (table == "root")
		return file_system_root.get();
	else
	{
		NodeMap::iterator i = node_map.find(table);
		if (i != node_map.end())
			return i->second;
	}

	return nullptr;
}
