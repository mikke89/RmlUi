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

#include <ShellRenderInterfaceExtensions.h>
#include <ShellRenderInterfaceOpenGL.h>
#include <Carbon/Carbon.h>
#include <Rocket/Core/Context.h>
#include <Rocket/Core.h>
#include <Rocket/Core/Platform.h>

void ShellRenderInterfaceOpenGL::SetContext(void *context)
{
	m_rocket_context = context;
}

void ShellRenderInterfaceOpenGL::SetViewport(int width, int height)
{
	if(m_width != width || m_height != height) {
		Rocket::Core::Matrix4f projection, view;

		m_width = width;
		m_height = height;

		glViewport(0, 0, width, height);
		projection = Rocket::Core::Matrix4f::ProjectOrtho(0, width, height, 0, -1, 1);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(projection);
		view = Rocket::Core::Matrix4f::Identity();
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(view);

		aglUpdateContext(gl_context);

		if(m_rocket_context != NULL)
		{
			((Rocket::Core::Context*)m_rocket_context)->SetDimensions(Rocket::Core::Vector2i(width, height));
			((Rocket::Core::Context*)m_rocket_context)->ProcessProjectionChange(projection);
			((Rocket::Core::Context*)m_rocket_context)->ProcessViewChange(view);
		}
	}
}

bool ShellRenderInterfaceOpenGL::AttachToNative(void *nativeWindow)
{
	WindowRef window = (WindowRef)nativeWindow;
	static GLint attributes[] =
	{
		AGL_RGBA,
		AGL_DOUBLEBUFFER,
		AGL_ALPHA_SIZE, 8,
		AGL_DEPTH_SIZE, 24,
		AGL_STENCIL_SIZE, 8,
		AGL_ACCELERATED,
		AGL_NONE
	};
	
	AGLPixelFormat pixel_format = aglChoosePixelFormat(NULL, 0, attributes);
	if (pixel_format == NULL)
		return false;
	
	CGrafPtr window_port = GetWindowPort(window);
	if (window_port == NULL)
		return false;
	
	this->gl_context = aglCreateContext(pixel_format, NULL);
	if (this->gl_context == NULL)
		return false;
	
	aglSetDrawable(this->gl_context, window_port);
	aglSetCurrentContext(this->gl_context);
	
	aglDestroyPixelFormat(pixel_format);
	
	// Set up the GL state.
	glClearColor(0, 0, 0, 1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect crect;
	GetWindowBounds(window, kWindowContentRgn, &crect);
	glOrtho(0, (crect.right - crect.left), (crect.bottom - crect.top), 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void ShellRenderInterfaceOpenGL::DetachFromNative()
{
	// Shutdown OpenGL if necessary.
	aglSetCurrentContext(NULL);
	aglSetDrawable(this->gl_context, NULL);
	aglDestroyContext(this->gl_context);
}

void ShellRenderInterfaceOpenGL::PrepareRenderBuffer()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void ShellRenderInterfaceOpenGL::PresentRenderBuffer()
{
	// Flips the OpenGL buffers.
	aglSwapBuffers(this->gl_context);
}
