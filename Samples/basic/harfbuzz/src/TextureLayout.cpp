#include "TextureLayout.h"
#include <algorithm>

struct RectangleSort {
	bool operator()(const TextureLayoutRectangle& lhs, const TextureLayoutRectangle& rhs) const
	{
		return lhs.GetDimensions().y > rhs.GetDimensions().y;
	}
};

TextureLayout::TextureLayout() {}

TextureLayout::~TextureLayout() {}

void TextureLayout::AddRectangle(uint64_t id, Vector2i dimensions)
{
	rectangles.push_back(TextureLayoutRectangle(id, dimensions));
}

TextureLayoutRectangle& TextureLayout::GetRectangle(int index)
{
	RMLUI_ASSERT(index >= 0);
	RMLUI_ASSERT(index < GetNumRectangles());

	return rectangles[index];
}

int TextureLayout::GetNumRectangles() const
{
	return (int)rectangles.size();
}

TextureLayoutTexture& TextureLayout::GetTexture(int index)
{
	RMLUI_ASSERT(index >= 0);
	RMLUI_ASSERT(index < GetNumTextures());

	return textures[index];
}

int TextureLayout::GetNumTextures() const
{
	return (int)textures.size();
}

bool TextureLayout::GenerateLayout(int max_texture_dimensions)
{
	// Sort the rectangles by height.
	std::sort(rectangles.begin(), rectangles.end(), RectangleSort());

	int num_placed_rectangles = 0;
	while (num_placed_rectangles != GetNumRectangles())
	{
		TextureLayoutTexture texture;
		int texture_size = texture.Generate(*this, max_texture_dimensions);
		if (texture_size == 0)
			return false;

		textures.push_back(texture);
		num_placed_rectangles += texture_size;
	}

	return true;
}
