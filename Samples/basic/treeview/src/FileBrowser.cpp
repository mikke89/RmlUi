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

#include "FileBrowser.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <Shell.h>

namespace FileBrowser {

struct File {
	File(bool directory, int depth, Rml::String name) 
		: visible(depth == 0), directory(directory), collapsed(true), depth(depth), name(std::move(name)) {}

	bool visible;
	bool directory;
	bool collapsed;
	int depth;
	Rml::String name;
};

static Rml::Vector<File> files;

static void BuildTree(const Rml::String& current_directory, int current_depth)
{
	const Rml::StringList directories = Shell::ListDirectories(current_directory);

	for (const Rml::String& directory : directories)
	{
		files.push_back(File(true, current_depth, directory));

		// Recurse into the child directory
		const Rml::String next_directory = current_directory + directory + '/';
		BuildTree(next_directory, current_depth + 1);
	}

	const Rml::StringList filenames = Shell::ListFiles(current_directory);
	for (const Rml::String& filename : filenames)
	{
		files.push_back(File(false, current_depth, filename));
	}
}

static void ToggleExpand(Rml::DataModelHandle handle, Rml::Event& /*ev*/, const Rml::VariantList& parameters)
{
	if (parameters.empty())
		return;

	// The index of the file/directory being toggled is passed in as the first parameter.
	const size_t toggle_index = (size_t)parameters[0].Get<int>();
	if (toggle_index >= files.size())
		return;

	File& toggle_file = files[toggle_index];

	const bool collapsing = !toggle_file.collapsed;
	const int depth = toggle_file.depth;

	toggle_file.collapsed = collapsing;

	// Loop through all descendent entries.
	for (size_t i = toggle_index + 1; i < files.size() && files[i].depth > depth; i++)
	{
		// Hide all items if we are collapsing. If instead we are expanding, make all direct children visible.
		files[i].visible = !collapsing && (files[i].depth == depth + 1);
		files[i].collapsed = true;
	}

	handle.DirtyVariable("files");
}


bool Initialise(Rml::Context* context, const Rml::String& root_dir)
{
	BuildTree(root_dir, 0);

	Rml::DataModelConstructor constructor = context->CreateDataModel("filebrowser");

	if (!constructor)
		return false;

	if (auto file_handle = constructor.RegisterStruct<File>())
	{
		file_handle.RegisterMember("visible", &File::visible);
		file_handle.RegisterMember("directory", &File::directory);
		file_handle.RegisterMember("collapsed", &File::collapsed);
		file_handle.RegisterMember("depth", &File::depth);
		file_handle.RegisterMember("name", &File::name);
	}

	constructor.RegisterArray<decltype(files)>();

	constructor.Bind("files", &files);

	constructor.BindEventCallback("toggle_expand", &ToggleExpand);

	return true;
}

} // namespace FileBrowser
