#pragma once

#include "../../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../../Include/RmlUi/Core/FontGlyph.h"
#include "../../../Include/RmlUi/Core/Geometry.h"
#include "../../../Include/RmlUi/Core/MeshUtilities.h"
#include "../TextureLayout.h"

namespace Rml {

class FontEffect;
class FontFaceHandleDefault;

/**
    A textured layer stored as part of a font face handle. Each handle will have at least a base
    layer for the standard font. Further layers can be added to allow rendering of text effects.
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
	bool Generate(const FontFaceHandleDefault* handle, const FontFaceLayer* clone = nullptr, bool clone_glyph_origins = false);

	/// Generates the texture data for a layer (for the texture database).
	/// @param[out] texture_data The generated texture data.
	/// @param[out] texture_dimensions The dimensions of the texture.
	/// @param[in] texture_id The index of the texture within the layer to generate.
	/// @param[in] glyphs The glyphs required by the font face handle.
	bool GenerateTexture(Vector<byte>& texture_data, Vector2i& texture_dimensions, int texture_id, const FontGlyphMap& glyphs);

	/// Generates the geometry required to render a single character.
	/// @param[out] mesh_list An array of meshes this layer will write to. It must be at least as big as the number of textures in this layer.
	/// @param[in] character_code The character to generate geometry for.
	/// @param[in] position The position of the baseline.
	/// @param[in] colour The colour of the string.
	inline void GenerateGeometry(TexturedMesh* mesh_list, const Character character_code, const Vector2f position,
		const ColourbPremultiplied colour) const
	{
		auto it = character_boxes.find(character_code);
		if (it == character_boxes.end())
			return;

		const TextureBox& box = it->second;

		if (box.texture_index < 0)
			return;

		// Generate the geometry for the character.
		Mesh& mesh = mesh_list[box.texture_index].mesh;
		MeshUtilities::GenerateQuad(mesh, (position + box.origin).Round(), box.dimensions, colour, box.texcoords[0], box.texcoords[1]);
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

	using CharacterMap = UnorderedMap<Character, TextureBox>;
	using TextureList = Vector<CallbackTextureSource>;

	SharedPtr<const FontEffect> effect;

	TextureList textures_owned;
	TextureList* textures_ptr = &textures_owned;

	TextureLayout texture_layout;
	CharacterMap character_boxes;
	Colourb colour;
};

} // namespace Rml
