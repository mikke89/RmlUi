#pragma once

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
