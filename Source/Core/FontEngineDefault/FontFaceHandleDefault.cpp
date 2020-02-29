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

#include "FontFaceHandleDefault.h"
#include "../../../Include/RmlUi/Core/StringUtilities.h"
#include "../TextureLayout.h"
#include "FontProvider.h"
#include "FontFaceLayer.h"
#include "FreeTypeInterface.h"
#include <algorithm>

namespace Rml {
namespace Core {

FontFaceHandleDefault::FontFaceHandleDefault()
{
	base_layer = nullptr;
	metrics = {};
	ft_face = 0;
}

FontFaceHandleDefault::~FontFaceHandleDefault()
{
	glyphs.clear();
	layers.clear();
}

bool FontFaceHandleDefault::Initialize(FontFaceHandleFreetype face, int font_size)
{
	ft_face = face;

	RMLUI_ASSERTMSG(layer_configurations.empty(), "Initialize must only be called once.");

	if (!FreeType::InitialiseFaceHandle(ft_face, font_size, glyphs, metrics))
	{
		return false;
	}

	// Generate the default layer and layer configuration.
	base_layer = GetOrCreateLayer(nullptr);
	layer_configurations.push_back(LayerConfiguration{ base_layer });

	return true;
}

// Returns the point size of this font face.
int FontFaceHandleDefault::GetSize() const
{
	return metrics.size;
}

// Returns the pixel height of a lower-case x in this font face.
int FontFaceHandleDefault::GetXHeight() const
{
	return metrics.x_height;
}

// Returns the default height between this font face's baselines.
int FontFaceHandleDefault::GetLineHeight() const
{
	return metrics.line_height;
}

// Returns the font's baseline.
int FontFaceHandleDefault::GetBaseline() const
{
	return metrics.baseline;
}

// Returns the font's glyphs.
const FontGlyphMap& FontFaceHandleDefault::GetGlyphs() const
{
	return glyphs;
}

float FontFaceHandleDefault::GetUnderline(float& thickness) const
{
	thickness = metrics.underline_thickness;
	return metrics.underline_position;
}

// Returns the width a string will take up if rendered with this handle.
int FontFaceHandleDefault::GetStringWidth(const String& string, Character prior_character)
{
	int width = 0;
	for (auto it_string = StringIteratorU8(string); it_string; ++it_string)
	{
		Character character = *it_string;

		const FontGlyph* glyph = GetOrAppendGlyph(character);
		if (!glyph)
			continue;

		// Adjust the cursor for the kerning between this character and the previous one.
		if (prior_character != Character::Null)
			width += GetKerning(prior_character, character);
		// Adjust the cursor for this character's advance.
		width += glyph->advance;

		prior_character = character;
	}

	return width;
}

// Generates, if required, the layer configuration for a given array of font effects.
int FontFaceHandleDefault::GenerateLayerConfiguration(const FontEffectList& font_effects)
{
	if (font_effects.empty())
		return 0;

	// Check each existing configuration for a match with this arrangement of effects.
	int configuration_index = 1;
	for (; configuration_index < (int) layer_configurations.size(); ++configuration_index)
	{
		const LayerConfiguration& configuration = layer_configurations[configuration_index];

		// Check the size is correct. For a match, there should be one layer in the configuration
		// plus an extra for the base layer.
		if (configuration.size() != font_effects.size() + 1)
			continue;

		// Check through each layer, checking it was created by the same effect as the one we're
		// checking.
		size_t effect_index = 0;
		for (size_t i = 0; i < configuration.size(); ++i)
		{
			// Skip the base layer ...
			if (configuration[i]->GetFontEffect() == nullptr)
				continue;

			// If the ith layer's effect doesn't match the equivalent effect, then this
			// configuration can't match.
			if (configuration[i]->GetFontEffect() != font_effects[effect_index].get())
				break;

			// Check the next one ...
			++effect_index;
		}

		if (effect_index == font_effects.size())
			return configuration_index;
	}

	// No match, so we have to generate a new layer configuration.
	layer_configurations.push_back(LayerConfiguration());
	LayerConfiguration& layer_configuration = layer_configurations.back();

	bool added_base_layer = false;

	for (size_t i = 0; i < font_effects.size(); ++i)
	{
		if (!added_base_layer && font_effects[i]->GetLayer() == FontEffect::Layer::Front)
		{
			layer_configuration.push_back(base_layer);
			added_base_layer = true;
		}

		FontFaceLayer* new_layer = GetOrCreateLayer(font_effects[i]);
		layer_configuration.push_back(new_layer);
	}

	// Add the base layer now if we still haven't added it.
	if (!added_base_layer)
		layer_configuration.push_back(base_layer);

	return (int) (layer_configurations.size() - 1);
}

// Generates the texture data for a layer (for the texture database).
bool FontFaceHandleDefault::GenerateLayerTexture(UniquePtr<const byte[]>& texture_data, Vector2i& texture_dimensions, const FontEffect* font_effect, int texture_id, int handle_version) const
{
	if (handle_version != version)
	{
		RMLUI_ERRORMSG("While generating font layer texture: Handle version mismatch in texture vs font-face.");
		return false;
	}

	auto it = std::find_if(layers.begin(), layers.end(), [font_effect](const EffectLayerPair& pair) { return pair.font_effect == font_effect; });

	if (it == layers.end())
	{
		RMLUI_ERRORMSG("While generating font layer texture: Layer id not found.");
		return false;
	}

	return it->layer->GenerateTexture(texture_data, texture_dimensions, texture_id, glyphs);
}

// Generates the geometry required to render a single line of text.
int FontFaceHandleDefault::GenerateString(GeometryList& geometry, const String& string, const Vector2f& position, const Colourb& colour, int layer_configuration_index)
{
	int geometry_index = 0;
	int line_width = 0;

	RMLUI_ASSERT(layer_configuration_index >= 0);
	RMLUI_ASSERT(layer_configuration_index < (int) layer_configurations.size());

	UpdateLayersOnDirty();

	// Fetch the requested configuration and generate the geometry for each one.
	const LayerConfiguration& layer_configuration = layer_configurations[layer_configuration_index];

	// Reserve for the common case of one texture per layer.
	geometry.reserve(layer_configuration.size());

	for (size_t i = 0; i < layer_configuration.size(); ++i)
	{
		FontFaceLayer* layer = layer_configuration[i];

		Colourb layer_colour;
		if (layer == base_layer)
			layer_colour = colour;
		else
			layer_colour = layer->GetColour();

		const int num_textures = layer->GetNumTextures();

		if (num_textures == 0)
			continue;

		// Resize the geometry list if required.
		if ((int)geometry.size() < geometry_index + num_textures)
			geometry.resize(geometry_index + num_textures);

		RMLUI_ASSERT(geometry_index < (int)geometry.size());

		// Bind the textures to the geometries.
		for (int i = 0; i < num_textures; ++i)
			geometry[geometry_index + i].SetTexture(layer->GetTexture(i));

		line_width = 0;
		Character prior_character = Character::Null;

		geometry[geometry_index].GetIndices().reserve(string.size() * 6);
		geometry[geometry_index].GetVertices().reserve(string.size() * 4);

		for (auto it_string = StringIteratorU8(string); it_string; ++it_string)
		{
			Character character = *it_string;

			const FontGlyph* glyph = GetOrAppendGlyph(character);
			if (!glyph)
				continue;

			// Adjust the cursor for the kerning between this character and the previous one.
			if (prior_character != Character::Null)
				line_width += GetKerning(prior_character, character);

			layer->GenerateGeometry(&geometry[geometry_index], character, Vector2f(position.x + line_width, position.y), layer_colour);

			line_width += glyph->advance;
			prior_character = character;
		}

		geometry_index += num_textures;
	}

	// Cull any excess geometry from a previous generation.
	geometry.resize(geometry_index);

	return line_width;
}

bool FontFaceHandleDefault::UpdateLayersOnDirty()
{
	bool result = false;

	// If we are dirty, regenerate all the layers and increment the version
	if(is_layers_dirty && base_layer)
	{
		is_layers_dirty = false;
		++version;

		// Regenerate all the layers.
		// Note: The layer regeneration needs to happen in the order in which the layers were created,
		// otherwise we may end up cloning a layer which has not yet been regenerated. This means trouble!
		for (auto& pair : layers)
		{
			GenerateLayer(pair.layer.get());
		}

		result = true;
	}

	return result;
}

int FontFaceHandleDefault::GetVersion() const 
{
	return version;
}

bool FontFaceHandleDefault::AppendGlyph(Character character)
{
	bool result = FreeType::AppendGlyph(ft_face, metrics.size, character, glyphs);
	return result;
}

int FontFaceHandleDefault::GetKerning(Character lhs, Character rhs) const
{
	int result = FreeType::GetKerning(ft_face, metrics.size, lhs, rhs);
	return result;
}

const FontGlyph* FontFaceHandleDefault::GetOrAppendGlyph(Character& character, bool look_in_fallback_fonts)
{
	// Don't try to render control characters
	if ((char32_t)character < (char32_t)' ')
		return nullptr;

	auto it_glyph = glyphs.find(character);
	if (it_glyph == glyphs.end())
	{
		bool result = AppendGlyph(character);

		if (result)
		{
			it_glyph = glyphs.find(character);
			if (it_glyph == glyphs.end())
			{
				RMLUI_ERROR;
				return nullptr;
			}

			is_layers_dirty = true;
		}
		else if (look_in_fallback_fonts)
		{
			const int num_fallback_faces = FontProvider::CountFallbackFontFaces();
			for (int i = 0; i < num_fallback_faces; i++)
			{
				FontFaceHandleDefault* fallback_face = FontProvider::GetFallbackFontFace(i, metrics.size);
				if (!fallback_face || fallback_face == this)
					continue;

				const FontGlyph* glyph = fallback_face->GetOrAppendGlyph(character, false);
				if (glyph)
				{
					// Insert the new glyph into our own set of glyphs
					auto pair = glyphs.emplace(character, glyph->WeakCopy());
					it_glyph = pair.first;
					if(pair.second)
						is_layers_dirty = true;
					break;
				}
			}

			// If we still have not found a glyph, use the replacement character.
			if(it_glyph == glyphs.end())
			{
				character = Character::Replacement;
				it_glyph = glyphs.find(character);
				if (it_glyph == glyphs.end())
					return nullptr;
			}
		}
		else
		{
			return nullptr;
		}
	}

	const FontGlyph* glyph = &it_glyph->second;
	return glyph;
}

// Generates (or shares) a layer derived from a font effect.
FontFaceLayer* FontFaceHandleDefault::GetOrCreateLayer(const SharedPtr<const FontEffect>& font_effect)
{
	// Search for the font effect layer first, it may have been instanced before as part of a different configuration.
	const FontEffect* font_effect_ptr = font_effect.get();
	auto it = std::find_if(layers.begin(), layers.end(), [font_effect_ptr](const EffectLayerPair& pair) { return pair.font_effect == font_effect_ptr; });

	if (it != layers.end())
		return it->layer.get();

	// No existing effect matches, generate a new layer for the effect.
	layers.push_back(EffectLayerPair{ font_effect_ptr, nullptr });
	auto& layer = layers.back().layer;
	
	layer = std::make_unique<FontFaceLayer>(font_effect);
	GenerateLayer(layer.get());

	return layer.get();
}

bool FontFaceHandleDefault::GenerateLayer(FontFaceLayer* layer)
{
	RMLUI_ASSERT(layer);
	const FontEffect* font_effect = layer->GetFontEffect();
	bool result = false;

	if (!font_effect)
	{
		result = layer->Generate(this);
	}
	else
	{
		// Determine which, if any, layer the new layer should copy its geometry and textures from.
		FontFaceLayer* clone = nullptr;
		bool clone_glyph_origins = true;
		String generation_key;
		size_t fingerprint = font_effect->GetFingerprint();

		if (!font_effect->HasUniqueTexture())
		{
			clone = base_layer;
			clone_glyph_origins = false;
		}
		else
		{
			auto cache_iterator = layer_cache.find(fingerprint);
			if (cache_iterator != layer_cache.end() && cache_iterator->second != layer)
				clone = cache_iterator->second;
		}

		// Create a new layer.
		result = layer->Generate(this, clone, clone_glyph_origins);

		// Cache the layer in the layer cache if it generated its own textures (ie, didn't clone).
		if (!clone)
			layer_cache[fingerprint] = layer;
	}

	return result;
}

}
}
