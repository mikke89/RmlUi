#pragma once

#include "FontGlyph.h"
#include "TextureLayout.h"
#include "TextureLayoutRectangle.h"
#include <RmlUi/Core.h>

using Rml::byte;
using Rml::CallbackTextureFunction;
using Rml::CallbackTextureInterface;
using Rml::CallbackTextureSource;
using Rml::Character;
using Rml::ColorFormat;
using Rml::Colourb;
using Rml::ColourbPremultiplied;
using Rml::FontEffect;
using Rml::FontGlyph;
using Rml::Geometry;
using Rml::Mesh;
using Rml::RenderManager;
using Rml::SharedPtr;
using Rml::Texture;
using Rml::TexturedMesh;
using Rml::UniquePtr;
using Rml::UnorderedMap;
using Rml::Vector;
using Rml::Vector2f;
using Rml::Vector2i;

class FontFaceHandleHarfBuzz;

/**
    A textured layer stored as part of a font face handle. Each handle will have at least a base
    layer for the standard font. Further layers can be added to allow rendering of text effects.

    Modified to support HarfBuzz text shaping.
 */

class FontFaceLayer {
public:
	FontFaceLayer(const SharedPtr<const FontEffect>& _effect);
	~FontFaceLayer();

	/// Generates or re-generates the character and texture data for the layer.
	/// @param[in] handle The handle generating this layer.
	/// @param[in] clone The layer to optionally clone geometry and texture data from.
	/// @param[in] clone_glyph_origins True to keep the character origins from the cloned layer, false to generate new ones.
	/// @return True if the layer was generated successfully, false if not.
	bool Generate(const FontFaceHandleHarfBuzz* handle, const FontFaceLayer* clone = nullptr, bool clone_glyph_origins = false);

	/// Generates the texture data for a layer (for the texture database).
	/// @param[out] texture_data The generated texture data.
	/// @param[out] texture_dimensions The dimensions of the texture.
	/// @param[in] texture_id The index of the texture within the layer to generate.
	/// @param[in] glyph_maps The glyph maps required by the font face handle.
	bool GenerateTexture(Vector<byte>& texture_data, Vector2i& texture_dimensions, int texture_id, const FontGlyphMaps& glyph_maps);

	/// Generates the geometry required to render a single character.
	/// @param[out] mesh_list An array of meshes this layer will write to. It must be at least as big as the number of textures in this layer.
	/// @param[in] character_code The character to generate geometry for.
	/// @param[in] is_cluster Whether the glyph is part of a cluster or not.
	/// @param[in] position The position of the baseline.
	/// @param[in] colour The colour of the string.
	inline void GenerateGeometry(TexturedMesh* mesh_list, const FontGlyphIndex glyph_index, const Character character_code, bool is_cluster,
		const Vector2f position, const ColourbPremultiplied colour) const
	{
		auto it = character_boxes.find(CreateFontGlyphID(glyph_index, character_code, is_cluster));
		if (it == character_boxes.end())
			return;

		const TextureBox& box = it->second;

		if (box.texture_index < 0)
			return;

		// Generate the geometry for the character.
		Mesh& mesh = mesh_list[box.texture_index].mesh;
		Rml::MeshUtilities::GenerateQuad(mesh, (position + box.origin).Round(), box.dimensions, colour, box.texcoords[0], box.texcoords[1]);
	}

	/// Returns the effect used to generate the layer.
	const FontEffect* GetFontEffect() const;

	/// Returns one of the layer's textures.
	Texture GetTexture(RenderManager& render_manager, int index);
	/// Returns the number of textures employed by this layer.
	int GetNumTextures() const;

	/// Returns the layer's colour after applying the given opacity.
	ColourbPremultiplied GetColour(float opacity) const;

private:
	/// Creates an ID for a font glyph from a glyph index and character codepoint.
	uint64_t CreateFontGlyphID(const FontGlyphIndex glyph_index, const Character character_code, bool is_cluster) const;

	/// Retrieves the font glyph index from a font glyph ID.
	FontGlyphIndex GetFontGlyphIndexFromID(const uint64_t glyph_id) const;

	/// Retrieves the character from a font glyph ID.
	Character GetCharacterCodepointFromID(const uint64_t glyph_id) const;

	/// Determines if a font glyph ID is part of a cluster instead of a single glyph.
	bool IsFontGlyphIDPartOfCluster(const uint64_t glyph_id) const;

	/// Creates a texture layout from the given glyph bitmap and data.
	void CreateTextureLayout(const FontGlyph& glyph, FontGlyphIndex glyph_index, Character glyph_character, bool is_cluster);

	/// Clones the given glyph bitmap and data into a texture box.
	void CloneTextureBox(const FontGlyph& glyph, FontGlyphIndex glyph_index, Character glyph_character, bool is_cluster);

	struct TextureBox {
		// The offset, in pixels, of the baseline from the start of this character's geometry.
		Vector2f origin;
		// The width and height, in pixels, of this character's geometry.
		Vector2f dimensions;
		// The texture coordinates for the character's geometry.
		Vector2f texcoords[2];

		// The texture this character renders from.
		int texture_index = -1;
	};

	using CharacterMap = UnorderedMap<uint64_t, TextureBox>;
	using TextureList = Vector<CallbackTextureSource>;

	static constexpr uint64_t font_glyph_id_cluster_bit_mask = 1ull << 31ull;

	SharedPtr<const FontEffect> effect;

	TextureList textures_owned;
	TextureList* textures_ptr = &textures_owned;

	TextureLayout texture_layout;
	CharacterMap character_boxes;
	Colourb colour;
};
