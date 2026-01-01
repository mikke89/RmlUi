#pragma once

#include "TextureLayoutRectangle.h"
#include "TextureLayoutTexture.h"
#include <RmlUi/Core.h>

/**
    A texture layout generates and stores a layout of rectangles within a series of textures. It is
    used primarily by the font system for generating font textures.

    Modified to support 64-bit IDs.
 */

using Rml::Vector;
using Rml::Vector2i;

class TextureLayout {
public:
	TextureLayout();
	~TextureLayout();

	/// Adds a rectangle to the list of rectangles to be laid out. All rectangles must be added to
	/// the layout before the layout is generated.
	/// @param[in] id The id of the rectangle; used to identify the rectangle after it has been positioned.
	/// @param[in] dimensions The dimensions of the rectangle.
	void AddRectangle(uint64_t id, Vector2i dimensions);

	/// Returns one of the layout's rectangles.
	/// @param[in] index The index of the desired rectangle.
	/// @return The desired rectangle.
	TextureLayoutRectangle& GetRectangle(int index);
	/// Returns the number of rectangles in the layout.
	/// @return The layout's rectangle count.
	int GetNumRectangles() const;

	/// Returns one of the layout's textures.
	/// @param[in] index The index of the desired texture.
	/// @return The desired texture.
	TextureLayoutTexture& GetTexture(int index);
	/// Returns the number of textures in the layout.
	/// @return The layout's texture count.
	int GetNumTextures() const;

	/// Attempts to generate an efficient texture layout for the rectangles.
	/// @param[in] max_texture_dimensions The maximum dimensions allowed for any single texture.
	/// @return True if the layout was generated successfully, false if not.
	bool GenerateLayout(int max_texture_dimensions);

private:
	using RectangleList = Vector<TextureLayoutRectangle>;
	using TextureList = Vector<TextureLayoutTexture>;

	TextureList textures;
	RectangleList rectangles;
};
