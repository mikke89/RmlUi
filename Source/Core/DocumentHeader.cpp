/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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
