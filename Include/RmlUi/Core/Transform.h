/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2014 Markus Schöngart
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

#ifndef RMLUI_CORE_TRANSFORM_H
#define RMLUI_CORE_TRANSFORM_H

#include "Header.h"
#include "TransformPrimitive.h"
#include "Types.h"

namespace Rml {

class Property;

/**
    The Transform class holds the information parsed from an element's `transform' property.

    The class holds a list of transform primitives making up a complete transformation specification
    of an element. Each transform instance is relative to the element's parent coordinate system.
    During the Context::Render call the transforms of the current element and its ancestors will be
    used to find the final transformation matrix for the global coordinate system.

    @author Markus Schöngart
    @see Rml::Variant
 */

class RMLUICORE_API Transform {
public:
	using PrimitiveList = Vector<TransformPrimitive>;

	/// Default constructor, initializes an identity transform
	Transform();

	/// Construct transform with a list of primitives
	Transform(PrimitiveList primitives);

	/// Helper function to create a 'transform' Property from the given list of primitives
	static Property MakeProperty(PrimitiveList primitives);

	/// Remove all Primitives from this Transform
	void ClearPrimitives();

	/// Add a Primitive to this Transform
	void AddPrimitive(const TransformPrimitive& p);

	/// Return the number of Primitives in this Transform
	int GetNumPrimitives() const noexcept;

	/// Return the i-th Primitive in this Transform
	const TransformPrimitive& GetPrimitive(int i) const noexcept;

	PrimitiveList& GetPrimitives() noexcept { return primitives; }
	const PrimitiveList& GetPrimitives() const noexcept { return primitives; }

private:
	PrimitiveList primitives;
};

} // namespace Rml
#endif
