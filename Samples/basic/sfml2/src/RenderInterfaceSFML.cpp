/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 Nuno Silva
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <RmlUi/Core.h>
#include "RenderInterfaceSFML.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifdef ENABLE_GLEW
class RmlUiSFMLRendererGeometryHandler
{
public:
	GLuint VertexID, IndexID;
	int NumVertices;
	Rml::TextureHandle Texture;

	RmlUiSFMLRendererGeometryHandler() : VertexID(0), IndexID(0), NumVertices(0), Texture(0)
	{
	};

	~RmlUiSFMLRendererGeometryHandler()
	{
		if(VertexID)
			glDeleteBuffers(1, &VertexID);

		if(IndexID)
			glDeleteBuffers(1, &IndexID);

		VertexID = IndexID = 0;
	};
};
#endif

struct RmlUiSFMLRendererVertex
{
	sf::Vector2f Position, TexCoord;
	sf::Color Color;
};

RmlUiSFMLRenderer::RmlUiSFMLRenderer()
{
}

void RmlUiSFMLRenderer::SetWindow(sf::RenderWindow *Window)
{
	MyWindow = Window;
}

sf::RenderWindow *RmlUiSFMLRenderer::GetWindow()
{
	return MyWindow;
}

// Called by RmlUi when it wants to render geometry that it does not wish to optimise.
void RmlUiSFMLRenderer::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
	MyWindow->pushGLStates();
	initViewport();

	glTranslatef(translation.x, translation.y, 0);

	Rml::Vector<Rml::Vector2f> Positions(num_vertices);
	Rml::Vector<Rml::Colourb> Colors(num_vertices);
	Rml::Vector<Rml::Vector2f> TexCoords(num_vertices);

	for(int i = 0; i < num_vertices; i++)
	{
		Positions[i] = vertices[i].position;
		Colors[i] = vertices[i].colour;
		TexCoords[i] = vertices[i].tex_coord;
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, &Positions[0]);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, &Colors[0]);
	glTexCoordPointer(2, GL_FLOAT, 0, &TexCoords[0]);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	sf::Texture *sfTexture = (sf::Texture *)texture;

	if(sfTexture)
	{
		sf::Texture::bind(sfTexture);
	}
	else
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, 0);
	};

	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, indices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glColor4f(1, 1, 1, 1);

	MyWindow->popGLStates();
}

// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.		
Rml::CompiledGeometryHandle RmlUiSFMLRenderer::CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture)
{
#ifdef ENABLE_GLEW
	Rml::Vector<RmlUiSFMLRendererVertex> Data(num_vertices);

	for(std::size_t i = 0; i < Data.size(); i++)
	{
		Data[i].Position = sf::Vector2f(vertices[i].position.x, vertices[i].position.y);
		Data[i].TexCoord = sf::Vector2f(vertices[i].tex_coord.x, vertices[i].tex_coord.y);
		Data[i].Color = sf::Color(vertices[i].colour.red, vertices[i].colour.green,
			vertices[i].colour.blue, vertices[i].colour.alpha);
	};

	RmlUiSFMLRendererGeometryHandler *Geometry = new RmlUiSFMLRendererGeometryHandler();
	Geometry->NumVertices = num_indices;

	glGenBuffers(1, &Geometry->VertexID);
	glBindBuffer(GL_ARRAY_BUFFER, Geometry->VertexID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(RmlUiSFMLRendererVertex) * num_vertices, &Data[0],
		GL_STATIC_DRAW);

	glGenBuffers(1, &Geometry->IndexID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Geometry->IndexID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * num_indices, indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Geometry->Texture = texture;

	return (Rml::CompiledGeometryHandle)Geometry;
#else
	return (Rml::CompiledGeometryHandle)nullptr;
#endif
}

// Called by RmlUi when it wants to render application-compiled geometry.		
void RmlUiSFMLRenderer::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation)
{
#ifdef ENABLE_GLEW
	RmlUiSFMLRendererGeometryHandler *RealGeometry = (RmlUiSFMLRendererGeometryHandler *)geometry;

	MyWindow->pushGLStates();
	initViewport();

	glTranslatef(translation.x, translation.y, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	sf::Texture *texture = (sf::Texture *)RealGeometry->Texture;

	if(texture)
	{
		sf::Texture::bind(texture);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	};

	glEnable(GL_VERTEX_ARRAY);
	glEnable(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_COLOR_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, RealGeometry->VertexID);
	glVertexPointer(2, GL_FLOAT, sizeof(RmlUiSFMLRendererVertex), (const void*)0);
	glTexCoordPointer(2, GL_FLOAT, sizeof(RmlUiSFMLRendererVertex), (const void*)sizeof(sf::Vector2f));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RmlUiSFMLRendererVertex), (const void*)sizeof(sf::Vector2f[2]));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RealGeometry->IndexID);
	glDrawElements(GL_TRIANGLES, RealGeometry->NumVertices, GL_UNSIGNED_INT, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_COLOR_ARRAY);
	glDisable(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_VERTEX_ARRAY);

	glColor4f(1, 1, 1, 1);

	MyWindow->popGLStates();
#else
	RMLUI_ASSERT(false);
#endif
}

// Called by RmlUi when it wants to release application-compiled geometry.		
void RmlUiSFMLRenderer::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry)
{
#ifdef ENABLE_GLEW
	delete (RmlUiSFMLRendererGeometryHandler *)geometry;
#else
	RMLUI_ASSERT(false);
#endif
}

// Called by RmlUi when it wants to enable or disable scissoring to clip content.		
void RmlUiSFMLRenderer::EnableScissorRegion(bool enable)
{
	if (enable)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);
}

// Called by RmlUi when it wants to change the scissor region.		
void RmlUiSFMLRenderer::SetScissorRegion(int x, int y, int width, int height)
{
	glScissor(x, MyWindow->getSize().y - (y + height), width, height);
}

// Called by RmlUi when a texture is required by the library.		
bool RmlUiSFMLRenderer::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
		return false;

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);
	
	char* buffer = new char[buffer_size];
	file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);

	sf::Texture *texture = new sf::Texture();

	if(!texture->loadFromMemory(buffer, buffer_size))
	{
		delete[] buffer;
		delete texture;

		return false;
	};
	delete[] buffer;

	texture_handle = (Rml::TextureHandle) texture;
	texture_dimensions = Rml::Vector2i(texture->getSize().x, texture->getSize().y);

	return true;
}

// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
bool RmlUiSFMLRenderer::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
	sf::Texture *texture = new sf::Texture();

	if (!texture->create(source_dimensions.x, source_dimensions.y)) {
		delete texture;
		return false;
	}

	texture->update(source, source_dimensions.x, source_dimensions.y, 0, 0);
	texture_handle = (Rml::TextureHandle)texture;

	return true;
}

// Called by RmlUi when a loaded texture is no longer required.		
void RmlUiSFMLRenderer::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	delete (sf::Texture *)texture_handle;
}

void RmlUiSFMLRenderer::initViewport() {
	glViewport(0, 0, MyWindow->getSize().x, MyWindow->getSize().y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, MyWindow->getSize().x, MyWindow->getSize().y, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}
