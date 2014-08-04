/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2014, David Wimsey
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

#ifndef ROCKETSHELLRENDERINTERFACE_H
#define ROCKETSHELLRENDERINTERFACE_H

/**
	Extensions to the RenderInterface class used by the Samples Shell to
 	handle various bits of rendering and rendering upkeep that would normally
 	be handled by the application rather than the libRocket RenderInterface class.
	@author David Wimsey
 */

class ShellRenderInterfaceExtensions
{
public:
    /**
     * @param[in] width width of viewport
     * @param[in] height height of viewport
	 */
    virtual void SetViewport(int width, int height) = 0;
	
    /**
	 * @param[in] context Rocket::Core::Context to set dimensions on when SetViewport is called
     */
    virtual void SetContext(void *context) = 0;
	
	/// Attach the internal window buffer to a native window
	/// @param[in] nativeWindow A handle to the OS specific native window handle
	virtual bool AttachToNative(void *nativeWindow) = 0;
	
	/// Detach and cleanup the internal window buffer from a native window
	virtual void DetachFromNative(void) = 0;
	
	/// Prepares the render buffer for drawing, in OpenGL, this would call glClear();
	virtual void PrepareRenderBuffer(void) = 0;

	/// Presents the rendered framebuffer to the screen, in OpenGL this would cal glSwapBuffers();
	virtual void PresentRenderBuffer(void) = 0;
};

#endif
