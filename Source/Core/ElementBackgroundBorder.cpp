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

#include "ElementBackgroundBorder.h"
#include "../../Include/RmlUi/Core/Box.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/Element.h"
#include "../../Include/RmlUi/Core/GeometryUtilities.h"

namespace Rml {


ElementBackgroundBorder::ElementBackgroundBorder(Element* element) : geometry(element)
{}

void ElementBackgroundBorder::Render(Element * element)
{
	if (background_dirty || border_dirty)
	{
		GenerateGeometry(element);

		background_dirty = false;
		border_dirty = false;
	}

	if (geometry)
		geometry.Render(element->GetAbsoluteOffset(Box::BORDER));
}

void ElementBackgroundBorder::DirtyBackground()
{
	background_dirty = true;
}

void ElementBackgroundBorder::DirtyBorder()
{
	border_dirty = true;
}

void ElementBackgroundBorder::GenerateGeometry(Element* element)
{
	const ComputedValues& computed = element->GetComputedValues();

	Colourb background_color = computed.background_color;
	Colourb border_colors[4] = {
		computed.border_top_color,
		computed.border_right_color,
		computed.border_bottom_color,
		computed.border_left_color,
	};
	
	// Apply opacity
	const float opacity = computed.opacity;
	background_color.alpha = (byte)(opacity * (float)background_color.alpha);

	if (opacity < 1)
	{
		for (int i = 0; i < 4; ++i)
			border_colors[i].alpha = (byte)(opacity * (float)border_colors[i].alpha);
	}

	geometry.GetVertices().clear();
	geometry.GetIndices().clear();

	const Vector4f radii(
		computed.border_top_left_radius,
		computed.border_top_right_radius,
		computed.border_bottom_right_radius,
		computed.border_bottom_left_radius
	);

	for (int i = 0; i < element->GetNumBoxes(); i++)
	{
		Vector2f offset;
		const Box& box = element->GetBox(i, offset);
		GeometryUtilities::GenerateBackgroundBorder(&geometry, box, offset, radii, background_color, border_colors);
	}

	geometry.Release();
}

} // namespace Rml
