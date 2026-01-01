#pragma once

#include "Header.h"
#include "TransformPrimitive.h"
#include "Types.h"

namespace Rml {

class Property;

/**
    The Transform class holds the information parsed from an element's `transform' property.

    The class holds a list of transform primitives making up a complete transformation specification
    of an element. Each transform instance is relative to the element's parent coordinate system.
    During the Context::Render call, the transforms of the current element and its ancestors will be
    used to find the final transformation matrix for the global coordinate system.
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
