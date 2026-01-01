#include "DocumentHeader.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/StringUtilities.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include "XMLParseTools.h"

namespace Rml {

void DocumentHeader::MergeHeader(const DocumentHeader& header)
{
	// Copy the title across if ours is empty
	if (title.empty())
		title = header.title;
	// Copy the url across if ours is empty
	if (source.empty())
		source = header.source;

	// Combine external data, keeping relative paths
	MergePaths(template_resources, header.template_resources, header.source);
	MergeResources(rcss, header.rcss);
	MergeResources(scripts, header.scripts);
}

void DocumentHeader::MergePaths(StringList& target, const StringList& source, const String& source_path)
{
	for (size_t i = 0; i < source.size(); i++)
	{
		String joined_path;
		::Rml::GetSystemInterface()->JoinPath(joined_path, StringUtilities::Replace(source_path, '|', ':'),
			StringUtilities::Replace(source[i], '|', ':'));

		target.push_back(StringUtilities::Replace(joined_path, ':', '|'));
	}
}

void DocumentHeader::MergeResources(ResourceList& target, const ResourceList& source)
{
	target.insert(target.end(), source.begin(), source.end());
}

} // namespace Rml
