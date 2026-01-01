#pragma once

#include <RmlUi/Core.h>

using Rml::byte;
using Rml::Vector2i;

/**
    A texture layout rectangle is an area positioned with a texture layout.

    Modified to support 64-bit IDs.
 */

class TextureLayoutRectangle {
public:
	TextureLayoutRectangle(uint64_t id, Vector2i dimensions);
	~TextureLayoutRectangle();

	/// Returns the rectangle's id.
	/// @return The rectangle's id.
	uint64_t GetId() const;
	/// Returns the rectangle's position; this is only valid if it has been placed.
	/// @return The rectangle's position within its texture.
	Vector2i GetPosition() const;
	/// Returns the rectangle's dimensions.
	/// @return The rectangle's dimensions.
	Vector2i GetDimensions() const;

	/// Places the rectangle within a texture.
	/// @param[in] texture_index The index of the texture this rectangle is placed on.
	/// @param[in] position The position within the texture of this rectangle's top-left corner.
	void Place(int texture_index, Vector2i position);
	/// Unplaces the rectangle.
	void Unplace();
	/// Returns the rectangle's placed state.
	/// @return True if the rectangle has been placed, false if not.
	bool IsPlaced() const;

	/// Sets the rectangle's texture data and stride.
	/// @param[in] texture_data The pointer to the top-left corner of the texture's data.
	/// @param[in] texture_stride The stride of the texture data, in bytes.
	void Allocate(byte* texture_data, int texture_stride);

	/// Returns the index of the texture this rectangle is placed on.
	/// @return The texture index.
	int GetTextureIndex();
	/// Returns the rectangle's allocated texture data.
	/// @return The texture data.
	byte* GetTextureData();
	/// Returns the stride of the rectangle's texture data.
	/// @return The texture data stride.
	int GetTextureStride() const;

private:
	uint64_t id;
	Vector2i dimensions;

	int texture_index;
	Vector2i texture_position;

	byte* texture_data;
	int texture_stride;
};
