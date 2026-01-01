#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/Types.h>

using Rml::FontEffectsHandle;
using Rml::FontFaceHandle;

using Rml::byte;
using Rml::Character;
using Rml::ColourbPremultiplied;
using Rml::Span;
using Rml::String;
using Rml::StringView;
using Rml::Texture;
using Rml::Vector2f;
using Rml::Vector2i;
using Rml::Style::FontStyle;
using Rml::Style::FontWeight;

using Rml::FontEffectList;
using Rml::FontMetrics;
using Rml::RenderManager;
using Rml::TextShapingContext;
using Rml::TexturedMeshList;

class FontEngineInterfaceBitmap : public Rml::FontEngineInterface {
public:
	/// Called when RmlUi is being initialized.
	void Initialize() override;

	/// Called when RmlUi is being shut down.
	void Shutdown() override;

	/// Called by RmlUi when it wants to load a font face from file.
	bool LoadFontFace(const String& file_name, int face_index, bool fallback_face, FontWeight weight) override;

	/// Called by RmlUi when it wants to load a font face from memory, registered using the provided family, style, and weight.
	/// @param[in] data A pointer to the data.
	bool LoadFontFace(Span<const byte> data, int face_index, const String& family, FontStyle style, FontWeight weight, bool fallback_face) override;

	/// Called by RmlUi when a font configuration is resolved for an element. Should return a handle that
	/// can later be used to resolve properties of the face, and generate string geometry to be rendered.
	FontFaceHandle GetFontFaceHandle(const String& family, FontStyle style, FontWeight weight, int size) override;

	/// Called by RmlUi when a list of font effects is resolved for an element with a given font face.
	FontEffectsHandle PrepareFontEffects(FontFaceHandle handle, const FontEffectList& font_effects) override;

	/// Should return the font metrics of the given font face.
	const FontMetrics& GetFontMetrics(FontFaceHandle handle) override;

	/// Called by RmlUi when it wants to retrieve the width of a string when rendered with this handle.
	int GetStringWidth(FontFaceHandle handle, StringView string, const TextShapingContext& text_shaping_context,
		Character prior_character = Character::Null) override;

	/// Called by RmlUi when it wants to retrieve the geometry required to render a single line of text.
	int GenerateString(RenderManager& render_manager, FontFaceHandle face_handle, FontEffectsHandle font_effects_handle, StringView string,
		Vector2f position, ColourbPremultiplied colour, float opacity, const TextShapingContext& text_shaping_context,
		TexturedMeshList& mesh_list) override;

	/// Called by RmlUi to determine if the text geometry is required to be re-generated.eometry.
	int GetVersion(FontFaceHandle handle) override;
};
