#include "FontFaceLayer.h"
#include "FontFaceHandleHarfBuzz.h"
#include <string.h>
#include <type_traits>

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
		textures_owned.clear();
		textures_ptr = &textures_owned;
	}

	const FontGlyphMap& glyphs = handle->GetGlyphs();
	const FallbackFontGlyphMap& fallback_glyphs = handle->GetFallbackGlyphs();
	const FallbackFontClusterGlyphsMap& fallback_cluster_glyphs = handle->GetFallbackClusterGlyphs();

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
				FontGlyphIndex glyph_index = pair.first;
				const FontGlyph& glyph = pair.second.bitmap;
				const Character glyph_character = pair.second.character;

				CloneTextureBox(glyph, glyph_index, glyph_character, false);
			}

			for (auto& pair : fallback_glyphs)
			{
				const Character glyph_character = pair.first;
				const FontGlyph& glyph = pair.second;

				CloneTextureBox(glyph, 0, glyph_character, false);
			}

			for (auto& pair : fallback_cluster_glyphs)
				for (auto& cluster_glyph : pair.second)
				{
					const Character glyph_character = cluster_glyph.glyph_data.character;
					const FontGlyph& glyph = cluster_glyph.glyph_data.bitmap;

					CloneTextureBox(glyph, cluster_glyph.glyph_index, glyph_character, true);
				}
		}
	}
	else
	{
		// Initialise the texture layout for the glyphs.
		character_boxes.reserve(glyphs.size() + fallback_glyphs.size() + fallback_cluster_glyphs.size());
		for (auto& pair : glyphs)
		{
			FontGlyphIndex glyph_index = pair.first;
			const FontGlyph& glyph = pair.second.bitmap;
			Character glyph_character = pair.second.character;

			CreateTextureLayout(glyph, glyph_index, glyph_character, false);
		}

		for (auto& pair : fallback_glyphs)
		{
			Character glyph_character = pair.first;
			const FontGlyph& glyph = pair.second;

			CreateTextureLayout(glyph, 0, glyph_character, false);
		}

		for (auto& pair : fallback_cluster_glyphs)
			for (auto& cluster_glyph : pair.second)
			{
				const Character glyph_character = cluster_glyph.glyph_data.character;
				const FontGlyph& glyph = cluster_glyph.glyph_data.bitmap;

				CreateTextureLayout(glyph, cluster_glyph.glyph_index, glyph_character, true);
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
			uint64_t font_glyph_id = rectangle.GetId();
			RMLUI_ASSERT(character_boxes.find(font_glyph_id) != character_boxes.end());
			TextureBox& box = character_boxes[font_glyph_id];

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

bool FontFaceLayer::GenerateTexture(Vector<byte>& texture_data, Vector2i& texture_dimensions, int texture_id, const FontGlyphMaps& glyph_maps)
{
	if (texture_id < 0 || texture_id > texture_layout.GetNumTextures())
		return false;

	// Generate the texture data.
	texture_data = texture_layout.GetTexture(texture_id).AllocateTexture();
	texture_dimensions = texture_layout.GetTexture(texture_id).GetDimensions();

	for (int i = 0; i < texture_layout.GetNumRectangles(); ++i)
	{
		TextureLayoutRectangle& rectangle = texture_layout.GetRectangle(i);
		uint64_t font_glyph_id = rectangle.GetId();
		RMLUI_ASSERT(character_boxes.find(font_glyph_id) != character_boxes.end());

		TextureBox& box = character_boxes[font_glyph_id];

		if (box.texture_index != texture_id)
			continue;

		const FontGlyph* glyph = nullptr;
		FontGlyphIndex glyph_index = GetFontGlyphIndexFromID(font_glyph_id);
		Rml::Character glyph_character = GetCharacterCodepointFromID(font_glyph_id);
		bool is_cluster = IsFontGlyphIDPartOfCluster(font_glyph_id);

		// Get the glyph bitmap by looking it up with the glyph index.
		RMLUI_ASSERT(glyph_maps.glyphs != nullptr);
		auto it = glyph_maps.glyphs->find(is_cluster ? 0 : glyph_index);
		if (it == glyph_maps.glyphs->end() || glyph_index == 0 || is_cluster)
		{
			// Glyph was not found; attempt to find it in the fallback cluster glyphs.
			if (is_cluster && glyph_maps.fallback_cluster_glyphs)
			{
				uint64_t cluster_glyph_lookup_id = GetFallbackFontClusterGlyphLookupID(glyph_index, glyph_character);
				auto cluster_glyph_it = glyph_maps.fallback_cluster_glyphs->find(cluster_glyph_lookup_id);
				if (cluster_glyph_it != glyph_maps.fallback_cluster_glyphs->end())
					glyph = cluster_glyph_it->second;
			}

			// Glyph was still not found; attempt to find it in the fallback glyphs.
			if (!glyph && !is_cluster && glyph_maps.fallback_glyphs)
			{
				auto fallback_it = glyph_maps.fallback_glyphs->find(glyph_character);
				if (fallback_it != glyph_maps.fallback_glyphs->end())
					// Fallback glyph was found.
					glyph = &fallback_it->second;
			}

			if (!glyph)
			{
				if (it != glyph_maps.glyphs->end())
					// Fallback glyph was not found, but replacement glyph bitmap exists, so use it instead.
					glyph = &it->second.bitmap;
				else
					// No fallback glyph nor replacement glyph bitmap was found; ignore this glyph.
					continue;
			}
		}
		else
			// Glyph was found.
			glyph = &it->second.bitmap;

		if (effect == nullptr)
		{
			// Copy the glyph's bitmap data into its allocated texture.
			if (glyph->bitmap_data)
			{
				byte* destination = rectangle.GetTextureData();
				const byte* source = glyph->bitmap_data;
				const int num_bytes_per_line = glyph->bitmap_dimensions.x * (glyph->color_format == ColorFormat::RGBA8 ? 4 : 1);

				for (int j = 0; j < glyph->bitmap_dimensions.y; ++j)
				{
					switch (glyph->color_format)
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
			effect->GenerateGlyphTexture(rectangle.GetTextureData(), Vector2i(box.dimensions), rectangle.GetTextureStride(), *glyph);
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

uint64_t FontFaceLayer::CreateFontGlyphID(const FontGlyphIndex glyph_index, const Character character_code, bool is_cluster) const
{
	// Font glyph ID details:
	// 64                  48                  32                  16
	// 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000
	// | <---------- glyph_index ----------> | ^| <-------- character_code --------> |
	//                                         |
	//                                     is_cluster
	// 
	// The maximum valid Unicode codepoint is U+10FFFF (slightly larger than 2^20),
	// so dedicating the 32nd bit of 'character_code' to 'is_cluster' shouldn't cause any issues.

	uint64_t font_glyph_id = (static_cast<uint64_t>(glyph_index) << (sizeof(Character) * 8)) | static_cast<uint64_t>(character_code);

	if (is_cluster)
		font_glyph_id |= font_glyph_id_cluster_bit_mask;
	else
		font_glyph_id &= ~font_glyph_id_cluster_bit_mask;

	return font_glyph_id;
}

FontGlyphIndex FontFaceLayer::GetFontGlyphIndexFromID(const uint64_t glyph_id) const
{
	return static_cast<FontGlyphIndex>(glyph_id >> (sizeof(Character) * 8));
}

Character FontFaceLayer::GetCharacterCodepointFromID(const uint64_t glyph_id) const
{
	uint64_t character_codepoint = glyph_id & static_cast<std::underlying_type_t<Character>>(-1);
	character_codepoint &= ~font_glyph_id_cluster_bit_mask;

	return static_cast<Character>(character_codepoint);
}

bool FontFaceLayer::IsFontGlyphIDPartOfCluster(const uint64_t glyph_id) const
{
	return glyph_id & font_glyph_id_cluster_bit_mask;
}

void FontFaceLayer::CreateTextureLayout(const FontGlyph& glyph, FontGlyphIndex glyph_index, Character glyph_character, bool is_cluster)
{
	Vector2i glyph_origin(0, 0);
	Vector2i glyph_dimensions = glyph.bitmap_dimensions;

	// Adjust glyph origin / dimensions for the font effect.
	if (effect)
	{
		if (!effect->GetGlyphMetrics(glyph_origin, glyph_dimensions, glyph))
			return;
	}

	TextureBox box;
	box.origin = Vector2f(float(glyph_origin.x + glyph.bearing.x), float(glyph_origin.y - glyph.bearing.y));
	box.dimensions = Vector2f(glyph_dimensions);

	RMLUI_ASSERT(box.dimensions.x >= 0 && box.dimensions.y >= 0);

	uint64_t font_glyph_id = CreateFontGlyphID(glyph_index, glyph_character, is_cluster);
	character_boxes[font_glyph_id] = box;

	// Add the character's dimensions into the texture layout engine.
	texture_layout.AddRectangle(font_glyph_id, glyph_dimensions);
}

void FontFaceLayer::CloneTextureBox(const FontGlyph& glyph, FontGlyphIndex glyph_index, Character glyph_character, bool is_cluster)
{
	auto it = character_boxes.find(CreateFontGlyphID(glyph_index, glyph_character, is_cluster));
	if (it == character_boxes.end())
	{
		// This can happen if the layers have been dirtied in FontHandleDefault. We will
		// probably be regenerated soon, just skip the character for now.
		return;
	}

	TextureBox& box = it->second;

	Vector2i glyph_origin = Vector2i(box.origin);
	Vector2i glyph_dimensions = Vector2i(box.dimensions);

	if (effect->GetGlyphMetrics(glyph_origin, glyph_dimensions, glyph))
		box.origin = Vector2f(glyph_origin);
	else
		box.texture_index = -1;

}
