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

#include "precompiled.h"
#include "ElementImage.h"
#include "../../Include/RmlUi/Core.h"
#include "TextureDatabase.h"

namespace Rml {
namespace Core {

// Constructs a new ElementImage.
ElementImage::ElementImage(const String& tag) : Element(tag), dimensions(-1, -1), coords_source(CoordsSource::None), geometry(this)
{
	geometry_dirty = false;
	texture_dirty = true;
}

ElementImage::~ElementImage()
{
}

// Sizes the box to the element's inherent size.
bool ElementImage::GetIntrinsicDimensions(Vector2f& _dimensions)
{
	// Check if we need to reload the texture.
	if (texture_dirty)
		LoadTexture();

	// Calculate the x dimension.
	if (HasAttribute("width"))
		dimensions.x = GetAttribute< float >("width", -1);
	else if (coords_source != CoordsSource::None)
		dimensions.x = coords.width;
	else
		dimensions.x = (float) texture.GetDimensions(GetRenderInterface()).x;

	// Calculate the y dimension.
	if (HasAttribute("height"))
		dimensions.y = GetAttribute< float >("height", -1);
	else if (coords_source != CoordsSource::None)
		dimensions.y = coords.height;
	else
		dimensions.y = (float) texture.GetDimensions(GetRenderInterface()).y;

	// Return the calculated dimensions. If this changes the size of the element, it will result in
	// a 'resize' event which is caught below and will regenerate the geometry.
	_dimensions = dimensions;
	return true;
}

// Renders the element.
void ElementImage::OnRender()
{
	// Regenerate the geometry if required (this will be set if 'coords' changes but does not
	// result in a resize).
	if (geometry_dirty)
		GenerateGeometry();

	// Render the geometry beginning at this element's content region.
	geometry.Render(GetAbsoluteOffset(Rml::Core::Box::CONTENT).Round());
}

// Called when attributes on the element are changed.
void ElementImage::OnAttributeChange(const Rml::Core::ElementAttributes& changed_attributes)
{
	// Call through to the base element's OnAttributeChange().
	Rml::Core::Element::OnAttributeChange(changed_attributes);

	float dirty_layout = false;

	// Check for a changed 'src' attribute. If this changes, the old texture handle is released,
	// forcing a reload when the layout is regenerated.
	if (changed_attributes.find("src") != changed_attributes.end())
	{
		texture_dirty = true;
		dirty_layout = true;
	}

	// Check for a changed 'width' attribute. If this changes, a layout is forced which will
	// recalculate the dimensions.
	if (changed_attributes.find("width") != changed_attributes.end() ||
		changed_attributes.find("height") != changed_attributes.end())
	{
		dirty_layout = true;
	}

	// Check for a change to the 'coords' attribute. If this changes, the coordinates are
	// recomputed and a layout forced. If a sprite is set to source, then that will override any attribute.
	if (changed_attributes.find("coords") != changed_attributes.end() && coords_source != CoordsSource::Sprite)
	{
		if (HasAttribute("coords"))
		{
			StringList coords_list;
			StringUtilities::ExpandString(coords_list, GetAttribute< String >("coords", ""), ' ');

			if (coords_list.size() != 4)
			{
				Rml::Core::Log::Message(Log::LT_WARNING, "Element '%s' has an invalid 'coords' attribute; coords requires 4 values, found %d.", GetAddress().c_str(), coords_list.size());
				ResetCoords();
			}
			else
			{
				coords.x = (float)std::atof(coords_list[0].c_str());
				coords.y = (float)std::atof(coords_list[1].c_str());
				coords.width = (float)std::atof(coords_list[2].c_str());
				coords.height = (float)std::atof(coords_list[3].c_str());

				// We have new, valid coordinates; force the geometry to be regenerated.
				geometry_dirty = true;
				coords_source = CoordsSource::Attribute;

			}
		}
		else
			ResetCoords();

		// Coordinates have changes; this will most likely result in a size change, so we need to force a layout.
		dirty_layout = true;
	}

	if (dirty_layout)
		DirtyLayout();
}

void ElementImage::OnPropertyChange(const PropertyIdSet& changed_properties)
{
    Element::OnPropertyChange(changed_properties);

    if (changed_properties.Contains(PropertyId::ImageColor) ||
        changed_properties.Contains(PropertyId::Opacity)) {
        GenerateGeometry();
    }
}

// Regenerates the element's geometry.
void ElementImage::OnResize()
{
	GenerateGeometry();
}

void ElementImage::GenerateGeometry()
{
	// Release the old geometry before specifying the new vertices.
	geometry.Release(true);

	std::vector< Rml::Core::Vertex >& vertices = geometry.GetVertices();
	std::vector< int >& indices = geometry.GetIndices();

	vertices.resize(4);
	indices.resize(6);

	// Generate the texture coordinates.
	Vector2f texcoords[2];
	if (coords_source != CoordsSource::None)
	{
		Vector2f texture_dimensions((float) texture.GetDimensions(GetRenderInterface()).x, (float) texture.GetDimensions(GetRenderInterface()).y);
		if (texture_dimensions.x == 0)
			texture_dimensions.x = 1;
		if (texture_dimensions.y == 0)
			texture_dimensions.y = 1;

		texcoords[0].x = coords.x / texture_dimensions.x;
		texcoords[0].y = coords.y / texture_dimensions.y;

		texcoords[1].x = (coords.x + coords.width) / texture_dimensions.x;
		texcoords[1].y = (coords.y + coords.height) / texture_dimensions.y;
	}
	else
	{
		texcoords[0] = Vector2f(0, 0);
		texcoords[1] = Vector2f(1, 1);
	}

	const ComputedValues& computed = GetComputedValues();

	float opacity = computed.opacity;
	Colourb quad_colour = computed.image_color;
    quad_colour.alpha = (byte)(opacity * (float)quad_colour.alpha);
	
	Vector2f quad_size = GetBox().GetSize(Rml::Core::Box::CONTENT).Round();

	Rml::Core::GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), quad_size, quad_colour,  texcoords[0], texcoords[1]);

	geometry_dirty = false;
}

bool ElementImage::LoadTexture()
{
	texture_dirty = false;

	// Get the sprite name or image source URL.
	const String source_name = GetAttribute< String >("src", "");
	if (source_name.empty())
		return false;

	geometry_dirty = true;

	bool valid_sprite = false;
	URL source_url;

	Rml::Core::ElementDocument* document = GetOwnerDocument();

	// Check if the name is a sprite first, otherwise treat it as an image url.
	if (document) 
	{
		if (auto& style_sheet = document->GetStyleSheet())
		{
			if (const Sprite* sprite = style_sheet->GetSprite(source_name))
			{
				coords = sprite->rectangle;
				coords_source = CoordsSource::Sprite;
				texture = sprite->sprite_sheet->texture;
				valid_sprite = true;
			}
		}

		if(!valid_sprite)
			source_url.SetURL(document->GetSourceURL());
	}

	if(!valid_sprite)
		texture.Set(source_name, source_url.GetPath());

	// Set the texture onto our geometry object.
	geometry.SetTexture(&texture);
	return true;
}

void ElementImage::ResetCoords()
{
	coords = {};
	coords_source = CoordsSource::None;
}

}
}
