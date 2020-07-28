/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "Screenshot.h"
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>
#include <RmlUi/Core/Log.h>

#define LODEPNG_NO_COMPILE_CPP

#include <lodepng.h>


bool CaptureScreenshot(ShellRenderInterfaceOpenGL* shell_renderer, const Rml::String& filename, int clip_width)
{
	using Image = ShellRenderInterfaceOpenGL::Image;

	Image image_orig = shell_renderer->CaptureScreen();

	if (!image_orig.data)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screenshot from OpenGL window.");
		return false;
	}

	if (clip_width == 0)
		clip_width = image_orig.width;

	// Create a new image flipped vertically, and clipped to the given clip width.
	Image image;
	image.width = clip_width;
	image.height = image_orig.height;
	image.num_components = image_orig.num_components;
	image.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[image.width * image.height * image.num_components]);

	const int c = image.num_components;
	for (int y = 0; y < image.height; y++)
	{
		const int flipped_y = image_orig.height - y - 1;

		const int yb = y * image.width * c;
		const int yb_orig = flipped_y * image_orig.width * c;
		const int wb = image.width * c;

		for (int xb = 0; xb < wb; xb++)
		{
			image.data[yb + xb] = image_orig.data[yb_orig + xb];
		}
	}

	const Rml::String output_directory = GetOutputDirectory() + "/" + filename;

	if (lodepng_encode24_file(output_directory.c_str(), image.data.get(), image.width, image.height))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not write the captured screenshot to %s", output_directory.c_str());
		return false;
	}

	return true;
}


Rml::String GetOutputDirectory()
{
#ifdef RMLUI_VISUAL_TESTS_OUTPUT_DIRECTORY
	const Rml::String output_directory = Rml::String(RMLUI_VISUAL_TESTS_OUTPUT_DIRECTORY);
#else
	const Rml::String output_directory = Shell::FindSamplesRoot() + "../Tests/Output";
#endif
	return output_directory;
}



// Suppress warnings emitted by lodepng
#if defined(RMLUI_PLATFORM_WIN32) && !defined(__MINGW32__)
#pragma warning(disable : 4334)
#pragma warning(disable : 4267)
#endif

#include <lodepng.cpp>
