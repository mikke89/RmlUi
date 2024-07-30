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

#include "../../Include/RmlUi/Core/ConvolutionFilter.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include <float.h>
#include <string.h>

namespace Rml {

ConvolutionFilter::ConvolutionFilter() {}

ConvolutionFilter::~ConvolutionFilter() {}

bool ConvolutionFilter::Initialise(int _kernel_radius, FilterOperation _operation)
{
	return Initialise(Vector2i(_kernel_radius), _operation);
}

bool ConvolutionFilter::Initialise(Vector2i _kernel_radii, FilterOperation _operation)
{
	if (_kernel_radii.x < 0 || _kernel_radii.y < 0)
	{
		RMLUI_ERRORMSG("Invalid input parameters to convolution filter.");
		return false;
	}

	kernel_size = _kernel_radii * 2 + Vector2i(1);

	kernel = UniquePtr<float[]>(new float[kernel_size.x * kernel_size.y]());

	operation = _operation;
	return true;
}

float* ConvolutionFilter::operator[](int kernel_y_index)
{
	RMLUI_ASSERT(kernel != nullptr && kernel_y_index >= 0 && kernel_y_index < kernel_size.y);

	kernel_y_index = Math::Clamp(kernel_y_index, 0, kernel_size.y - 1);

	return kernel.get() + kernel_size.x * kernel_y_index;
}

void ConvolutionFilter::Run(byte* destination, const Vector2i destination_dimensions, const int destination_stride,
	const ColorFormat destination_color_format, const byte* source, const Vector2i source_dimensions, const Vector2i source_offset,
	const ColorFormat source_color_format) const
{
	RMLUI_ZoneScopedNC("ConvFilter::Run", 0xd6bf49);

	const int destination_bytes_per_pixel = (destination_color_format == ColorFormat::RGBA8 ? 4 : 1);
	const int destination_alpha_offset = (destination_color_format == ColorFormat::RGBA8 ? 3 : 0);
	const int source_bytes_per_pixel = (source_color_format == ColorFormat::RGBA8 ? 4 : 1);
	const int source_alpha_offset = (source_color_format == ColorFormat::RGBA8 ? 3 : 0);

	const Vector2i kernel_radius = (kernel_size - Vector2i(1)) / 2;

	for (int y = 0; y < destination_dimensions.y; ++y)
	{
		for (int x = 0; x < destination_dimensions.x; ++x)
		{
			float opacity = 0.f;

			for (int kernel_y = 0; kernel_y < kernel_size.y; ++kernel_y)
			{
				const int source_y = y - source_offset.y - kernel_radius.y + kernel_y;

				for (int kernel_x = 0; kernel_x < kernel_size.x; ++kernel_x)
				{
					const int source_x = x - source_offset.x - kernel_radius.x + kernel_x;
					if (source_y >= 0 && source_y < source_dimensions.y && source_x >= 0 && source_x < source_dimensions.x)
					{
						const int source_index = (source_y * source_dimensions.x + source_x) * source_bytes_per_pixel + source_alpha_offset;
						const float pixel_opacity = float(source[source_index]) * kernel[kernel_y * kernel_size.x + kernel_x];

						switch (operation)
						{
						case FilterOperation::Sum: opacity += pixel_opacity; break;
						case FilterOperation::Dilation: opacity = Math::Max(opacity, pixel_opacity); break;
						}
					}
				}
			}

			opacity = Math::Min(255.f, opacity);

			const int destination_index = y * destination_stride + x * destination_bytes_per_pixel + destination_alpha_offset;
			destination[destination_index] = byte(opacity);
		}
	}
}

} // namespace Rml
