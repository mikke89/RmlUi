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
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <Rocket/Core.h>
#include <Rocket/Core/Platform.h>
#include "../../include/Shell.h"

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
	this->nwData.display = ((__X11NativeWindowData *)nativeWindow)->display;
	this->nwData.window = ((__X11NativeWindowData *)nativeWindow)->window;
	this->nwData.visual_info = ((__X11NativeWindowData *)nativeWindow)->visual_info;

	this->gl_context = glXCreateContext(nwData.display, nwData.visual_info, NULL, GL_TRUE);
	if (this->gl_context == NULL)
		return false;
	
	if (!glXMakeCurrent(nwData.display, nwData.window, this->gl_context))
		return false;
	
	if (!glXIsDirect(nwData.display, this->gl_context))
		Shell::Log("OpenGL context does not support direct rendering; performance is likely to be poor.");

	Window root_window;
	int x, y;
	unsigned int width, height;
	unsigned int border_width, depth;
	XGetGeometry(nwData.display, nwData.window, &root_window, &x, &y, &width, &height, &border_width, &depth);
	
	// Set up the GL state.
	glClearColor(0, 0, 0, 1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1024, 768, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	return true;
}

void ShellRenderInterfaceOpenGL::DetachFromNative()
{
	// Shutdown OpenGL	
	glXMakeCurrent(this->nwData.display, None, NULL);
	glXDestroyContext(this->nwData.display, this->gl_context);
	this->gl_context = NULL;
}

void ShellRenderInterfaceOpenGL::PrepareRenderBuffer()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void ShellRenderInterfaceOpenGL::PresentRenderBuffer()
{

	// Flips the OpenGL buffers.
	glXSwapBuffers(this->nwData.display, this->nwData.window);
}
