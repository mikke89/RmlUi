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
#include "../../../Include/RmlUi/Core/RenderManager.h"
#include "FontFaceHandleDefault.h"
#include <string.h>
#include <type_traits>

namespace Rml {

FontFaceLayer::FontFaceLayer(const SharedPtr<const FontEffect>& _effect) : colour(255, 255, 255)
{
	effect = _effect;
	if (effect)
		colour = effect->GetColour();
}

FontFaceLayer::~FontFaceLayer() {}

bool FontFaceLayer::Generate(const FontFaceHandleDefault* handle, const FontFaceLayer* clone, bool clone_glyph_origins)
{
	// Clear the old layout if it exists.
	{
		// @performance: We could be much smarter about this, e.g. such as adding new glyphs to the existing texture layout and textures.
		// Right now we re-generate the whole thing, including textures.
		texture_layout = TextureLayout{};
		character_boxes.clear();
		textures_owned.clear();
		textures_ptr = &textures_owned;
	}

	const FontGlyphMap& glyphs = handle->GetGlyphs();

	// Generate the new layout.
	if (clone)
	{
		// Clone the geometry and textures from the clone layer.
		character_boxes = clone->character_boxes;

		// Point our textures to the cloned layer's textures.
		textures_ptr = clone->textures_ptr;

		// Request the effect (if we have one) and adjust the origins as appropriate.
		if (effect && !clone_glyph_origins)
		{
			for (auto& pair : glyphs)
			{
				Character character = pair.first;
				const FontGlyph& glyph = pair.second;

				auto it = character_boxes.find(character);
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
			Character character = pair.first;
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

			character_boxes[character] = box;

			// Add the character's dimensions into the texture layout engine.
			texture_layout.AddRectangle((int)character, glyph_dimensions);
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
			TextureLayoutRectangle& rectangle = texture_layout.GetRectangle(i);
			const TextureLayoutTexture& texture = texture_layout.GetTexture(rectangle.GetTextureIndex());
			Character character = (Character)rectangle.GetId();
			RMLUI_ASSERT(character_boxes.find(character) != character_boxes.end());
			TextureBox& box = character_boxes[character];

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

			CallbackTextureFunction texture_callback = [handle, effect_ptr, texture_id, handle_version](
														   const CallbackTextureInterface& texture_interface) -> bool {
				Vector2i dimensions;
				Vector<byte> data;
				if (!handle->GenerateLayerTexture(data, dimensions, effect_ptr, texture_id, handle_version) || data.empty())
					return false;
				if (!texture_interface.GenerateTexture(data, dimensions))
					return false;
				return true;
			};

			static_assert(std::is_nothrow_move_constructible<CallbackTextureSource>::value,
				"CallbackTextureSource must be nothrow move constructible so that it can be placed in the vector below.");

			textures_owned.emplace_back(std::move(texture_callback));
		}
	}

	return true;
}

bool FontFaceLayer::GenerateTexture(Vector<byte>& texture_data, Vector2i& texture_dimensions, int texture_id, const FontGlyphMap& glyphs)
{
	if (texture_id < 0 || texture_id > texture_layout.GetNumTextures())
		return false;

	// Generate the texture data.
	texture_data = texture_layout.GetTexture(texture_id).AllocateTexture();
	texture_dimensions = texture_layout.GetTexture(texture_id).GetDimensions();

	for (int i = 0; i < texture_layout.GetNumRectangles(); ++i)
	{
		TextureLayoutRectangle& rectangle = texture_layout.GetRectangle(i);
		Character character = (Character)rectangle.GetId();
		RMLUI_ASSERT(character_boxes.find(character) != character_boxes.end());

		TextureBox& box = character_boxes[character];

		if (box.texture_index != texture_id)
			continue;

		auto it = glyphs.find((Character)rectangle.GetId());
		if (it == glyphs.end())
			continue;

		const FontGlyph& glyph = it->second;

		if (effect == nullptr)
		{
			// Copy the glyph's bitmap data into its allocated texture.
			if (glyph.bitmap_data)
			{
				byte* destination = rectangle.GetTextureData();
				const byte* source = glyph.bitmap_data;
				const int num_bytes_per_line = glyph.bitmap_dimensions.x * (glyph.color_format == ColorFormat::RGBA8 ? 4 : 1);

				for (int j = 0; j < glyph.bitmap_dimensions.y; ++j)
				{
					switch (glyph.color_format)
					{
					case ColorFormat::A8:
					{
						// We use premultiplied alpha, so copy the alpha into all four channels.
						for (int k = 0; k < num_bytes_per_line; ++k)
							for (int c = 0; c < 4; ++c)
								destination[k * 4 + c] = source[k];
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

const FontEffect* FontFaceLayer::GetFontEffect() const
{
	return effect.get();
}

Texture FontFaceLayer::GetTexture(RenderManager& render_manager, int index)
{
	RMLUI_ASSERT(index >= 0);
	RMLUI_ASSERT(index < GetNumTextures());

	return (*textures_ptr)[index].GetTexture(render_manager);
}

int FontFaceLayer::GetNumTextures() const
{
	return (int)textures_ptr->size();
}

ColourbPremultiplied FontFaceLayer::GetColour(float opacity) const
{
	return colour.ToPremultiplied(opacity);
}

} // namespace Rml
