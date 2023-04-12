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

#ifndef RMLUI_CORE_TRANSFORMUTILITIES_H
#define RMLUI_CORE_TRANSFORMUTILITIES_H

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

struct TransformPrimitive;
namespace Transforms {
	struct DecomposedMatrix4;
}

namespace TransformUtilities {
	// Set the primitive to its identity value.
	void SetIdentity(TransformPrimitive& primitive) noexcept;

	// Resolve the primitive into a transformation matrix, given the current element properties and layout.
	Matrix4f ResolveTransform(const TransformPrimitive& primitive, Element& e) noexcept;

	// Prepares the primitive for interpolation. This must be done before calling InterpolateWith().
	// Promote units to basic types which can be interpolated, that is, convert 'length -> pixel' for unresolved primitives.
	// Returns false if the owning transform must to be converted to a DecomposedMatrix4 primitive.
	bool PrepareForInterpolation(TransformPrimitive& primitive, Element& e) noexcept;

	// If primitives do not match, try to convert them to a common generic type, e.g. TranslateX -> Translate3D.
	// Returns true if they are already the same type or were converted to a common generic type.
	bool TryConvertToMatchingGenericType(TransformPrimitive& p0, TransformPrimitive& p1) noexcept;

	// Interpolate the target primitive with another primitive, weighted by alpha [0, 1].
	// Primitives must be of the same type, and PrepareForInterpolation() must previously have been called on both.
	bool InterpolateWith(TransformPrimitive& target, const TransformPrimitive& other, float alpha) noexcept;

	// Decompose a Matrix4 into its decomposed components.
	// Returns true on success, or false if the matrix is singular.
	bool Decompose(Transforms::DecomposedMatrix4& decomposed_matrix, const Matrix4f& matrix) noexcept;

	String ToString(const TransformPrimitive& primitive) noexcept;
} // namespace TransformUtilities

} // namespace Rml
#endif
