/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019- The RmlUi Team, and contributors
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
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "SVGCache.h"
#include <chrono>
#include <random>

namespace Rml {

std::mt19937 Rml::ElementSVG::rand_gen;

void ElementSVG::Initialize()
{
	// Initialize static rng for generating element ids for non file based svg tags to be used as cache keys
	std::random_device rd;
	rand_gen = std::mt19937(rd());
}

ElementSVG::ElementSVG(const String& tag) : Element(tag) {}

ElementSVG::~ElementSVG()
{
	handle.reset();
}

bool ElementSVG::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	UpdateCachedData();

	dimensions = handle ? handle->intrinsic_dimensions : Vector2f(0);

	if (HasAttribute("width"))
	{
		dimensions.x = GetAttribute<float>("width", 0);
	}
	if (HasAttribute("height"))
	{
		dimensions.y = GetAttribute<float>("height", 0);
	}

	if (dimensions.y > 0)
		ratio = dimensions.x / dimensions.y;

	return true;
}

void ElementSVG::EnsureSourceLoaded()
{
	UpdateCachedData();
}

void ElementSVG::OnRender()
{
	UpdateCachedData();
	if (handle)
		handle->geometry.Render(GetAbsoluteOffset(BoxArea::Content), handle->texture);
}

void ElementSVG::OnResize()
{
	svg_dirty = true;
}

void ElementSVG::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.count("src") || changed_attributes.count("crop-to-content") || changed_attributes.count("_cdata"))
	{
		svg_dirty = true;
		DirtyLayout();
	}

	if (changed_attributes.find("width") != changed_attributes.end() || changed_attributes.find("height") != changed_attributes.end())
	{
		DirtyLayout();
	}
}

void ElementSVG::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	if (changed_properties.Contains(PropertyId::ImageColor) || changed_properties.Contains(PropertyId::Opacity))
	{
		svg_dirty = true;
	}
}

void ElementSVG::UpdateCachedData()
{
	if (!svg_dirty)
		return;

	svg_dirty = false;

	const bool crop_to_content = HasAttribute("crop-to-content");
	const std::string source = GetAttribute<String>("src", "");
	if (source.empty())
	{
		auto cdata = GetAttribute<String>("_cdata", "");
		if (handle)
			handle.reset(); // The old handle won't be re-used so clear it.

		if (cdata.empty())
			return;

		// Build an svg wrapper tag, copying all but src/_cdata attributes (expected attributes could be width, height, viewBox, etc.)
		String svg_tag = "<svg ";
		ElementAttributes attrs = GetAttributes();
		for (auto& attr : attrs)
		{
			if (attr.first == "_source-id" || attr.first == "_cdata" || attr.first == "src")
				continue;
			svg_tag.append(attr.first);
			svg_tag.append("=\"");
			svg_tag.append(StringUtilities::Replace(attr.second.Get<String>(), "\"", "&quot;"));
			svg_tag.append("\" ");
		}
		svg_tag.append(">");
		String svg_data = svg_tag + cdata + "</svg>";
		cdata.erase(); // Clean up early

		auto source_id = GetAttribute<String>("_source-id", "");
		if (source_id.empty())
		{
			source_id = "svg_" +
				std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) +
				"_" + std::to_string(std::generate_canonical<double, 10>(rand_gen) * 1000000);
			SetAttribute("_source-id", source_id);
		}

		handle = SVG::SVGCache::GetHandle(source_id, svg_data, SVG::SVGCache::SOURCE_DATA, this, crop_to_content, BoxArea::Content);
	}
	else
	{
		handle = SVG::SVGCache::GetHandle(source, source, SVG::SVGCache::SOURCE_FILE, this, crop_to_content, BoxArea::Content);
	}
}

} // namespace Rml
