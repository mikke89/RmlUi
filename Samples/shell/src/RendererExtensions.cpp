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
#include <RmlUi/Core/RenderInterface.h>

RendererExtensions::Image RendererExtensions::CaptureScreen(Rml::RenderInterface* p_render_interface)
{
	if (p_render_interface)
	{
		Image img;

		Rml::byte* p_image_data;
		size_t image_data_size;

		bool status = p_render_interface->CaptureScreen(img.width, img.height, img.num_components, img.row_pitch, p_image_data, image_data_size);
		RMLUI_ASSERT(status && "failed to make a screenshot (some variants why: driver failure, early calling, OS failure)");

		if (!status)
			return Image();

		img.data = Rml::UniquePtr<Rml::byte[]>(p_image_data);

		return img;
	}

	return Image();
}
