#include "TextureLayoutRectangle.h"

namespace Rml {

TextureLayoutRectangle::TextureLayoutRectangle(const int _id, const Vector2i dimensions) : dimensions(dimensions), texture_position(0, 0)
{
	id = _id;
	texture_index = -1;

	texture_data = nullptr;
	texture_stride = 0;
}

TextureLayoutRectangle::~TextureLayoutRectangle() {}

int TextureLayoutRectangle::GetId() const
{
	return id;
}

Vector2i TextureLayoutRectangle::GetPosition() const
{
	return texture_position;
}

Vector2i TextureLayoutRectangle::GetDimensions() const
{
	return dimensions;
}

void TextureLayoutRectangle::Place(const int _texture_index, const Vector2i position)
{
	texture_index = _texture_index;
	texture_position = position;
}

void TextureLayoutRectangle::Unplace()
{
	texture_index = -1;
}

bool TextureLayoutRectangle::IsPlaced() const
{
	return texture_index > -1;
}

void TextureLayoutRectangle::Allocate(byte* _texture_data, int _texture_stride)
{
	texture_data = _texture_data + ((texture_position.y * _texture_stride) + texture_position.x * 4);
	texture_stride = _texture_stride;
}

int TextureLayoutRectangle::GetTextureIndex()
{
	return texture_index;
}

byte* TextureLayoutRectangle::GetTextureData()
{
	return texture_data;
}

int TextureLayoutRectangle::GetTextureStride() const
{
	return texture_stride;
}

} // namespace Rml
