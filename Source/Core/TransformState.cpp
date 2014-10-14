/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2014 Markus Schöngart
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

#include "precompiled.h"
#include "../../Include/Rocket/Core/TransformState.h"
#include <cmath>

namespace Rocket {
namespace Core {

Matrix4f TransformState::Perspective::GetProjection() const throw()
{
	float depth = Math::Max(view_size.x, view_size.y);

	if (distance == 0)
	{
		return Matrix4f::ProjectOrtho(
			-0.5 * view_size.x,
			+0.5 * view_size.x,
			+0.5 * view_size.y,
			-0.5 * view_size.y,
			-1.0 * depth,
			+1.0 * depth
		) * Matrix4f::Translate(
			-0.5f * view_size.x,
			-0.5f * view_size.y,
			0
		);
	}
	else if (distance > 0)
	{
		float far = distance + 1.0 * depth;
		const float FAR_NEAR_RATIO = 256.0f;
		float near = Math::Max(1.0f, far / FAR_NEAR_RATIO);
		float scale = near / distance;
		return Matrix4f::ProjectPerspective(
			(-vanish.x) * scale,
			(view_size.x - vanish.x) * scale,
			(view_size.y - vanish.y) * scale,
			(-vanish.y) * scale,
			near,
			far
		) * Matrix4f::Translate(
			-vanish.x,
			-vanish.y,
			-distance
		);
	}
	else /* if (distance < 0) */
	{
		return Matrix4f::Identity();
	}
}

Vector3f TransformState::Perspective::Project(const Vector3f &point) const throw()
{
	if (distance < 0)
	{
		return point;
	}
	else /* if (distance >= 0) */
	{
		return GetProjection() * point;
	}
}

Vector3f TransformState::Perspective::Unproject(const Vector3f &point) const throw()
{
	if (distance < 0)
	{
		return point;
	}
	else /* if (distance >= 0) */
	{
		Matrix4f projection_inv = GetProjection();
		projection_inv.Invert();
		return projection_inv * point;
	}
}

Matrix4f TransformState::LocalPerspective::GetProjection() const throw()
{
	float depth = Math::Max(view_size.x, view_size.y);

	if (distance == 0)
	{
		return Matrix4f::ProjectOrtho(
			-0.5 * view_size.x,
			+0.5 * view_size.x,
			+0.5 * view_size.y,
			-0.5 * view_size.y,
			-0.5 * depth,
			+0.5 * depth
		) * Matrix4f::Translate(
			-0.5f * view_size.x,
			-0.5f * view_size.y,
			0
		);
	}
	else if (distance > 0)
	{
		return Matrix4f::ProjectPerspective(
			(0 - 0.5f) * view_size.x,
			(1 - 0.5f) * view_size.x,
			(1 - 0.5f) * view_size.y,
			(0 - 0.5f) * view_size.y,
			distance,
			distance + depth
		) * Matrix4f::Translate(
			-0.5f * view_size.x,
			-0.5f * view_size.y,
			-distance - 0.5f * depth
		);
	}
	else /* if (distance < 0) */
	{
		return Matrix4f::Identity();
	}
}

Vector3f TransformState::LocalPerspective::Project(const Vector3f &point) const throw()
{
	if (distance < 0)
	{
		return point;
	}
	else /* if (distance >= 0) */
	{
		return GetProjection() * point;
	}
}

Vector3f TransformState::LocalPerspective::Unproject(const Vector3f &point) const throw()
{
	if (distance < 0)
	{
		return point;
	}
	else /* if (distance >= 0) */
	{
		Matrix4f projection_inv = GetProjection();
		projection_inv.Invert();
		return projection_inv * point;
	}
}

TransformState::TransformState()
	: have_perspective(false), have_local_perspective(false),
	  have_parent_recursive_transform(false), have_transform(false)
{
}

void TransformState::SetPerspective(const Perspective *perspective) throw()
{
	if (perspective)
	{
		this->perspective = perspective->distance;
		this->view_size = perspective->view_size;
		this->vanish = perspective->vanish;
	}

	have_perspective = perspective != 0;
}

bool TransformState::GetPerspective(Perspective *perspective) const throw()
{
	if (have_perspective && perspective)
	{
		perspective->distance = this->perspective;
		perspective->view_size = this->view_size;
		perspective->vanish = this->vanish;
	}

	return have_perspective;
}

void TransformState::SetLocalPerspective(const LocalPerspective *local_perspective) throw()
{
	if (local_perspective)
	{
		this->local_perspective = local_perspective->distance;
		this->view_size = local_perspective->view_size;
	}

	have_local_perspective = local_perspective != 0;
}

bool TransformState::GetLocalPerspective(LocalPerspective *local_perspective) const throw()
{
	if (have_local_perspective && local_perspective)
	{
		local_perspective->distance = this->local_perspective;
		local_perspective->view_size = this->view_size;
	}

	return have_local_perspective;
}

void TransformState::SetTransform(const Matrix4f *transform) throw()
{
	if (transform)
	{
		this->transform = *transform;
	}

	have_transform = transform != 0;
}

bool TransformState::GetTransform(Matrix4f *transform) const throw()
{
	if (have_transform)
	{
		if (transform)
		{
			*transform = this->transform;
		}

		return true;
	}
	else
	{
		return false;
	}
}

void TransformState::SetParentRecursiveTransform(const Matrix4f *parent_recursive_transform) throw()
{
	if (parent_recursive_transform)
	{
		this->parent_recursive_transform = *parent_recursive_transform;
	}

	have_parent_recursive_transform = parent_recursive_transform != 0;
}

bool TransformState::GetParentRecursiveTransform(Matrix4f *transform) const throw()
{
	if (have_parent_recursive_transform)
	{
		if (transform)
		{
			*transform = this->parent_recursive_transform;
		}

		return true;
	}
	else
	{
		return false;
	}
}

Vector3f TransformState::Transform(const Vector3f &point) const throw()
{
	if (have_parent_recursive_transform && have_transform)
	{
		return parent_recursive_transform * (transform * point);
	}
	else if (have_parent_recursive_transform)
	{
		return parent_recursive_transform * point;
	}
	else if (have_transform)
	{
		return transform * point;
	}
	else
	{
		return point;
	}
}

Vector3f TransformState::Untransform(const Vector3f &point) const throw()
{
	Matrix4f transform_inv;

	if (have_parent_recursive_transform && have_transform)
	{
		transform_inv = parent_recursive_transform * transform;
		transform_inv.Invert();
	}
	else if (have_parent_recursive_transform)
	{
		transform_inv = parent_recursive_transform;
		transform_inv.Invert();
	}
	else if (have_transform)
	{
		transform_inv = transform;
		transform_inv.Invert();
	}
	else
	{
		return point;
	}

	return transform_inv * point;
}

bool TransformState::GetRecursiveTransform(Matrix4f *recursive_transform) const throw()
{
	if (recursive_transform)
	{
		if (have_parent_recursive_transform && have_transform)
		{
			*recursive_transform = parent_recursive_transform * transform;
		}
		else if (have_parent_recursive_transform)
		{
			*recursive_transform = parent_recursive_transform;
		}
		else if (have_transform)
		{
			*recursive_transform = transform;
		}
	}

	return have_parent_recursive_transform || have_transform;
}

}
}
