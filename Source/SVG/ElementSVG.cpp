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
#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Math.h"
#include "../../Include/RmlUi/Core/PropertyIdSet.h"
#include "../../Include/RmlUi/Core/SystemInterface.h"
#include <cmath>
#include <lunasvg.h>
#include <string.h>

namespace Rml {


ElementSVG::ElementSVG(const String& tag) : Element(tag), geometry(this)
{
}

ElementSVG::~ElementSVG()
{
}

bool ElementSVG::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	if (source_dirty)
		LoadSource();

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
	if (svg_document)
	{
		if (geometry_dirty)
			GenerateGeometry();

		UpdateTexture();
		geometry.Render(GetAbsoluteOffset(Box::CONTENT));
	}
}

void ElementSVG::OnResize()
{
	geometry_dirty = true;
	texture_dirty = true;
}

void ElementSVG::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.count("src"))
	{
		source_dirty = true;
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
		geometry_dirty = true;
	}
}

void ElementSVG::GenerateGeometry()
{
	geometry.Release(true);

	Vector< Vertex >& vertices = geometry.GetVertices();
	Vector< int >& indices = geometry.GetIndices();

	vertices.resize(4);
	indices.resize(6);

	Vector2f texcoords[2] = {
		{0.0f, 0.0f},
		{1.0f, 1.0f}
	};

	const ComputedValues& computed = GetComputedValues();

	const float opacity = computed.opacity();
	Colourb quad_colour = computed.image_color();
	quad_colour.alpha = (byte)(opacity * (float)quad_colour.alpha);

	const Vector2f render_dimensions_f = GetBox().GetSize(Box::CONTENT).Round();
	render_dimensions.x = int(render_dimensions_f.x);
	render_dimensions.y = int(render_dimensions_f.y);

	GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), render_dimensions_f, quad_colour, texcoords[0], texcoords[1]);

	geometry_dirty = false;
}

bool ElementSVG::LoadSource()
{
	source_dirty = false;
	texture_dirty = true;
	intrinsic_dimensions = Vector2f{};
	geometry.SetTexture(nullptr);
	svg_document.reset();

	const String attribute_src = GetAttribute<String>("src", "");

	if (attribute_src.empty())
		return false;

	String path = attribute_src;
	String directory;

	if (ElementDocument* document = GetOwnerDocument())
	{
		const String document_source_url = StringUtilities::Replace(document->GetSourceURL(), '|', ':');
		GetSystemInterface()->JoinPath(path, document_source_url, attribute_src);
		GetSystemInterface()->JoinPath(directory, document_source_url, "");
	}

	String svg_data;

	if (path.empty() || !GetFileInterface()->LoadFile(path, svg_data))
	{
		Log::Message(Rml::Log::Type::LT_WARNING, "Could not load SVG file %s", path.c_str());
		return false;
	}

	// We use a reset-release approach here in case clients use a non-std unique_ptr (lunasvg uses std::unique_ptr)
	svg_document.reset(lunasvg::Document::loadFromData(svg_data).release());

	if (!svg_document)
	{
		Log::Message(Rml::Log::Type::LT_WARNING, "Could not load SVG data from file %s", path.c_str());
		return false;
	}

	intrinsic_dimensions.x = Math::Max(float(svg_document->width()), 1.0f);
	intrinsic_dimensions.y = Math::Max(float(svg_document->height()), 1.0f);

	return true;
}

void ElementSVG::UpdateTexture()
{
	if (!svg_document || !texture_dirty)
		return;

	// Callback for generating texture.
	auto p_callback = [this](const String& /*name*/, UniquePtr<const byte[]>& data, Vector2i& dimensions) -> bool {
		RMLUI_ASSERT(svg_document);

		const size_t total_bytes = 4 * render_dimensions.x * render_dimensions.y;

		lunasvg::Bitmap bitmap = svg_document->renderToBitmap(render_dimensions.x, render_dimensions.y);

		data.reset(new byte[total_bytes]);
		memcpy((void*)data.get(), bitmap.data(), total_bytes);
		dimensions = render_dimensions;

		return true;
	};

	texture.Set("svg", p_callback);
	geometry.SetTexture(&texture);
	texture_dirty = false;
}

} // namespace Rml
