#include "RmlUi_Renderer_BackwardCompatible_GL2.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <string.h>

#if defined RMLUI_PLATFORM_WIN32
	#include "RmlUi_Include_Windows.h"
	#include <gl/Gl.h>
	#include <gl/Glu.h>
#elif defined RMLUI_PLATFORM_MACOSX
	#include <AGL/agl.h>
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>
	#include <OpenGL/glu.h>
#elif defined RMLUI_PLATFORM_UNIX
	#include "RmlUi_Include_Xlib.h"
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include <GL/glu.h>
	#include <GL/glx.h>
#endif

#define GL_CLAMP_TO_EDGE 0x812F

RenderInterface_BackwardCompatible_GL2::RenderInterface_BackwardCompatible_GL2() {}

void RenderInterface_BackwardCompatible_GL2::SetViewport(int in_viewport_width, int in_viewport_height)
{
	viewport_width = in_viewport_width;
	viewport_height = in_viewport_height;
}

void RenderInterface_BackwardCompatible_GL2::BeginFrame()
{
	RMLUI_ASSERT(viewport_width >= 0 && viewport_height >= 0);
	glViewport(0, 0, viewport_width, viewport_height);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Rml::Matrix4f projection = Rml::Matrix4f::ProjectOrtho(0, (float)viewport_width, (float)viewport_height, 0, -10000, 10000);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection.data());
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	transform_enabled = false;
}

void RenderInterface_BackwardCompatible_GL2::EndFrame() {}

void RenderInterface_BackwardCompatible_GL2::Clear()
{
	glClearStencil(0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RenderInterface_BackwardCompatible_GL2::RenderGeometry(Rml::Vertex* vertices, int /*num_vertices*/, int* indices, int num_indices,
	const Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
	glPushMatrix();
	glTranslatef(translation.x, translation.y, 0);

	glVertexPointer(2, GL_FLOAT, sizeof(Rml::Vertex), &vertices[0].position);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Rml::Vertex), &vertices[0].colour);

	if (!texture)
	{
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		glEnable(GL_TEXTURE_2D);

		if (texture != TextureEnableWithoutBinding)
			glBindTexture(GL_TEXTURE_2D, (GLuint)texture);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Rml::Vertex), &vertices[0].tex_coord);
	}

	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, indices);

	glPopMatrix();
}

void RenderInterface_BackwardCompatible_GL2::EnableScissorRegion(bool enable)
{
	if (enable)
	{
		if (!transform_enabled)
		{
			glEnable(GL_SCISSOR_TEST);
			glDisable(GL_STENCIL_TEST);
		}
		else
		{
			glDisable(GL_SCISSOR_TEST);
			glEnable(GL_STENCIL_TEST);
		}
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_STENCIL_TEST);
	}
}

void RenderInterface_BackwardCompatible_GL2::SetScissorRegion(int x, int y, int width, int height)
{
	if (!transform_enabled)
	{
		glScissor(x, viewport_height - (y + height), width, height);
	}
	else
	{
		// clear the stencil buffer
		glStencilMask(GLuint(-1));
		glClear(GL_STENCIL_BUFFER_BIT);

		// fill the stencil buffer
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glStencilFunc(GL_NEVER, 1, GLuint(-1));
		glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

		float fx = (float)x;
		float fy = (float)y;
		float fwidth = (float)width;
		float fheight = (float)height;

		// draw transformed quad
		GLfloat vertices[] = {fx, fy, 0, fx, fy + fheight, 0, fx + fwidth, fy + fheight, 0, fx + fwidth, fy, 0};
		glDisableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		GLushort indices[] = {1, 2, 0, 3};
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, indices);
		glEnableClientState(GL_COLOR_ARRAY);

		// prepare for drawing the real thing
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		glStencilMask(0);
		glStencilFunc(GL_EQUAL, 1, GLuint(-1));
	}
}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1)
struct TGAHeader {
	char idLength;
	char colourMapType;
	char dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char bitsPerPixel;
	char imageDescriptor;
};
// Restore packing
#pragma pack()

bool RenderInterface_BackwardCompatible_GL2::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions,
	const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
	{
		return false;
	}

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	if (buffer_size <= sizeof(TGAHeader))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Texture file size is smaller than TGAHeader, file is not a valid TGA image.");
		file_interface->Close(file_handle);
		return false;
	}

	char* buffer = new char[buffer_size];
	file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer, sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	int image_size = header.width * header.height * 4; // We always make 32bit textures

	if (header.dataType != 2)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		delete[] buffer;
		return false;
	}

	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported.");
		delete[] buffer;
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
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4)
				image_dest[write_index + 3] = image_src[read_index + 3];
			else
				image_dest[write_index + 3] = 255;

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;

	bool success = GenerateTexture(texture_handle, image_dest, texture_dimensions);

	delete[] image_dest;
	delete[] buffer;

	return success;
}

bool RenderInterface_BackwardCompatible_GL2::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source,
	const Rml::Vector2i& source_dimensions)
{
	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	if (texture_id == 0)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to generate texture.");
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	texture_handle = (Rml::TextureHandle)texture_id;

	return true;
}

void RenderInterface_BackwardCompatible_GL2::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	glDeleteTextures(1, (GLuint*)&texture_handle);
}

void RenderInterface_BackwardCompatible_GL2::SetTransform(const Rml::Matrix4f* transform)
{
	transform_enabled = (transform != nullptr);

	if (transform)
	{
		if (std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value)
			glLoadMatrixf(transform->data());
		else if (std::is_same<Rml::Matrix4f, Rml::RowMajorMatrix4f>::value)
			glLoadMatrixf(transform->Transpose().data());
	}
	else
		glLoadIdentity();
}
