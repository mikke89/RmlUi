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

#include "FontFaceLayer.h"
#include "FontFaceHandleHarfBuzz.h"
#include <string.h>

FontFaceLayer::FontFaceLayer(const SharedPtr<const FontEffect>& _effect) : colour(255, 255, 255)
{
	effect = _effect;
	if (effect)
		colour = effect->GetColour();
}

FontFaceLayer::~FontFaceLayer() {}

bool FontFaceLayer::Generate(const FontFaceHandleHarfBuzz* handle, const FontFaceLayer* clone, bool clone_glyph_origins)
{
	// Clear the old layout if it exists.
	{
		// @performance: We could be much smarter about this, e.g. such as adding new glyphs to the existing texture layout and textures.
		// Right now we re-generate the whole thing, including textures.
		texture_layout = TextureLayout{};
		character_boxes.clear();
		textures.clear();
	}

	const FontGlyphMap& glyphs = handle->GetGlyphs();

	// Generate the new layout.
	if (clone)
	{
		// Clone the geometry and textures from the clone layer.
		character_boxes = clone->character_boxes;

		// Copy the cloned layer's textures.
		for (size_t i = 0; i < clone->textures.size(); ++i)
			textures.push_back(clone->textures[i]);

		// Request the effect (if we have one) and adjust the origins as appropriate.
		if (effect && !clone_glyph_origins)
		{
			for (auto& pair : glyphs)
			{
				FontGlyphIndex glyph_index = pair.first;
				const FontGlyph& glyph = pair.second;

				auto it = character_boxes.find(glyph_index);
				if (it == character_boxes.end())
				{
					// This can happen if the layers have been dirtied in FontHandleDefault. We will
					// probably be regenerated soon, just skip the character for now.
					continue;
				}

				TextureBox& box = it->second;

				Vector2i glyph_origin = Vector2i(box.origin);
				Vector2i glyph_dimensions = Vector2i(box.dimensions);

				if (effect->GetGlyphMetrics(glyph_origin, glyph_dimensions, glyph))
					box.origin = Vector2f(glyph_origin);
				else
					box.texture_index = -1;
			}
		}
	}
	else
	{
		// Initialise the texture layout for the glyphs.
		character_boxes.reserve(glyphs.size());
		for (auto& pair : glyphs)
		{
			FontGlyphIndex glyph_index = pair.first;
			const FontGlyph& glyph = pair.second;

			Vector2i glyph_origin(0, 0);
			Vector2i glyph_dimensions = glyph.bitmap_dimensions;

			// Adjust glyph origin / dimensions for the font effect.
			if (effect)
			{
				if (!effect->GetGlyphMetrics(glyph_origin, glyph_dimensions, glyph))
					continue;
			}

			TextureBox box;
			box.origin = Vector2f(float(glyph_origin.x + glyph.bearing.x), float(glyph_origin.y - glyph.bearing.y));
			box.dimensions = Vector2f(glyph_dimensions);

			RMLUI_ASSERT(box.dimensions.x >= 0 && box.dimensions.y >= 0);

			character_boxes[glyph_index] = box;

			// Add the character's dimensions into the texture layout engine.
			texture_layout.AddRectangle((int)glyph_index, glyph_dimensions);
		}

		constexpr int max_texture_dimensions = 1024;

		// Generate the texture layout; this will position the glyph rectangles efficiently and
		// allocate the texture data ready for writing.
		if (!texture_layout.GenerateLayout(max_texture_dimensions))
			return false;

		// Iterate over each rectangle in the layout, copying the glyph data into the rectangle as
		// appropriate and generating geometry.
		for (int i = 0; i < texture_layout.GetNumRectangles(); ++i)
		{
			Rml::TextureLayoutRectangle& rectangle = texture_layout.GetRectangle(i);
			const Rml::TextureLayoutTexture& texture = texture_layout.GetTexture(rectangle.GetTextureIndex());
			FontGlyphIndex glyph_index = (FontGlyphIndex)rectangle.GetId();
			RMLUI_ASSERT(character_boxes.find(glyph_index) != character_boxes.end());
			TextureBox& box = character_boxes[glyph_index];

			// Set the character's texture index.
			box.texture_index = rectangle.GetTextureIndex();

			// Generate the character's texture coordinates.
			box.texcoords[0].x = float(rectangle.GetPosition().x) / float(texture.GetDimensions().x);
			box.texcoords[0].y = float(rectangle.GetPosition().y) / float(texture.GetDimensions().y);
			box.texcoords[1].x = float(rectangle.GetPosition().x + rectangle.GetDimensions().x) / float(texture.GetDimensions().x);
			box.texcoords[1].y = float(rectangle.GetPosition().y + rectangle.GetDimensions().y) / float(texture.GetDimensions().y);
		}

		const FontEffect* effect_ptr = effect.get();
		const int handle_version = handle->GetVersion();

		// Generate the textures.
		for (int i = 0; i < texture_layout.GetNumTextures(); ++i)
		{
			const int texture_id = i;

			Rml::TextureCallback texture_callback = [handle, effect_ptr, texture_id, handle_version](Rml::RenderInterface* render_interface,
												   const String& /*name*/, Rml::TextureHandle& out_texture_handle, Vector2i& out_dimensions) -> bool {
				UniquePtr<const byte[]> data;
				if (!handle->GenerateLayerTexture(data, out_dimensions, effect_ptr, texture_id, handle_version) || !data)
					return false;
				if (!render_interface->GenerateTexture(out_texture_handle, data.get(), out_dimensions))
					return false;
				return true;
			};

			Texture texture;
			texture.Set("font-face-layer", texture_callback);
			textures.push_back(texture);
		}
	}

	return true;
}

bool FontFaceLayer::GenerateTexture(UniquePtr<const byte[]>& texture_data, Vector2i& texture_dimensions, int texture_id, const FontGlyphMap& glyphs)
{
	if (texture_id < 0 || texture_id > texture_layout.GetNumTextures())
		return false;

	// Generate the texture data.
	texture_data = texture_layout.GetTexture(texture_id).AllocateTexture();
	texture_dimensions = texture_layout.GetTexture(texture_id).GetDimensions();

	for (int i = 0; i < texture_layout.GetNumRectangles(); ++i)
	{
		Rml::TextureLayoutRectangle& rectangle = texture_layout.GetRectangle(i);
		FontGlyphIndex glyph_index = (FontGlyphIndex)rectangle.GetId();
		RMLUI_ASSERT(character_boxes.find(glyph_index) != character_boxes.end());

		TextureBox& box = character_boxes[glyph_index];

		if (box.texture_index != texture_id)
			continue;

		auto it = glyphs.find((FontGlyphIndex)rectangle.GetId());
		if (it == glyphs.end())
			continue;

		const FontGlyph& glyph = it->second;

		if (effect == nullptr)
		{
			// Copy the glyph's bitmap data into its allocated texture.
			if (glyph.bitmap_data)
			{
				using Rml::ColorFormat;

				byte* destination = rectangle.GetTextureData();
				const byte* source = glyph.bitmap_data;
				const int num_bytes_per_line = glyph.bitmap_dimensions.x * (glyph.color_format == ColorFormat::RGBA8 ? 4 : 1);

				for (int j = 0; j < glyph.bitmap_dimensions.y; ++j)
				{
					switch (glyph.color_format)
					{
					case ColorFormat::A8:
					{
						for (int k = 0; k < num_bytes_per_line; ++k)
							destination[k * 4 + 3] = source[k];
					}
					break;
					case ColorFormat::RGBA8:
					{
						memcpy(destination, source, num_bytes_per_line);
					}
					break;
					}

					destination += rectangle.GetTextureStride();
					source += num_bytes_per_line;
				}
			}
		}
		else
		{
			effect->GenerateGlyphTexture(rectangle.GetTextureData(), Vector2i(box.dimensions), rectangle.GetTextureStride(), glyph);
		}
	}

	return true;
}

void FontFaceLayer::GenerateGeometry(Geometry* geometry, const FontGlyphIndex glyph_index, const Vector2f position, const Colourb colour) const
{
	auto it = character_boxes.find(glyph_index);
	if (it == character_boxes.end())
		return;

	const TextureBox& box = it->second;

	if (box.texture_index < 0)
		return;

	// Generate the geometry for the character.
	Vector<Rml::Vertex>& character_vertices = geometry[box.texture_index].GetVertices();
	Vector<int>& character_indices = geometry[box.texture_index].GetIndices();

	character_vertices.resize(character_vertices.size() + 4);
	character_indices.resize(character_indices.size() + 6);
	Rml::GeometryUtilities::GenerateQuad(&character_vertices[0] + (character_vertices.size() - 4),
		&character_indices[0] + (character_indices.size() - 6), Vector2f(position.x + box.origin.x, position.y + box.origin.y).Round(),
		box.dimensions, colour, box.texcoords[0], box.texcoords[1], (int)character_vertices.size() - 4);
}

const FontEffect* FontFaceLayer::GetFontEffect() const
{
	return effect.get();
}

const Texture* FontFaceLayer::GetTexture(int index)
{
	RMLUI_ASSERT(index >= 0);
	RMLUI_ASSERT(index < GetNumTextures());

	return &(textures[index]);
}

int FontFaceLayer::GetNumTextures() const
{
	return (int)textures.size();
}

Colourb FontFaceLayer::GetColour() const
{
	return colour;
}
