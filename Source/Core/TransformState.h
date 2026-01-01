#pragma once

#include "../../Include/RmlUi/Core/Header.h"
#include "../../Include/RmlUi/Core/Types.h"

namespace Rml {

class TransformState {
public:
	// Returns true if transform was changed.
	bool SetTransform(const Matrix4f* in_transform);

	// Returns true if local perspecitve was changed.
	bool SetLocalPerspective(const Matrix4f* in_perspective);

	const Matrix4f* GetTransform() const;
	const Matrix4f* GetLocalPerspective() const;

	// Returns a nullptr if there is no transform set, or the transform is singular.
	const Matrix4f* GetInverseTransform() const;

private:
	bool have_transform = false;
	bool have_perspective = false;
	mutable bool have_inverse_transform = false;
	mutable bool dirty_inverse_transform = false;

	// The accumulated transform matrix combines all transform and perspective properties of the owning element and all ancestors.
	Matrix4f transform;

	// Local perspective which applies to children of the owning element.
	Matrix4f local_perspective;

	// The inverse of the transform matrix for projecting points from screen space to the current element's space, such as used for picking elements.
	mutable Matrix4f inverse_transform;
};

} // namespace Rml
