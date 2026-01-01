#include "FontEngineInterfaceBitmap.h"
#include "FontEngineBitmap.h"
#include <RmlUi/Core.h>

void FontEngineInterfaceBitmap::Initialize()
{
	FontProviderBitmap::Initialise();
}

void FontEngineInterfaceBitmap::Shutdown()
{
	FontProviderBitmap::Shutdown();
}

bool FontEngineInterfaceBitmap::LoadFontFace(const String& file_name, int /*face_index*/, bool /*fallback_face*/, FontWeight /*weight*/)
{
	return FontProviderBitmap::LoadFontFace(file_name);
}

bool FontEngineInterfaceBitmap::LoadFontFace(Span<const byte> /*data*/, int /*face_index*/, const String& font_family, FontStyle /*style*/, FontWeight /*weight*/,
	bool /*fallback_face*/)
{
	// We return 'true' here to allow the debugger to continue loading, but we will use our own fonts when it asks for a handle.
	// The debugger might look a bit off with our own fonts, but hey it works.
	if (font_family == "rmlui-debugger-font")
		return true;

	return false;
}

FontFaceHandle FontEngineInterfaceBitmap::GetFontFaceHandle(const String& family, FontStyle style, FontWeight weight, int size)
{
	auto handle = FontProviderBitmap::GetFontFaceHandle(family, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle);
}

FontEffectsHandle FontEngineInterfaceBitmap::PrepareFontEffects(FontFaceHandle /*handle*/, const FontEffectList& /*font_effects*/)
{
	// Font effects are not rendered in this implementation.
	return 0;
}

const FontMetrics& FontEngineInterfaceBitmap::GetFontMetrics(FontFaceHandle handle)
{
	auto handle_bitmap = reinterpret_cast<FontFaceBitmap*>(handle);
	return handle_bitmap->GetMetrics();
}

int FontEngineInterfaceBitmap::GetStringWidth(FontFaceHandle handle, StringView string, const TextShapingContext& /*text_shaping_context*/,
	Character prior_character)
{
	auto handle_bitmap = reinterpret_cast<FontFaceBitmap*>(handle);
	return handle_bitmap->GetStringWidth(string, prior_character);
}

int FontEngineInterfaceBitmap::GenerateString(RenderManager& render_manager, FontFaceHandle handle, FontEffectsHandle /*font_effects_handle*/,
	StringView string, Vector2f position, ColourbPremultiplied colour, float /*opacity*/, const TextShapingContext& /*text_shaping_context*/,
	TexturedMeshList& mesh_list)
{
	auto handle_bitmap = reinterpret_cast<FontFaceBitmap*>(handle);
	return handle_bitmap->GenerateString(render_manager, string, position, colour, mesh_list);
}

int FontEngineInterfaceBitmap::GetVersion(FontFaceHandle /*handle*/)
{
	return 0;
}
