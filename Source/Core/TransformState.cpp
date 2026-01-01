#include "TransformState.h"

namespace Rml {

bool TransformState::SetTransform(const Matrix4f* in_transform)
{
	bool is_changed = (have_transform != (bool)in_transform);
	if (in_transform)
	{
		is_changed |= (have_transform && transform != *in_transform);
		transform = *in_transform;
		have_transform = true;
	}
	else
		have_transform = false;

	if (is_changed)
		dirty_inverse_transform = true;

	return is_changed;
}
bool TransformState::SetLocalPerspective(const Matrix4f* in_perspective)
{
	bool is_changed = (have_perspective != (bool)in_perspective);

	if (in_perspective)
	{
		is_changed |= (have_perspective && local_perspective != *in_perspective);
		local_perspective = *in_perspective;
		have_perspective = true;
	}
	else
		have_perspective = false;

	return is_changed;
}

const Matrix4f* TransformState::GetTransform() const
{
	return have_transform ? &transform : nullptr;
}

const Matrix4f* TransformState::GetLocalPerspective() const
{
	return have_perspective ? &local_perspective : nullptr;
}

const Matrix4f* TransformState::GetInverseTransform() const
{
	if (!have_transform)
		return nullptr;

	if (dirty_inverse_transform)
	{
		inverse_transform = transform;
		have_inverse_transform = inverse_transform.Invert();
		dirty_inverse_transform = false;
	}

	if (have_inverse_transform)
		return &inverse_transform;

	return nullptr;
}

} // namespace Rml
