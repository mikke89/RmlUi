#include "FileBrowser.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <PlatformExtensions.h>
#include <Shell.h>

namespace FileBrowser {

struct File {
	File(bool directory, int depth, Rml::String name) :
		visible(depth == 0), directory(directory), collapsed(true), depth(depth), name(std::move(name))
	{}

	bool visible;
	bool directory;
	bool collapsed;
	int depth;
	Rml::String name;
};

static Rml::Vector<File> files;

static void BuildTree(const Rml::String& current_directory, int current_depth)
{
	const Rml::StringList directories = PlatformExtensions::ListDirectories(current_directory);

	for (const Rml::String& directory : directories)
	{
		files.push_back(File(true, current_depth, directory));

		// Recurse into the child directory
		const Rml::String next_directory = current_directory + directory + '/';
		BuildTree(next_directory, current_depth + 1);
	}

	const Rml::StringList filenames = PlatformExtensions::ListFiles(current_directory);
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
