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


#include "SVGCache.h"

#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Texture.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Core.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/FileInterface.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"
#include "../../Include/RmlUi/Core/Utilities.h"

#include <lunasvg.h>

namespace Rml {
namespace SVG {
	
struct SVGDoc
{
	struct Size
	{
		struct Colour
		{
			size_t ref_count = 1u;
			Colourb colour;
			UniquePtr<Geometry> geometry;
		};

		using ColourList = Vector<Colour>;

		// The requested display dimensions of the element or decorator
		Vector2i render_dimensions; 
		bool content_fit;
		UniquePtr<Texture> texture;
		ColourList colours;
	};

	using SizeList = Vector<Size>;

	// The image's intrinsic dimensions based on the svg data.
	Vector2f intrinsic_dimensions;
	UniquePtr<lunasvg::Document> svg_document;
	SizeList render_sizes;
};

using DocumentMap = UnorderedMap<String, SVGDoc>;

struct Handle
{
	size_t ref_count = 1u;

	Geometry* geometry = nullptr;
	Vector2i dimensions;
	Vector2f intrinsic_dimensions;
	String source;
};

using HandleMap = UnorderedMap<SVGHandle, Handle>;

static DocumentMap documents;
static HandleMap handles;

SVGDoc::SizeList::iterator FindSize(SVGDoc& doc, const Vector2i& dimensions)
{
	return std::find_if(doc.render_sizes.begin(), doc.render_sizes.end(), [&dimensions](SVGDoc::Size const& el)
		{
			return el.render_dimensions == dimensions;
		});
}

void SVGCache::Deninitialize()
{
	documents.clear();
	handles.clear();
}

SVGHandle SVGCache::GetHandle(const String& source, const Vector2i& dimensions, const bool content_fit, const Colourb colour)
{
	// generate handle
	SVGHandle handle = 0u;
	Utilities::HashCombine(handle, source);
	Utilities::HashCombine(handle, dimensions.x);
	Utilities::HashCombine(handle, dimensions.y);
	Utilities::HashCombine(handle, content_fit);
	Utilities::HashCombine(handle, *reinterpret_cast<uint16_t const*>(&colour));
	
	auto const found_handle = handles.find(handle);
	if (found_handle != handles.cend())
	{
		found_handle->second.ref_count++;
	}
	else
	{
		// find or create a document
		auto found_doc = documents.find(source);
		if (found_doc == documents.cend())
		{
			SVGDoc doc;
			{
				String svg_data;

				if (source.empty() || !GetFileInterface()->LoadFile(source, svg_data))
				{
					Log::Message(Rml::Log::Type::LT_WARNING, "Could not load SVG file %s", source.c_str());
					return 0u;
				}

				// We use a reset-release approach here in case clients use a non-std unique_ptr (lunasvg uses std::unique_ptr)
				doc.svg_document.reset(lunasvg::Document::loadFromData(svg_data).release());

				if (!doc.svg_document)
				{
					Log::Message(Rml::Log::Type::LT_WARNING, "Could not load SVG data from file %s", source.c_str());
					return 0u;
				}
			}

			doc.intrinsic_dimensions.x = Math::Max(float(doc.svg_document->width()), 1.0f);
			doc.intrinsic_dimensions.y = Math::Max(float(doc.svg_document->height()), 1.0f);

			auto const inserted_it = documents.insert_or_assign(source, std::move(doc));
			RMLUI_ASSERT(inserted_it.second);
			
			found_doc = inserted_it.first;
		}

		SVGDoc& doc = found_doc->second;

		// find or create texture
		auto found_texture = FindSize(doc, dimensions);
		if (found_texture == doc.render_sizes.cend())
		{
			SVGDoc::Size tex_size;
			tex_size.render_dimensions = dimensions;
			tex_size.content_fit = content_fit;
			tex_size.texture = MakeUnique<Texture>();

			// Callback for generating texture.
			auto p_callback = 
				[svg_document = doc.svg_document.get(), dimensions, content_fit](const String& /*name*/, UniquePtr<const byte[]>& data, Vector2i& dim)
				{
					RMLUI_ASSERT(svg_document);

					const size_t total_bytes = 4 * dimensions.x * dimensions.y;

					lunasvg::Bitmap bitmap;
					if (content_fit)
					{
						RMLUI_ASSERT(dimensions.x != 0);
						RMLUI_ASSERT(dimensions.y != 0);

						const lunasvg::Box smallest_fit = svg_document->box();

						lunasvg::Matrix matrix(dimensions.x / svg_document->width(), 0, 0, dimensions.y / svg_document->height(), 0, 0);
						matrix.scale(svg_document->width() / smallest_fit.w, svg_document->height() / smallest_fit.h);
						matrix.translate(-smallest_fit.x, -smallest_fit.y);

						bitmap = lunasvg::Bitmap(dimensions.x, dimensions.y);
						bitmap.clear(0x00000000);
						svg_document->render(bitmap, matrix);
					}
					else
					{
						bitmap = svg_document->renderToBitmap(dimensions.x, dimensions.y);
					}

					data.reset(new byte[total_bytes]);
					memcpy((void*)data.get(), bitmap.data(), total_bytes);
					dim = dimensions;

					return true;
				};

			tex_size.texture->Set("svg", p_callback);

			doc.render_sizes.push_back(std::move(tex_size));
			found_texture = std::prev(doc.render_sizes.end());
		}

		SVGDoc::Size& tex_size = *found_texture;

		// find or create geometry
		auto found_colour = std::find_if(tex_size.colours.begin(), tex_size.colours.end(), [&colour](SVGDoc::Size::Colour const& col_el)
			{
				return col_el.colour == colour;
			});
		if (found_colour != tex_size.colours.cend())
			found_colour->ref_count++;
		else
		{
			SVGDoc::Size::Colour coloured_geo;
			coloured_geo.colour = colour;

			coloured_geo.geometry = MakeUnique<Geometry>();

			Vector< Vertex >& vertices = coloured_geo.geometry->GetVertices();
			Vector< int >& indices = coloured_geo.geometry->GetIndices();

			vertices.resize(4);
			indices.resize(6);

			Vector2f texcoords[2] = {
				{0.0f, 0.0f},
				{1.0f, 1.0f}
			};

			const Vector2f render_dimensions(static_cast<float>(tex_size.render_dimensions.x), static_cast<float>(tex_size.render_dimensions.y));

			GeometryUtilities::GenerateQuad(&vertices[0], &indices[0], Vector2f(0, 0), render_dimensions, colour, texcoords[0], texcoords[1]);

			coloured_geo.geometry->SetTexture(tex_size.texture.get());

			tex_size.colours.push_back(std::move(coloured_geo));
			found_colour = std::prev(tex_size.colours.end());
		}

		// create the handle
		Handle svg_handle;
		svg_handle.geometry = found_colour->geometry.get();
		svg_handle.dimensions = dimensions;
		svg_handle.source = source;
		if (content_fit)
		{
			const lunasvg::Box smallest_fit = doc.svg_document->box();
			svg_handle.intrinsic_dimensions.x = static_cast<float>(smallest_fit.w);
			svg_handle.intrinsic_dimensions.y = static_cast<float>(smallest_fit.h);
		}
		else
			svg_handle.intrinsic_dimensions = doc.intrinsic_dimensions;

		handles[handle] = svg_handle;
	}

	return handle;
}

SVGHandle SVGCache::GetHandle(const String& source, Element* const element, const bool content_fit, const Box::Area area)
{
	const ComputedValues& computed = element->GetComputedValues();

	const float opacity = computed.opacity();
	Colourb colour = computed.image_color();
	colour.alpha = (byte)(opacity * (float)colour.alpha);

	const Vector2f dimensions_f = element->GetBox().GetSize(area).Round();

	return GetHandle(source, Vector2i(static_cast<int>(dimensions_f.x), static_cast<int>(dimensions_f.y)), content_fit, colour);
}

void SVGCache::ReleaseHandle(const SVGHandle handle)
{
	auto found_it = handles.find(handle);
	RMLUI_ASSERT(found_it != handles.cend());
	Handle& svg_handle = found_it->second;
	
	if (--svg_handle.ref_count == 0u)
	{
		auto const found_doc = documents.find(svg_handle.source);
		RMLUI_ASSERT(found_doc != documents.cend());
		SVGDoc& doc = found_doc->second;

		auto found_texture = FindSize(doc, svg_handle.dimensions);
		RMLUI_ASSERT(found_texture != doc.render_sizes.cend());
		SVGDoc::Size& tex_size = *found_texture;

		auto found_geo = std::find_if(tex_size.colours.begin(), tex_size.colours.end(), [&svg_handle](SVGDoc::Size::Colour const& col_el)
			{
				return col_el.geometry.get() == svg_handle.geometry;
			});
		RMLUI_ASSERT(found_geo != tex_size.colours.cend());

		// delete from doc map
		if (--found_geo->ref_count == 0u)
		{
			if (tex_size.colours.size() > 1u)
			{
				std::iter_swap(found_geo, std::prev(tex_size.colours.end()));
				tex_size.colours.pop_back();
			}
			else
			{
				if (doc.render_sizes.size() > 1u)
				{
					std::iter_swap(found_texture, std::prev(doc.render_sizes.end()));
					doc.render_sizes.pop_back();
				}
				else
				{
					documents.erase(found_doc);
				}
			}
		}

		handles.erase(found_it);
	}
}

Geometry* SVGCache::GetGeometry(const SVGHandle handle, Vector2f& intrinsic_dimensions) 
{
	auto const found_it = handles.find(handle);
	if (found_it == handles.cend())
		return nullptr;

	intrinsic_dimensions = found_it->second.intrinsic_dimensions;
	return found_it->second.geometry;
}


} // namespace SVG
} // namespace Rml
