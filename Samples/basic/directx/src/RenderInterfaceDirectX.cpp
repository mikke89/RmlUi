/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "RenderInterfaceDirectX.h"
#include <Rocket/Core.h>
#include <d3dx9.h>

// This structure is created for each set of geometry that Rocket compiles. It stores the vertex and index buffers and
// the texture associated with the geometry, if one was specified.
struct RocketD3D9CompiledGeometry
{
	LPDIRECT3DVERTEXBUFFER9 vertices;
	DWORD num_vertices;

	LPDIRECT3DINDEXBUFFER9 indices;
	DWORD num_primitives;

	LPDIRECT3DTEXTURE9 texture;
};

// The internal format of the vertex we use for rendering Rocket geometry. We could optimise space by having a second
// untextured vertex for use when rendering coloured borders and backgrounds.
struct RocketD3D9Vertex
{
	FLOAT x, y, z;
	DWORD colour;
	FLOAT u, v;
};

DWORD vertex_fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

RenderInterfaceDirectX::RenderInterfaceDirectX()
{
	g_pD3D = NULL;
	g_pd3dDevice = NULL;
	m_rocket_context = NULL;
}

RenderInterfaceDirectX::~RenderInterfaceDirectX()
{
}

// Called by Rocket when it wants to render geometry that it does not wish to optimise.
void RenderInterfaceDirectX::RenderGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation)
{
	/// @TODO We've chosen to not support non-compiled geometry in the DirectX renderer. If you wanted to render non-compiled
	/// geometry, for example for very small sections of geometry, you could use DrawIndexedPrimitiveUP or write to a
	/// dynamic vertex buffer which is flushed when either the texture changes or compiled geometry is drawn.

	if(g_pd3dDevice == NULL)
	{
		return;
	}

	/// @TODO, HACK, just use the compiled geometry framework for now, this is inefficient but better than absolutely nothing
	/// for the time being
	Rocket::Core::CompiledGeometryHandle geom = this->CompileGeometry(vertices, num_vertices, indices, num_indices, texture);
	this->RenderCompiledGeometry(geom, translation);
	this->ReleaseCompiledGeometry(geom);
}

// Called by Rocket when it wants to compile geometry it believes will be static for the forseeable future.
Rocket::Core::CompiledGeometryHandle RenderInterfaceDirectX::CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture)
{
	if(g_pd3dDevice == NULL)
	{
		return false;
	}

	// Construct a new RocketD3D9CompiledGeometry structure, which will be returned as the handle, and the buffers to
	// store the geometry.
	RocketD3D9CompiledGeometry* geometry = new RocketD3D9CompiledGeometry();
	g_pd3dDevice->CreateVertexBuffer(num_vertices * sizeof(RocketD3D9Vertex), D3DUSAGE_WRITEONLY, vertex_fvf, D3DPOOL_DEFAULT, &geometry->vertices, NULL);
	g_pd3dDevice->CreateIndexBuffer(num_indices * sizeof(unsigned int), D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &geometry->indices, NULL);

	// Fill the vertex buffer.
	RocketD3D9Vertex* d3d9_vertices;
	geometry->vertices->Lock(0, 0, (void**) &d3d9_vertices, 0);
	for (int i = 0; i < num_vertices; ++i)
	{
		d3d9_vertices[i].x = vertices[i].position.x;
		d3d9_vertices[i].y = vertices[i].position.y;
		d3d9_vertices[i].z = 0;

		d3d9_vertices[i].colour = D3DCOLOR_RGBA(vertices[i].colour.red, vertices[i].colour.green, vertices[i].colour.blue, vertices[i].colour.alpha);

		d3d9_vertices[i].u = vertices[i].tex_coord[0];
		d3d9_vertices[i].v = vertices[i].tex_coord[1];
	}
	geometry->vertices->Unlock();

	// Fill the index buffer.
	unsigned int* d3d9_indices;
	geometry->indices->Lock(0, 0, (void**) &d3d9_indices, 0);
	memcpy(d3d9_indices, indices, sizeof(unsigned int) * num_indices);
	geometry->indices->Unlock();

	geometry->num_vertices = (DWORD) num_vertices;
	geometry->num_primitives = (DWORD) num_indices / 3;

	geometry->texture = texture == NULL ? NULL : (LPDIRECT3DTEXTURE9) texture;

	return (Rocket::Core::CompiledGeometryHandle)geometry;
}

// Called by Rocket when it wants to render application-compiled geometry.
void RenderInterfaceDirectX::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f& translation)
{
	if(g_pd3dDevice == NULL)
	{
		return;
	}

	// Build and set the transform matrix.
	D3DXMATRIX world_transform;
	D3DXMatrixTranslation(&world_transform, translation.x, translation.y, 0);
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &world_transform);

	RocketD3D9CompiledGeometry* d3d9_geometry = (RocketD3D9CompiledGeometry*) geometry;

	// Set the vertex format for the Rocket vertices, and bind the vertex and index buffers.
	g_pd3dDevice->SetFVF(vertex_fvf);
	g_pd3dDevice->SetStreamSource(0, d3d9_geometry->vertices, 0, sizeof(RocketD3D9Vertex));
	g_pd3dDevice->SetIndices(d3d9_geometry->indices);

	// Set the texture, if this geometry has one.
	if (d3d9_geometry->texture != NULL)
		g_pd3dDevice->SetTexture(0, d3d9_geometry->texture);
	else
		g_pd3dDevice->SetTexture(0, NULL);

	// Draw the primitives.
	g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, d3d9_geometry->num_vertices, 0, d3d9_geometry->num_primitives);
}

// Called by Rocket when it wants to release application-compiled geometry.
void RenderInterfaceDirectX::ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry)
{
	RocketD3D9CompiledGeometry* d3d9_geometry = (RocketD3D9CompiledGeometry*) geometry;

	d3d9_geometry->vertices->Release();
	d3d9_geometry->indices->Release();

	delete d3d9_geometry;
}

// Called by Rocket when it wants to enable or disable scissoring to clip content.
void RenderInterfaceDirectX::EnableScissorRegion(bool enable)
{
	if(g_pd3dDevice == NULL)
	{
		return;
	}

	g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, enable);
}

// Called by Rocket when it wants to change the scissor region.
void RenderInterfaceDirectX::SetScissorRegion(int x, int y, int width, int height)
{
	if(g_pd3dDevice == NULL)
	{
		return;
	}

	RECT scissor_rect;
	scissor_rect.left = x;
	scissor_rect.right = x + width;
	scissor_rect.top = y;
	scissor_rect.bottom = y + height;

	g_pd3dDevice->SetScissorRect(&scissor_rect);
}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1) 
struct TGAHeader 
{
	char  idLength;
	char  colourMapType;
	char  dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char  colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char  bitsPerPixel;
	char  imageDescriptor;
};
// Restore packing
#pragma pack()

// Called by Rocket when a texture is required by the library.
bool RenderInterfaceDirectX::LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source)
{
	if(g_pd3dDevice == NULL)
	{
		return false;
	}

	Rocket::Core::FileInterface* file_interface = Rocket::Core::GetFileInterface();
	Rocket::Core::FileHandle file_handle = file_interface->Open(source);
	if (file_handle == NULL)
		return false;

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);
	
	char* buffer = new char[buffer_size];
	file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer, sizeof(TGAHeader));
	
	int color_mode = header.bitsPerPixel / 8;
	int image_size = header.width * header.height * 4; // We always make 32bit textures 
	
	if (header.dataType != 2)
	{
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return false;
	}
	
	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Only 24 and 32bit textures are supported");
		return false;
	}
	
	const char* image_src = buffer + sizeof(TGAHeader);
	unsigned char* image_dest = new unsigned char[image_size];
	
	// Targa is BGR, swap to RGB and flip Y axis
	for (long y = 0; y < header.height; y++)
	{
		long read_index = y * header.width * color_mode;
		long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : (header.height - y - 1) * header.width * color_mode;
		for (long x = 0; x < header.width; x++)
		{
			image_dest[write_index] = image_src[read_index+2];
			image_dest[write_index+1] = image_src[read_index+1];
			image_dest[write_index+2] = image_src[read_index];
			if (color_mode == 4)
				image_dest[write_index+3] = image_src[read_index+3];
			else
				image_dest[write_index+3] = 255;
			
			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;
	
	bool success = GenerateTexture(texture_handle, image_dest, texture_dimensions);
	
	delete [] image_dest;
	delete [] buffer;
	
	return success;
}

// Called by Rocket when a texture is required to be built from an internally-generated sequence of pixels.
bool RenderInterfaceDirectX::GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const byte* source, const Rocket::Core::Vector2i& source_dimensions)
{
	if(g_pd3dDevice == NULL)
	{
		return false;
	}

	// Create a Direct3DTexture9, which will be set as the texture handle. Note that we only create one surface for
	// this texture; because we're rendering in a 2D context, mip-maps are not required.
	LPDIRECT3DTEXTURE9 d3d9_texture;
	if (g_pd3dDevice->CreateTexture(source_dimensions.x, source_dimensions.y, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &d3d9_texture, NULL) != D3D_OK)
		return false;

	// Lock the top surface and write the pixel data onto it.
	D3DLOCKED_RECT locked_rect;
	d3d9_texture->LockRect(0, &locked_rect, NULL, 0);
	for (int y = 0; y < source_dimensions.y; ++y)
	{
		for (int x = 0; x < source_dimensions.x; ++x)
		{
			const byte* source_pixel = source + (source_dimensions.x * 4 * y) + (x * 4);
			byte* destination_pixel = ((byte*) locked_rect.pBits) + locked_rect.Pitch * y + x * 4;

			destination_pixel[0] = source_pixel[2];
			destination_pixel[1] = source_pixel[1];
			destination_pixel[2] = source_pixel[0];
			destination_pixel[3] = source_pixel[3];
		}
	}
	d3d9_texture->UnlockRect(0);

	// Set the handle on the Rocket texture structure.
	texture_handle = (Rocket::Core::TextureHandle)d3d9_texture;
	return true;
}

// Called by Rocket when a loaded texture is no longer required.
void RenderInterfaceDirectX::ReleaseTexture(Rocket::Core::TextureHandle texture_handle)
{
	((LPDIRECT3DTEXTURE9) texture_handle)->Release();
}

// Returns the native horizontal texel offset for the renderer.
float RenderInterfaceDirectX::GetHorizontalTexelOffset()
{
	return -0.5f;
}

// Returns the native vertical texel offset for the renderer.
float RenderInterfaceDirectX::GetVerticalTexelOffset()
{
	return -0.5f;
}

