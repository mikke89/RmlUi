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
