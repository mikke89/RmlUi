#include "FontFaceHandleHarfBuzz.h"
#include "FontEngineDefault/FreeTypeInterface.h"
#include "FontFaceLayer.h"
#include "FontProvider.h"
#include "FreeTypeInterface.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <hb-ft.h>
#include <hb.h>
#include <numeric>
#include <utility>

static bool IsControlCharacter(Character c)
{
	return (char32_t)c < U' ' || ((char32_t)c >= U'\x7F' && (char32_t)c <= U'\x9F');
}

static bool IsKerningEnabled(const TextShapingContext& text_shaping_context, int font_size)
{
	static constexpr int minimum_font_size_to_enable_kerning = 14;

	switch (text_shaping_context.font_kerning)
	{
	case Style::FontKerning::Normal: return true;
	case Style::FontKerning::None: return false;
	default: return font_size >= minimum_font_size_to_enable_kerning;
	}
}

static Vector<hb_feature_t> GetTextShapingFeatures(const TextShapingContext& text_shaping_context, int font_size)
{
	Vector<hb_feature_t> shaping_features;

	if (!IsKerningEnabled(text_shaping_context, font_size))
	{
		// Kerning is enabled by default in HarfBuzz, so we need to explicitly disable it.
		shaping_features.emplace_back();
		hb_feature_from_string("kern=0", -1, &shaping_features.back());
	}

	return shaping_features;
}

FontFaceHandleHarfBuzz::FontFaceHandleHarfBuzz()
{
	base_layer = nullptr;
	metrics = {};
	ft_face = 0;
	hb_font = nullptr;
}

FontFaceHandleHarfBuzz::~FontFaceHandleHarfBuzz()
{
	hb_font_destroy(hb_font);

	glyphs.clear();
	layers.clear();
}

bool FontFaceHandleHarfBuzz::Initialize(FontFaceHandleFreetype face, int font_size, bool load_default_glyphs)
{
	ft_face = face;

	RMLUI_ASSERTMSG(layer_configurations.empty(), "Initialize must only be called once.");

	if (!FreeType::InitialiseFaceHandle(ft_face, font_size, glyphs, metrics, load_default_glyphs))
		return false;

	hb_font = hb_ft_font_create_referenced((FT_Face)ft_face);
	RMLUI_ASSERT(hb_font != nullptr);
	hb_font_set_ptem(hb_font, (float)font_size);
	hb_ft_font_set_funcs(hb_font);

	// Generate the default layer and layer configuration.
	base_layer = GetOrCreateLayer(nullptr);
	layer_configurations.push_back(LayerConfiguration{base_layer});

	return true;
}

const FontMetrics& FontFaceHandleHarfBuzz::GetFontMetrics() const
{
	return metrics;
}

const FontGlyphMap& FontFaceHandleHarfBuzz::GetGlyphs() const
{
	return glyphs;
}

const FallbackFontGlyphMap& FontFaceHandleHarfBuzz::GetFallbackGlyphs() const
{
	return fallback_glyphs;
}

const FallbackFontClusterGlyphsMap& FontFaceHandleHarfBuzz::GetFallbackClusterGlyphs() const
{
	return fallback_cluster_glyphs;
}

int FontFaceHandleHarfBuzz::GetStringWidth(StringView string, const TextShapingContext& text_shaping_context,
	const LanguageDataMap& registered_languages, Character /*prior_character*/)
{
	int width = 0;

	// Apply text shaping.
	hb_buffer_t* shaping_buffer = hb_buffer_create();
	RMLUI_ASSERT(shaping_buffer != nullptr);
	ConfigureTextShapingBuffer(shaping_buffer, string, text_shaping_context, registered_languages, nullptr);
	hb_buffer_add_utf8(shaping_buffer, string.begin(), (int)string.size(), 0, (int)string.size());

	Vector<hb_feature_t> shaping_features = GetTextShapingFeatures(text_shaping_context, metrics.size);
	const hb_feature_t* shaping_features_pointer = !shaping_features.empty() ? shaping_features.data() : nullptr;
	hb_shape(hb_font, shaping_buffer, shaping_features_pointer, (unsigned int)shaping_features.size());

	unsigned int glyph_count = 0;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(shaping_buffer, &glyph_count);
	hb_glyph_position_t* glyph_positions = hb_buffer_get_glyph_positions(shaping_buffer, &glyph_count);

	Queue<Pair<FontGlyphIndex, const FontGlyph*>> glyph_queue;

	for (int g = 0; g < (int)glyph_count; ++g)
	{
		Character character = Rml::StringUtilities::ToCharacter(string.begin() + glyph_info[g].cluster, string.end());

		// Don't render control characters.
		if (IsControlCharacter(character))
			continue;

		const FontGlyphIndex glyph_index = glyph_info[g].codepoint;
		int extra_glyph_index_offset = 0;

		if (glyph_index == 0)
		{
			// Check to see if the glyph is the start of an unsupported multi-character cluster.
			int cluster_codepoint_count = 0;
			StringView cluster_string = GetCurrentClusterString(glyph_info, glyph_count, g, character, string, cluster_codepoint_count);

			if (cluster_codepoint_count > 1)
			{
				// Unsupported cluster detected; use fallback cluster glyph if one is available.
				const Vector<FontClusterGlyphData>* cluster_glyphs =
					GetOrAppendFallbackClusterGlyphs(cluster_string, text_shaping_context, registered_languages, shaping_features);

				if (cluster_glyphs)
				{
					extra_glyph_index_offset = cluster_codepoint_count - 1;
					for (const auto& cluster_glyph : *cluster_glyphs)
						glyph_queue.emplace(cluster_glyph.glyph_index, &cluster_glyph.glyph_data.bitmap);
				}
			}
		}

		if (glyph_queue.empty())
		{
			const FontGlyph* glyph = GetOrAppendGlyph(glyph_index, character);
			if (glyph)
				glyph_queue.emplace(glyph_index, glyph);
		}

		while (!glyph_queue.empty())
		{
			auto& glyph_pair = glyph_queue.front();

			// Adjust the cursor for this character's advance.
			if (glyph_info[g].codepoint != 0)
				width += glyph_positions[g].x_advance >> 6;
			else
				// Use the unshaped advance for unsupported characters.
				width += glyph_pair.second->advance;

			width += (int)text_shaping_context.letter_spacing;

			glyph_queue.pop();
		}
		
		g += extra_glyph_index_offset;
	}

	hb_buffer_destroy(shaping_buffer);

	return Rml::Math::Max(width, 0);
}

int FontFaceHandleHarfBuzz::GenerateLayerConfiguration(const FontEffectList& font_effects)
{
	if (font_effects.empty())
		return 0;

	// Check each existing configuration for a match with this arrangement of effects.
	int configuration_index = 1;
	for (; configuration_index < (int)layer_configurations.size(); ++configuration_index)
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

	return (int)(layer_configurations.size() - 1);
}

bool FontFaceHandleHarfBuzz::GenerateLayerTexture(Vector<byte>& texture_data, Vector2i& texture_dimensions, const FontEffect* font_effect,
	int texture_id, int handle_version) const
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

	return it->layer->GenerateTexture(texture_data, texture_dimensions, texture_id,
		FontGlyphMaps{&glyphs, &fallback_glyphs, &fallback_cluster_glyphs_lookup});
}

int FontFaceHandleHarfBuzz::GenerateString(RenderManager& render_manager, TexturedMeshList& mesh_list, StringView string, const Vector2f position,
	const ColourbPremultiplied colour, const float opacity, const TextShapingContext& text_shaping_context,
	const LanguageDataMap& registered_languages, const int layer_configuration_index)
{
	int geometry_index = 0;
	int line_width = 0;

	RMLUI_ASSERT(layer_configuration_index >= 0);
	RMLUI_ASSERT(layer_configuration_index < (int)layer_configurations.size());

	UpdateLayersOnDirty();

	// Fetch the requested configuration and generate the geometry for each one.
	const LayerConfiguration& layer_configuration = layer_configurations[layer_configuration_index];

	// Each texture represents one geometry.
	const int num_geometries = std::accumulate(layer_configuration.begin(), layer_configuration.end(), 0,
		[](int sum, const FontFaceLayer* layer) { return sum + layer->GetNumTextures(); });

	mesh_list.resize(num_geometries);

	hb_buffer_t* shaping_buffer = hb_buffer_create();
	RMLUI_ASSERT(shaping_buffer != nullptr);
	Vector<hb_feature_t> shaping_features = GetTextShapingFeatures(text_shaping_context, metrics.size);
	const hb_feature_t* shaping_features_pointer = !shaping_features.empty() ? shaping_features.data() : nullptr;

	for (size_t layer_index = 0; layer_index < layer_configuration.size(); ++layer_index)
	{
		FontFaceLayer* layer = layer_configuration[layer_index];

		ColourbPremultiplied layer_colour;
		if (layer == base_layer)
			layer_colour = colour;
		else
			layer_colour = layer->GetColour(opacity);

		const int num_textures = layer->GetNumTextures();
		if (num_textures == 0)
			continue;

		RMLUI_ASSERT(geometry_index + num_textures <= (int)mesh_list.size());

		line_width = 0;

		// Set the mesh and textures to the geometries.
		for (int tex_index = 0; tex_index < num_textures; ++tex_index)
			mesh_list[geometry_index + tex_index].texture = layer->GetTexture(render_manager, tex_index);

		// Set up and apply text shaping.
		hb_buffer_clear_contents(shaping_buffer);
		ConfigureTextShapingBuffer(shaping_buffer, string, text_shaping_context, registered_languages, nullptr);
		hb_buffer_add_utf8(shaping_buffer, string.begin(), (int)string.size(), 0, (int)string.size());
		hb_shape(hb_font, shaping_buffer, shaping_features_pointer, (unsigned int)shaping_features.size());

		unsigned int glyph_count = 0;
		hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(shaping_buffer, &glyph_count);
		hb_glyph_position_t* glyph_positions = hb_buffer_get_glyph_positions(shaping_buffer, &glyph_count);

		mesh_list[geometry_index].mesh.indices.reserve(string.size() * 6);
		mesh_list[geometry_index].mesh.vertices.reserve(string.size() * 4);

		Queue<Pair<FontGlyphIndex, FontGlyphReference>> glyph_queue;

		for (int g = 0; g < (int)glyph_count; ++g)
		{
			Character character = Rml::StringUtilities::ToCharacter(string.begin() + glyph_info[g].cluster, string.end());

			// Don't render control characters.
			if (IsControlCharacter(character))
				continue;

			const FontGlyphIndex glyph_index = glyph_info[g].codepoint;
			int extra_glyph_index_offset = 0;
			bool is_cluster = false;

			if (glyph_index == 0)
			{
				// Check to see if the glyph is the start of an unsupported multi-character cluster.
				int cluster_codepoint_count = 0;
				StringView cluster_string = GetCurrentClusterString(glyph_info, glyph_count, g, character, string, cluster_codepoint_count);

				if (cluster_codepoint_count > 1)
				{
					// Unsupported cluster detected; use fallback cluster glyph if one is available.
					const Vector<FontClusterGlyphData>* cluster_glyphs =
						GetOrAppendFallbackClusterGlyphs(cluster_string, text_shaping_context, registered_languages, shaping_features);
					
					if (cluster_glyphs)
					{
						extra_glyph_index_offset = cluster_codepoint_count - 1;
						is_cluster = true;

						for (const auto& cluster_glyph : *cluster_glyphs)
							glyph_queue.emplace(cluster_glyph.glyph_index, FontGlyphReference{&cluster_glyph.glyph_data.bitmap, cluster_glyph.glyph_data.character});
					}
				}
			}

			if (glyph_queue.empty())
			{
				const FontGlyph* glyph = GetOrAppendGlyph(glyph_index, character);
				if (glyph)
					glyph_queue.emplace(glyph_index, FontGlyphReference{glyph, character});
			}

			while (!glyph_queue.empty())
			{
				auto& glyph_pair = glyph_queue.front();

				ColourbPremultiplied glyph_color = layer_colour;
				// Use white vertex colors on RGB glyphs.
				if (layer == base_layer && glyph_pair.second.bitmap->color_format == ColorFormat::RGBA8)
					glyph_color = ColourbPremultiplied(layer_colour.alpha, layer_colour.alpha);

				Vector2f glyph_offset = {0.0f, 0.0f};
				glyph_offset += Vector2f{float(glyph_positions[g].x_offset >> 6), float(glyph_positions[g].y_offset >> 6)};
				Vector2f glyph_geometry_position = Vector2f{position.x + line_width, position.y} + glyph_offset;
				layer->GenerateGeometry(&mesh_list[geometry_index], glyph_pair.first, glyph_pair.second.character, is_cluster, glyph_geometry_position, glyph_color);

				// Adjust the cursor for this character's advance.
				if (glyph_info[g].codepoint != 0)
					line_width += glyph_positions[g].x_advance >> 6;
				else
					// Use the unshaped advance for unsupported characters.
					line_width += glyph_pair.second.bitmap->advance;

				line_width += (int)text_shaping_context.letter_spacing;

				glyph_queue.pop();
			}

			g += extra_glyph_index_offset;
		}

		geometry_index += num_textures;
	}

	hb_buffer_destroy(shaping_buffer);

	return Rml::Math::Max(line_width, 0);
}

bool FontFaceHandleHarfBuzz::UpdateLayersOnDirty()
{
	bool result = false;

	// If we are dirty, regenerate all the layers and increment the version
	if (is_layers_dirty && base_layer)
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

int FontFaceHandleHarfBuzz::GetVersion() const
{
	return version;
}

bool FontFaceHandleHarfBuzz::AppendGlyph(FontGlyphIndex glyph_index, Character character)
{
	bool result = FreeType::AppendGlyph(ft_face, metrics.size, glyph_index, character, glyphs);
	return result;
}

bool FontFaceHandleHarfBuzz::AppendFallbackGlyph(Character& character)
{
	const int num_fallback_faces = FontProvider::CountFallbackFontFaces();
	for (int i = 0; i < num_fallback_faces; i++)
	{
		FontFaceHandleHarfBuzz* fallback_face = FontProvider::GetFallbackFontFace(i, metrics.size);
		if (!fallback_face || fallback_face == this)
			continue;

		const FontGlyphIndex character_index = FreeType::GetGlyphIndexFromCharacter(fallback_face->ft_face, character);
		if (character_index == 0)
			continue;

		const FontGlyph* glyph = fallback_face->GetOrAppendGlyph(character_index, character, false);
		if (glyph)
		{
			// Insert the new glyph into our own set of fallback glyphs
			auto pair = fallback_glyphs.emplace(character, glyph->WeakCopy());
			if (pair.second)
				is_layers_dirty = true;

			return true;
		}
	}

	return false;
}

const FontGlyph* FontFaceHandleHarfBuzz::GetOrAppendGlyph(FontGlyphIndex glyph_index, Character& character, bool look_in_fallback_fonts)
{
	if (glyph_index == 0 && look_in_fallback_fonts && character != Character::Replacement)
	{
		auto fallback_glyph = GetOrAppendFallbackGlyph(character);
		if (fallback_glyph != nullptr)
			return fallback_glyph;
	}

	auto glyph_location = glyphs.find(glyph_index);
	if (glyph_location == glyphs.cend())
	{
		bool result = false;
		if (glyph_index != 0)
			result = AppendGlyph(glyph_index, character);

		if (result)
		{
			glyph_location = glyphs.find(glyph_index);
			if (glyph_location == glyphs.cend())
			{
				RMLUI_ERROR;
				return nullptr;
			}

			is_layers_dirty = true;
		}
		else
			return nullptr;
	}

	if (glyph_index == 0)
		character = Character::Replacement;
	else if (character != glyph_location->second.character)
		character = glyph_location->second.character;

	const FontGlyph* glyph = &glyph_location->second.bitmap;
	return glyph;
}

const FontGlyph* FontFaceHandleHarfBuzz::GetOrAppendFallbackGlyph(Character& character)
{
	auto fallback_glyph_location = fallback_glyphs.find(character);
	if (fallback_glyph_location != fallback_glyphs.cend())
		return &fallback_glyph_location->second;

	bool result = AppendFallbackGlyph(character);

	if (result)
	{
		fallback_glyph_location = fallback_glyphs.find(character);
		if (fallback_glyph_location == fallback_glyphs.cend())
		{
			RMLUI_ERROR;
			return nullptr;
		}

		is_layers_dirty = true;
	}
	else
		return nullptr;

	const FontGlyph* fallback_glyph = &fallback_glyph_location->second;
	return fallback_glyph;
}

bool FontFaceHandleHarfBuzz::AppendFallbackClusterGlyphs(StringView cluster, const TextShapingContext& text_shaping_context,
	const LanguageDataMap& registered_languages, Span<struct hb_feature_t> text_shaping_features)
{
	const hb_feature_t* shaping_features_pointer = !text_shaping_features.empty() ? text_shaping_features.data() : nullptr;
	hb_buffer_t* shaping_buffer = hb_buffer_create();
	RMLUI_ASSERT(shaping_buffer != nullptr);

	TextFlowDirection text_direction = TextFlowDirection::LeftToRight;

	// Iterate through all available fallback font faces.
	const int num_fallback_faces = FontProvider::CountFallbackFontFaces();
	for (int i = 0; i < num_fallback_faces; i++)
	{
		FontFaceHandleHarfBuzz* fallback_face = FontProvider::GetFallbackFontFace(i, metrics.size);
		if (!fallback_face || fallback_face == this)
			continue;

		// Insert the cluster into a shaping buffer and perform text shaping.
		hb_buffer_clear_contents(shaping_buffer);
		ConfigureTextShapingBuffer(shaping_buffer, cluster, text_shaping_context, registered_languages, &text_direction);
		hb_buffer_add_utf8(shaping_buffer, cluster.begin(), (int)cluster.size(), 0, (int)cluster.size());
		hb_shape(fallback_face->hb_font, shaping_buffer, shaping_features_pointer, (unsigned int)text_shaping_features.size());

		unsigned int glyph_count = 0;
		hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(shaping_buffer, &glyph_count);
		if (glyph_count == 0)
			continue;

		Vector<FontClusterGlyphData> cluster_glyphs;
		cluster_glyphs.reserve((size_t)glyph_count);

		int glyph_info_index_offset = text_direction == TextFlowDirection::RightToLeft ? (int)glyph_count - 1 : 0;
		int cluster_string_offset = 0;
		bool has_supported_glyph = false;

		// Create the cluster glyphs.
		for (int g = 0; g < (int)glyph_count; ++g)
		{
			int glyph_info_index = g + glyph_info_index_offset;
			RMLUI_ASSERT(glyph_info_index < (int)glyph_count);

			// Reverse the order of the glyphs in right-to-left text.
			if (text_direction == TextFlowDirection::RightToLeft)
				glyph_info_index_offset -= 2;

			Character character = Rml::StringUtilities::ToCharacter(cluster.begin() + cluster_string_offset, cluster.end());
			const FontGlyph* glyph = fallback_face->GetOrAppendGlyph(glyph_info[glyph_info_index].codepoint, character, false);
			if (glyph && glyph->bitmap_data && glyph->bitmap_dimensions.x > 0 && glyph->bitmap_dimensions.y > 0)
			{
				cluster_glyphs.push_back(FontClusterGlyphData{glyph_info[glyph_info_index].codepoint, FontGlyphData{glyph->WeakCopy(), character}});
				if (!has_supported_glyph && glyph_info[glyph_info_index].codepoint != 0)
					has_supported_glyph = true;
			}

			cluster_string_offset += (int)Rml::StringUtilities::BytesUTF8(character);
			RMLUI_ASSERT(cluster_string_offset <= (int)cluster.size());
		}

		if (cluster_glyphs.empty() || !has_supported_glyph)
			continue;

		// Insert the cluster glyphs into our own set of fallback cluster glyphs.
		auto pair = fallback_cluster_glyphs.emplace(cluster, std::move(cluster_glyphs));
		if (pair.second)
		{
			is_layers_dirty = true;

			// Populate quick-lookup glyph map to glyph search times during rendering.
			for (const auto& cluster_glyph : pair.first->second)
			{
				uint64_t cluster_glyph_id = GetFallbackFontClusterGlyphLookupID(cluster_glyph.glyph_index, cluster_glyph.glyph_data.character);
				fallback_cluster_glyphs_lookup.emplace(cluster_glyph_id, &cluster_glyph.glyph_data.bitmap);
			}
		}

		return true;
	}

	return false;
}

const Vector<FontClusterGlyphData>* FontFaceHandleHarfBuzz::GetOrAppendFallbackClusterGlyphs(StringView cluster,
	const TextShapingContext& text_shaping_context, const LanguageDataMap& registered_languages, Span<struct hb_feature_t> text_shaping_features)
{
	String cluster_string(cluster);
	auto fallback_cluster_glyphs_location = fallback_cluster_glyphs.find(cluster_string);
	if (fallback_cluster_glyphs_location != fallback_cluster_glyphs.cend())
	{
		return &fallback_cluster_glyphs_location->second;
	}

	bool result = AppendFallbackClusterGlyphs(cluster, text_shaping_context, registered_languages, text_shaping_features);

	if (result)
	{
		fallback_cluster_glyphs_location = fallback_cluster_glyphs.find(cluster_string);
		if (fallback_cluster_glyphs_location == fallback_cluster_glyphs.cend())
		{
			RMLUI_ERROR;
			return nullptr;
		}

		is_layers_dirty = true;
	}
	else
		return nullptr;

	const Vector<FontClusterGlyphData>* fallback_cluster_glyphs = &fallback_cluster_glyphs_location->second;
	return fallback_cluster_glyphs;
}

FontFaceLayer* FontFaceHandleHarfBuzz::GetOrCreateLayer(const SharedPtr<const FontEffect>& font_effect)
{
	// Search for the font effect layer first, it may have been instanced before as part of a different configuration.
	const FontEffect* font_effect_ptr = font_effect.get();
	auto it =
		std::find_if(layers.begin(), layers.end(), [font_effect_ptr](const EffectLayerPair& pair) { return pair.font_effect == font_effect_ptr; });

	if (it != layers.end())
		return it->layer.get();

	// No existing effect matches, generate a new layer for the effect.
	layers.push_back(EffectLayerPair{font_effect_ptr, nullptr});
	auto& layer = layers.back().layer;

	layer = Rml::MakeUnique<FontFaceLayer>(font_effect);
	GenerateLayer(layer.get());

	return layer.get();
}

bool FontFaceHandleHarfBuzz::GenerateLayer(FontFaceLayer* layer)
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

void FontFaceHandleHarfBuzz::ConfigureTextShapingBuffer(hb_buffer_t* shaping_buffer, StringView string,
	const TextShapingContext& text_shaping_context, const LanguageDataMap& registered_languages, TextFlowDirection* determined_text_direction) const
{
	// Set the buffer's language based on the value of the element's 'lang' attribute.
	hb_buffer_set_language(shaping_buffer, hb_language_from_string(text_shaping_context.language.c_str(), -1));

	// Set the buffer's script.
	hb_script_t script = HB_SCRIPT_UNKNOWN;
	auto registered_language_location = registered_languages.find(text_shaping_context.language);
	if (registered_language_location != registered_languages.cend())
		// Get script from registered language data.
		script = hb_script_from_string(registered_language_location->second.script_code.c_str(), -1);
	else
	{
		// Try to guess script from the first character of the string.
		hb_unicode_funcs_t* unicode_functions = hb_unicode_funcs_get_default();
		if (unicode_functions != nullptr && !string.empty())
		{
			Character first_character = Rml::StringUtilities::ToCharacter(string.begin(), string.end());
			script = hb_unicode_script(unicode_functions, (hb_codepoint_t)first_character);
		}
	}

	hb_buffer_set_script(shaping_buffer, script);

	// Set the buffer's text-flow direction based on the value of the element's 'dir' attribute.
	hb_direction_t text_direction = HB_DIRECTION_LTR;
	switch (text_shaping_context.text_direction)
	{
	case Rml::Style::Direction::Auto:
		if (registered_language_location != registered_languages.cend())
			// Automatically determine the text-flow direction from the registered language.
			switch (registered_language_location->second.text_flow_direction)
			{
			case TextFlowDirection::LeftToRight: text_direction = HB_DIRECTION_LTR; break;
			case TextFlowDirection::RightToLeft: text_direction = HB_DIRECTION_RTL; break;
			}
		else
		{
			// Language not registered; determine best text-flow direction based on script.
			text_direction = hb_script_get_horizontal_direction(script);
			if (text_direction == HB_DIRECTION_INVALID)
				// Some scripts support both horizontal directions of text flow; default to left-to-right.
				text_direction = HB_DIRECTION_LTR;
		}
		break;

	case Rml::Style::Direction::Ltr: text_direction = HB_DIRECTION_LTR; break;
	case Rml::Style::Direction::Rtl: text_direction = HB_DIRECTION_RTL; break;
	}

	RMLUI_ASSERT(text_direction == HB_DIRECTION_LTR || text_direction == HB_DIRECTION_RTL);
	hb_buffer_set_direction(shaping_buffer, text_direction);
	if (determined_text_direction)
		*determined_text_direction = text_direction == HB_DIRECTION_LTR ? TextFlowDirection::LeftToRight : TextFlowDirection::RightToLeft;

	// Set buffer flags for additional text-shaping configuration.
	int buffer_flags = HB_BUFFER_FLAG_DEFAULT | HB_BUFFER_FLAG_BOT | HB_BUFFER_FLAG_EOT;

#if HB_VERSION_ATLEAST(5, 1, 0)
	if (script == HB_SCRIPT_ARABIC)
		buffer_flags |= HB_BUFFER_FLAG_PRODUCE_SAFE_TO_INSERT_TATWEEL;
#endif
#if defined(RMLUI_DEBUG) && HB_VERSION_ATLEAST(3, 4, 0)
	buffer_flags |= HB_BUFFER_FLAG_VERIFY;
#endif

	hb_buffer_set_flags(shaping_buffer, (hb_buffer_flags_t)buffer_flags);
}

StringView FontFaceHandleHarfBuzz::GetCurrentClusterString(const hb_glyph_info_t* glyph_info, int glyph_count, int glyph_index,
	Character first_character, StringView string, int& cluster_codepoint_count) const
{
	unsigned int cluster_index = glyph_info[glyph_index].cluster;
	cluster_codepoint_count = 1;
	int cluster_offset = glyph_index + 1;
	int cluster_string_size = (int)Rml::StringUtilities::BytesUTF8(first_character);

	// Continue counting characters that are part of the same cluster.
	while (cluster_offset < (int)glyph_count && glyph_info[cluster_offset].cluster == cluster_index)
	{
		Character current_cluster_character =
			Rml::StringUtilities::ToCharacter(string.begin() + glyph_info[glyph_index].cluster + cluster_string_size, string.end());
		cluster_string_size += (int)Rml::StringUtilities::BytesUTF8(current_cluster_character);

		++cluster_codepoint_count;
		++cluster_offset;
	}

	return StringView(string.begin() + glyph_info[glyph_index].cluster, string.begin() + glyph_info[glyph_index].cluster + cluster_string_size);
}
