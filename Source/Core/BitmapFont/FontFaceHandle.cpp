/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "../precompiled.h"
#include "FontFaceHandle.h"
#include "FontFaceLayer.h"
#include <algorithm>
#include "../TextureLayout.h"

namespace Rocket {
namespace Core {
namespace BitmapFont {

class FontEffectSort
{
public:
	bool operator()(const Rocket::Core::FontEffect* lhs, const Rocket::Core::FontEffect* rhs)
	{
		return lhs->GetZIndex() < rhs->GetZIndex();
	}
};

FontFaceHandle::FontFaceHandle()
{
	size = 0;
	average_advance = 0;
	x_height = 0;
	line_height = 0;
	baseline = 0;

	underline_position = 0;
	underline_thickness = 0;

	base_layer = NULL;
}

FontFaceHandle::~FontFaceHandle()
{
}

// Initialises the handle so it is able to render text.
bool FontFaceHandle::Initialise(BitmapFontDefinitions *bm_face, const String& _charset, int _size)
{
	this->bm_face = bm_face;
	size = _size;
	line_height = _size;
	texture_width = bm_face->CommonCharactersInfo.ScaleWidth;
	texture_height = bm_face->CommonCharactersInfo.ScaleHeight;
	raw_charset = _charset;

	// Construct proper path to texture
	URL fnt_source = bm_face->Face.Source;
	URL bitmap_source = bm_face->Face.BitmapSource;
	if(bitmap_source.GetPath().Empty())
	{
		texture_source = fnt_source.GetPath() + bitmap_source.GetFileName();
		if(!bitmap_source.GetExtension().Empty())
		{
			texture_source += "." + bitmap_source.GetExtension();
		}
	}
	else
	{
		texture_source = bitmap_source.GetPathedFileName();
	}

	if (!UnicodeRange::BuildList(charset, raw_charset))
	{
		Log::Message(Log::LT_ERROR, "Invalid font charset '%s'.", raw_charset.CString());
		return false;
	}

	// Construct the list of the characters specified by the charset.
	for (size_t i = 0; i < charset.size(); ++i)
		BuildGlyphMap(bm_face, charset[i]);

	// Generate the metrics for the handle.
	GenerateMetrics(bm_face);

	// Generate the default layer and layer configuration.
	base_layer = GenerateLayer(NULL);
	layer_configurations.push_back(LayerConfiguration());
	layer_configurations.back().push_back(base_layer);

	return true;
}

// Returns the width a string will take up if rendered with this handle.
int FontFaceHandle::GetStringWidth(const WString& string, word prior_character) const
{
	int width = 0;

	for (size_t i = 0; i < string.Length(); i++)
	{
		word character_code = string[i];

		if (character_code >= glyphs.size())
			continue;
		const FontGlyph &glyph = glyphs[character_code];

		// Adjust the cursor for the kerning between this character and the previous one.
		if (prior_character != 0)
			width += GetKerning(prior_character, string[i]);
		// Adjust the cursor for this character's advance.
		width += glyph.advance;

		prior_character = character_code;
	}

	return width;
}

// Generates, if required, the layer configuration for a given array of font effects.
int FontFaceHandle::GenerateLayerConfiguration(FontEffectMap& font_effects)
{
	if (font_effects.empty())
		return 0;

	// Prepare a list of effects, sorted by z-index.
	FontEffectList sorted_effects;
	for (FontEffectMap::const_iterator i = font_effects.begin(); i != font_effects.end(); ++i)
		sorted_effects.push_back(i->second);

	std::sort(sorted_effects.begin(), sorted_effects.end(), FontEffectSort());

	// Check each existing configuration for a match with this arrangement of effects.
	int configuration_index = 1;
	for (; configuration_index < (int) layer_configurations.size(); ++configuration_index)
	{
		const LayerConfiguration& configuration = layer_configurations[configuration_index];

		// Check the size is correct. For a math, there should be one layer in the configuration
		// plus an extra for the base layer.
		if (configuration.size() != sorted_effects.size() + 1)
			continue;

		// Check through each layer, checking it was created by the same effect as the one we're
		// checking.
		size_t effect_index = 0;
		for (size_t i = 0; i < configuration.size(); ++i)
		{
			// Skip the base layer ...
			if (configuration[i]->GetFontEffect() == NULL)
				continue;

			// If the ith layer's effect doesn't match the equivalent effect, then this
			// configuration can't match.
			if (configuration[i]->GetFontEffect() != sorted_effects[effect_index])
				break;

			// Check the next one ...
			++effect_index;
		}

		if (effect_index == sorted_effects.size())
			return configuration_index;
	}

	// No match, so we have to generate a new layer configuration.
	layer_configurations.push_back(LayerConfiguration());
	LayerConfiguration& layer_configuration = layer_configurations.back();

	bool added_base_layer = false;

	for (size_t i = 0; i < sorted_effects.size(); ++i)
	{
		if (!added_base_layer &&
			sorted_effects[i]->GetZIndex() >= 0)
		{
			layer_configuration.push_back(base_layer);
			added_base_layer = true;
		}

		layer_configuration.push_back(GenerateLayer(sorted_effects[i]));
	}

	// Add the base layer now if we still haven't added it.
	if (!added_base_layer)
		layer_configuration.push_back(base_layer);

	return (int) (layer_configurations.size() - 1);
}

// Generates the texture data for a layer (for the texture database).
bool FontFaceHandle::GenerateLayerTexture(const byte*& texture_data, Vector2i& texture_dimensions, Rocket::Core::FontEffect* layer_id, int texture_id)
{
	FontLayerMap::iterator layer_iterator = layers.find(layer_id);
	if (layer_iterator == layers.end())
		return false;

	return layer_iterator->second->GenerateTexture(texture_data, texture_dimensions, texture_id);
}

// Generates the geometry required to render a single line of text.
int FontFaceHandle::GenerateString(GeometryList& geometry, const WString& string, const Vector2f& position, const Colourb& colour, int layer_configuration_index) const
{
	int geometry_index = 0;
	int line_width = 0;

	ROCKET_ASSERT(layer_configuration_index >= 0);
	ROCKET_ASSERT(layer_configuration_index < (int) layer_configurations.size());

	// Fetch the requested configuration and generate the geometry for each one.
	const LayerConfiguration& layer_configuration = layer_configurations[layer_configuration_index];
	for (size_t i = 0; i < layer_configuration.size(); ++i)
	{
		Rocket::Core::FontFaceLayer* layer = layer_configuration[i];

		Colourb layer_colour;
		if (layer == base_layer)
			layer_colour = colour;
		else
			layer_colour = layer->GetColour();

		// Resize the geometry list if required.
		if ((int) geometry.size() < geometry_index + layer->GetNumTextures())
			geometry.resize(geometry_index + layer->GetNumTextures());

		// Bind the textures to the geometries.
		for (int i = 0; i < layer->GetNumTextures(); ++i)
			geometry[geometry_index + i].SetTexture(layer->GetTexture(i));

		line_width = 0;
		word prior_character = 0;

		const word* string_iterator = string.CString();
		const word* string_end = string.CString() + string.Length();

		for (; string_iterator != string_end; string_iterator++)
		{
			if (*string_iterator >= glyphs.size())
				continue;
			const FontGlyph &glyph = glyphs[*string_iterator];

			// Adjust the cursor for the kerning between this character and the previous one.
			if (prior_character != 0)
				line_width += GetKerning(prior_character, *string_iterator);

			layer->GenerateGeometry(&geometry[geometry_index], *string_iterator, Vector2f(position.x + line_width, position.y), layer_colour);

			line_width += glyph.advance;
			prior_character = *string_iterator;
		}

		geometry_index += layer->GetNumTextures();
	}

	// Cull any excess geometry from a previous generation.
	geometry.resize(geometry_index);

	return line_width;
}

// Generates the geometry required to render a line above, below or through a line of text.
void FontFaceHandle::GenerateLine(Geometry* geometry, const Vector2f& position, int width, Font::Line height, const Colourb& colour) const
{
	std::vector< Vertex >& line_vertices = geometry->GetVertices();
	std::vector< int >& line_indices = geometry->GetIndices();

	float offset;
	switch (height)
	{
		case Font::UNDERLINE:			offset = -underline_position; break;
		case Font::OVERLINE:			// where to place? offset = -line_height - underline_position; break;
		case Font::STRIKE_THROUGH:		// where to place? offset = -line_height * 0.5f; break;
		default:						return;
	}

	line_vertices.resize(line_vertices.size() + 4);
	line_indices.resize(line_indices.size() + 6);
	GeometryUtilities::GenerateQuad(&line_vertices[0] + (line_vertices.size() - 4), &line_indices[0] + (line_indices.size() - 6), Vector2f(position.x, position.y + offset), Vector2f((float) width, underline_thickness), colour, line_vertices.size() - 4);
}

// Destroys the handle.
void FontFaceHandle::OnReferenceDeactivate()
{
	delete this;
}

void FontFaceHandle::GenerateMetrics(BitmapFontDefinitions *bm_face)
{
	line_height = bm_face->CommonCharactersInfo.LineHeight;
	baseline = bm_face->CommonCharactersInfo.BaseLine;

	underline_position = (float)line_height - bm_face->CommonCharactersInfo.BaseLine;
	baseline += int( underline_position / 1.6f );
	underline_thickness = 1.0f;

	average_advance = 0;
	for (FontGlyphList::iterator i = glyphs.begin(); i != glyphs.end(); ++i)
		average_advance += i->advance;

	// Bring the total advance down to the average advance, but scaled up 10%, just to be on the safe side.
	average_advance = Math::RealToInteger((float) average_advance / (glyphs.size() * 0.9f));

	// Determine the x-height of this font face.
	word x = (word) 'x';
	int index = bm_face->BM_Helper_GetCharacterTableIndex( x );// FT_Get_Char_Index(ft_face, x);

	if ( index >= 0)
		x_height = bm_face->CharactersInfo[ index ].Height;
	else
		x_height = 0;
}

void FontFaceHandle::BuildGlyphMap(BitmapFontDefinitions *bm_face, const UnicodeRange& unicode_range)
{
	glyphs.resize(unicode_range.max_codepoint + 1);

	for (word character_code = (word) (Math::Max< unsigned int >(unicode_range.min_codepoint, 32)); character_code <= unicode_range.max_codepoint; ++character_code)
	{
		int index = bm_face->BM_Helper_GetCharacterTableIndex( character_code );

		if ( index < 0 )
		{
			continue;
		}

		FontGlyph glyph;
		glyph.character = character_code;
		BuildGlyph(glyph, &bm_face->CharactersInfo[ index ] );
		glyphs[character_code] = glyph;
	}
}

void Rocket::Core::BitmapFont::FontFaceHandle::BuildGlyph(FontGlyph& glyph, CharacterInfo *bm_glyph)
{
	// Set the glyph's dimensions.
	glyph.dimensions.x = bm_glyph->Width;
	glyph.dimensions.y = bm_glyph->Height;

	// Set the glyph's bearing.
	glyph.bearing.x = bm_glyph->XOffset;
	glyph.bearing.y = bm_glyph->YOffset;

	// Set the glyph's advance.
	glyph.advance = bm_glyph->Advance;

	// Set the glyph's bitmap position.
	glyph.bitmap_dimensions.x = bm_glyph->X;
	glyph.bitmap_dimensions.y = bm_glyph->Y;

	glyph.bitmap_data = NULL;
}

int Rocket::Core::BitmapFont::FontFaceHandle::GetKerning(word lhs, word rhs) const
{
	if( bm_face != NULL)
	{
		return bm_face->BM_Helper_GetXKerning(lhs, rhs);
	}

	return 0;
}

// Generates (or shares) a layer derived from a font effect.
Rocket::Core::FontFaceLayer* FontFaceHandle::GenerateLayer( FontEffect* font_effect)
{
	// See if this effect has been instanced before, as part of a different configuration.
	FontLayerMap::iterator i = layers.find(font_effect);
	if (i != layers.end())
		return i->second;

	Rocket::Core::FontFaceLayer* layer = new Rocket::Core::BitmapFont::FontFaceLayer();
	layers[font_effect] = layer;

	if (font_effect == NULL)
	{
		layer->Initialise(this);
	}
	else
	{
		// Determine which, if any, layer the new layer should copy its geometry and textures from.
		Rocket::Core::FontFaceLayer* clone = NULL;
		bool deep_clone = true;
		String generation_key;

		if (!font_effect->HasUniqueTexture())
		{
			clone = base_layer;
			deep_clone = false;
		}
		else
		{
			generation_key = font_effect->GetName() + ";" + font_effect->GetGenerationKey();
			FontLayerCache::iterator cache_iterator = layer_cache.find(generation_key);
			if (cache_iterator != layer_cache.end())
				clone = cache_iterator->second;
		}

		// Create a new layer.
		layer->Initialise(this, font_effect, clone, deep_clone);

		// Cache the layer in the layer cache if it generated its own textures (ie, didn't clone).
		if (clone == NULL)
			layer_cache[generation_key] = (Rocket::Core::FontFaceLayer*) layer;
	}

	return (Rocket::Core::FontFaceLayer*)layer;
}

}
}
}
