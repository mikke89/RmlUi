#pragma once

#include "TextureLayoutRectangle.h"
#include <RmlUi/Core.h>

using Rml::byte;
using Rml::Vector;

class TextureLayout;

/**
    A texture layout row is a single row of rectangles positioned vertically within a texture.

    Modified to support 64-bit IDs.
 */

class TextureLayoutRow {
public:
	TextureLayoutRow();
	~TextureLayoutRow();

	/// Attempts to position unplaced rectangles from the layout into this row.
	/// @param[in] layout The layout to position rectangles from.
	/// @param[in] width The maximum width of this row.
	/// @param[in] y The y-coordinate of this row.
	/// @return The number of placed rectangles.
	int Generate(TextureLayout& layout, int width, int y);

	/// Assigns allocated texture data to all rectangles in this row.
	/// @param[in] texture_data The pointer to the beginning of the texture's data.
	/// @param[in] stride The stride of the texture's surface, in bytes;
	void Allocate(byte* texture_data, int stride);

	/// Returns the height of the row.
	/// @return The row's height.
	int GetHeight() const;

	/// Resets the placed status for all of the rectangles within this row.
	void Unplace();

private:
	using RectangleList = Vector<TextureLayoutRectangle*>;

	int height;
	RectangleList rectangles;
};
