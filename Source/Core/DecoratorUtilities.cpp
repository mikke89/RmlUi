/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2024 The RmlUi Team, and contributors
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

#include "DecoratorUtilities.h"
#include "../../Include/RmlUi/Core/Property.h"

namespace Rml {

Vector2Numeric ComputePosition(Array<const Property*, 2> p_position)
{
	Vector2Numeric position;
	for (int dimension = 0; dimension < 2; dimension++)
	{
		NumericValue& value = position[dimension];
		const Property& property = *p_position[dimension];
		if (property.unit == Unit::KEYWORD)
		{
			enum { TOP_LEFT, CENTER, BOTTOM_RIGHT };
			switch (property.Get<int>())
			{
			case TOP_LEFT: value = NumericValue(0.f, Unit::PERCENT); break;
			case CENTER: value = NumericValue(50.f, Unit::PERCENT); break;
			case BOTTOM_RIGHT: value = NumericValue(100.f, Unit::PERCENT); break;
			}
		}
		else
		{
			value = property.GetNumericValue();
		}
	}
	return position;
}

} // namespace Rml
