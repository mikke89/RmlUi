#pragma once

#include "TextureLayoutRow.h"
#include <RmlUi/Core.h>

using Rml::byte;
using Rml::Vector;

class TextureLayout;

/**
    A texture layout texture is a single rectangular area which sub-rectangles are placed on within
    a complete texture layout.

    Modified to support 64-bit IDs.
 */

class TextureLayoutTexture {
public:
	TextureLayoutTexture();
	~TextureLayoutTexture();

	/// Returns the texture's dimensions. This is only valid after the texture has been generated.
	/// @return The texture's dimensions.
	Vector2i GetDimensions() const;

	/// Attempts to position unplaced rectangles from the layout into this texture. The size of
	/// this texture will be determined by its contents.
	/// @param[in] layout The layout to position rectangles from.
	/// @param[in] maximum_dimensions The maximum dimensions of this texture. If this is not big enough to place all the rectangles, then as many will
	/// be placed as possible.
	/// @return The number of placed rectangles.
	int Generate(TextureLayout& layout, int maximum_dimensions);

	/// Allocates the texture.
	/// @return The allocated texture data.
	Vector<byte> AllocateTexture();

private:
	using RowList = Vector<TextureLayoutRow>;

	Vector2i dimensions;
	RowList rows;
};
