#include "FontFace.h"
#include "FontEngineDefault/FreeTypeInterface.h"

FontFace::FontFace(FontFaceHandleFreetype _face, Style::FontStyle _style, Style::FontWeight _weight)
{
	style = _style;
	weight = _weight;
	face = _face;
}

FontFace::~FontFace()
{
	if (face)
		Rml::FreeType::ReleaseFace(face);
}

Style::FontStyle FontFace::GetStyle() const
{
	return style;
}

Style::FontWeight FontFace::GetWeight() const
{
	return weight;
}

FontFaceHandleHarfBuzz* FontFace::GetHandle(int size, bool load_default_glyphs)
{
	auto it = handles.find(size);
	if (it != handles.end())
		return it->second.get();

	// See if this face has been released.
	if (!face)
	{
		Rml::Log::Message(Rml::Log::LT_WARNING, "Font face has been released, unable to generate new handle.");
		return nullptr;
	}

	// Construct and initialise the new handle.
	auto handle = Rml::MakeUnique<FontFaceHandleHarfBuzz>();
	if (!handle->Initialize(face, size, load_default_glyphs))
	{
		handles[size] = nullptr;
		return nullptr;
	}

	FontFaceHandleHarfBuzz* result = handle.get();

	// Save the new handle to the font face
	handles[size] = std::move(handle);

	return result;
}

void FontFace::ReleaseFontResources()
{
	HandleMap().swap(handles);
}
