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

#include "CaptureScreen.h"
#include "TestConfig.h"
#include <Shell.h>
#include <ShellRenderInterfaceOpenGL.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/StringUtilities.h>
#include <cmath>

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

	const Rml::String output_path = GetCaptureOutputDirectory() + "/" + filename;
	unsigned int lodepng_result = lodepng_encode24_file(output_path.c_str(), image.data.get(), image.width, image.height);
	if (lodepng_result)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not write the captured screenshot to %s: %s", output_path.c_str(), lodepng_error_text(lodepng_result));
		return false;
	}

	return true;
}


struct DeferFree {
	unsigned char* ptr = nullptr;
	~DeferFree() { free(ptr); }
};


ComparisonResult CompareScreenToPreviousCapture(ShellRenderInterfaceOpenGL* shell_renderer, const Rml::String& filename)
{
	using Image = ShellRenderInterfaceOpenGL::Image;

	const Rml::String input_path = GetCompareInputDirectory() + "/" + filename;

	unsigned char* data_ref = nullptr;
	unsigned int w_ref = 0, h_ref = 0;

	unsigned int lodepng_result = lodepng_decode24_file(&data_ref, &w_ref, &h_ref, input_path.c_str());
	DeferFree defer_free{ data_ref };

	if (lodepng_result)
	{
		ComparisonResult result;
		result.success = false;
		result.error_msg = Rml::CreateString(1024, "Could not read the captured screenshot from %s: %s", input_path.c_str(), lodepng_error_text(lodepng_result));
		return result;
	}
	RMLUI_ASSERT(w_ref > 0 && h_ref > 0 && data_ref);
	

	Image screen = shell_renderer->CaptureScreen();
	if (!screen.data)
	{
		ComparisonResult result;
		result.success = false;
		result.error_msg = "Could not capture screen from OpenGL window.";
		return result;
	}
	RMLUI_ASSERT(screen.num_components == 3);

	Image diff;
	diff.width = w_ref;
	diff.height = h_ref;
	diff.num_components = 3;
	diff.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[diff.width * diff.height * diff.num_components]);

	// So we have both images now, compare them! Also create a diff image.
	// In case they are not the same size, we require that the reference image size is smaller or equal to the screen
	// in both dimensions, and we compare them at the top-left corner.
	// Note that the loaded image is flipped vertically compared to the OpenGL capture!

	if (screen.width < (int)w_ref || screen.height < (int)h_ref)
	{
		ComparisonResult result;
		result.success = false;
		result.error_msg = "Test comparison failed. The screen is smaller than the reference image in one or both dimensions.";
		return result;
	}

	size_t sum_diff = 0;

	constexpr int c = 3;
	for (int y = 0; y < (int)h_ref; y++)
	{
		const int y_flipped_screen = screen.height - y - 1;
		const int yb_screen = y_flipped_screen * screen.width * c;

		const int wb_ref = w_ref * c;
		const int yb_ref = y * w_ref * c;

		for (int xb = 0; xb < wb_ref; xb++)
		{
			const int i_ref = yb_ref + xb;
			Rml::byte pix_ref = data_ref[i_ref];
			Rml::byte pix_screen = screen.data[yb_screen + xb];
			diff.data[i_ref] = (Rml::byte)std::abs((int)pix_ref - (int)pix_screen);
			sum_diff += (size_t)diff.data[i_ref];
		}
	}

	ComparisonResult result;
	result.skipped = false;
	result.success = true;
	result.is_equal = (sum_diff == 0);
	result.absolute_difference_sum = sum_diff;

	const size_t max_diff = size_t(c * 255) * size_t(w_ref) * size_t(h_ref);
	result.similarity_score = (sum_diff == 0 ? 1.0 : 1.0 - std::log(double(sum_diff)) / std::log(double(max_diff)));

	// Write the diff image to file if they are not equal.
	if (!result.is_equal)
	{
		Rml::String out_filename = filename;
		size_t offset = filename.rfind('.');
		if (offset == Rml::String::npos)
			offset = filename.size();
		out_filename.insert(offset, "-diff");

		const Rml::String output_path = GetCaptureOutputDirectory() + '/' + out_filename;
		lodepng_result = lodepng_encode24_file(output_path.c_str(), diff.data.get(), diff.width, diff.height);
		if (lodepng_result)
		{
			// We still report it as a success.
			result.error_msg = "Could not write output diff image to " + output_path + Rml::String(": ") + lodepng_error_text(lodepng_result);
		}
	}

	return result;
}


// Suppress warnings emitted by lodepng
#if defined(RMLUI_PLATFORM_WIN32) && !defined(__MINGW32__)
#pragma warning(disable : 4334)
#pragma warning(disable : 4267)
#endif

#include <lodepng.cpp>
