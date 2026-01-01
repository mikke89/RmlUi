#pragma once

#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

using LineNumberList = Vector<int>;

/**
    The document header struct contains the
    header details gathered from an XML document parse.
 */

class DocumentHeader {
public:
	/// Path and filename this document was loaded from
	String source;
	/// The title of the document
	String title;
	/// A list of template resources that can used while parsing the document
	StringList template_resources;

	struct Resource {
		String path;    // Content path for inline resources, source path for external resources.
		String content; // Only set for inline resources.
		bool is_inline = false;
		int line = 0;   // Only set for inline resources.
	};
	using ResourceList = Vector<Resource>;

	/// RCSS definitions
	ResourceList rcss;

	/// script source
	ResourceList scripts;

	/// Merges the specified header with this one
	/// @param header Header to merge
	void MergeHeader(const DocumentHeader& header);

	/// Merges paths from one string list to another, preserving the base_path
	void MergePaths(StringList& target, const StringList& source, const String& base_path);

	/// Merges resources
	void MergeResources(ResourceList& target, const ResourceList& source);
};

} // namespace Rml
