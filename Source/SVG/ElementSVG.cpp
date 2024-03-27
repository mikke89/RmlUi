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

#include "../../Include/RmlUi/SVG/ElementSVG.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/ElementDocument.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"

#include <cmath>
#include <string.h>

#include "SVGCache.h"

namespace Rml {


ElementSVG::ElementSVG(const String& tag) : Element(tag), geometry(nullptr)
{
}

ElementSVG::~ElementSVG()
{
	if (handle != 0u)
	{
		SVG::SVGCache::ReleaseHandle(handle);
	}
}

bool ElementSVG::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	if (source_path.empty() && !source_dirty)
		return false;

	UpdateCachedData();

	dimensions = intrinsic_dimensions;

	if (HasAttribute("width")) {
		dimensions.x = GetAttribute< float >("width", -1);
	}
	if (HasAttribute("height")) {
		dimensions.y = GetAttribute< float >("height", -1);
	}

	if (dimensions.y > 0)
		ratio = dimensions.x / dimensions.y;

	return true;
}

void ElementSVG::OnRender()
{
	UpdateCachedData();
	if (geometry)
	{
		geometry->Render(GetAbsoluteOffset(Box::CONTENT));
	}
}

void ElementSVG::OnResize()
{
	is_dirty = true;
}

void ElementSVG::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.count("src"))
	{
		source_dirty = true;
		DirtyLayout();
	}

	if (changed_attributes.count("crop-to-content"))
	{
		content_fit = GetAttribute<bool>("crop-to-content", false);
		is_dirty = true;
		DirtyLayout();
	}

	if (changed_attributes.find("width") != changed_attributes.end() ||
		changed_attributes.find("height") != changed_attributes.end())
	{
		DirtyLayout();
	}
}

void ElementSVG::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	if (changed_properties.Contains(PropertyId::ImageColor) ||
		changed_properties.Contains(PropertyId::Opacity)) {
		is_dirty = true;
	}
}

void ElementSVG::UpdateCachedData()
{
	if (!is_dirty || !source_dirty)
		return;

	if (source_dirty)
	{
		const String attribute_src = GetAttribute<String>("src", "");

		if (!attribute_src.empty())
		{
			source_path = attribute_src;

			if (ElementDocument* document = GetOwnerDocument())
			{
				const String document_source_url = StringUtilities::Replace(document->GetSourceURL(), '|', ':');
				GetSystemInterface()->JoinPath(source_path, document_source_url, attribute_src);
			}
		}
	}

	if (source_path.empty())
	{
		if (handle != 0u)
		{
			SVG::SVGCache::ReleaseHandle(handle);
		}

		geometry = nullptr;
		intrinsic_dimensions.x = 0.f;
		intrinsic_dimensions.y = 0.f;
		return;
	}

	SVG::SVGHandle const new_handle = SVG::SVGCache::GetHandle(source_path, this, content_fit, Box::CONTENT);
	if (new_handle == 0u)
	{
		geometry = nullptr;
		intrinsic_dimensions.x = 0.f;
		intrinsic_dimensions.y = 0.f;
		return;
	}

	geometry = SVG::SVGCache::GetGeometry(new_handle, intrinsic_dimensions);

	if (handle != 0u)
	{
		SVG::SVGCache::ReleaseHandle(handle);
	}

	handle = new_handle;
	is_dirty = false;
}

} // namespace Rml
