/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
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

#include "../include/RendererExtensions.h"
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>

#if defined RMLUI_RENDERER_GL2

	#if defined RMLUI_PLATFORM_WIN32
		#include <RmlUi_Include_Windows.h>
		#include <gl/Gl.h>
		#include <gl/Glu.h>
	#elif defined RMLUI_PLATFORM_MACOSX
		#include <AGL/agl.h>
		#include <OpenGL/gl.h>
		#include <OpenGL/glext.h>
		#include <OpenGL/glu.h>
	#elif defined RMLUI_PLATFORM_UNIX
		#include <RmlUi_Include_Xlib.h>
		#include <GL/gl.h>
		#include <GL/glext.h>
		#include <GL/glu.h>
		#include <GL/glx.h>
	#endif

#elif defined RMLUI_RENDERER_GL3 && !defined RMLUI_PLATFORM_EMSCRIPTEN

	#include <RmlUi_Include_GL3.h>

#elif defined RMLUI_RENDERER_GL3 && defined RMLUI_PLATFORM_EMSCRIPTEN

	#include <GLES3/gl3.h>

#endif

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
#if defined RMLUI_RENDERER_GL2 || defined RMLUI_RENDERER_GL3

	int viewport[4] = {}; // x, y, width, height
	glGetIntegerv(GL_VIEWPORT, viewport);

	Image image;
	image.num_components = 3;
	image.width = viewport[2];
	image.height = viewport[3];

	if (image.width < 1 || image.height < 1)
		return Image();

	const int byte_size = image.width * image.height * image.num_components;
	image.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[byte_size]);

	glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.data.get());

	bool result = true;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		result = false;
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screenshot, got GL error: 0x%x", err);
	}

	if (!result)
		return Image();

	return image;

#else

	return Image();

#endif
}
