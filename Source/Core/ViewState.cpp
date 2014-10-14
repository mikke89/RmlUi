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
#include "../../Include/Rocket/Core/ViewState.h"
#include <cmath>

namespace Rocket {
namespace Core {

ViewState::ViewState()
	: have_projection(false), have_view(false), projection_view_inv_dirty(true),
	  projection(), view()
{
}

void ViewState::SetProjection(const Matrix4f *projection) throw()
{
	if (projection)
	{
		this->projection = *projection;
	}

	have_projection = projection != 0;
	projection_view_inv_dirty = true;
}

void ViewState::SetView(const Matrix4f *view) throw()
{
	if (view)
	{
		this->view = *view;
	}

	have_view = view != 0;
	projection_view_inv_dirty = true;
}

bool ViewState::GetProjectionViewInv(Matrix4f& projection_view_inv) const throw()
{
	if (have_projection || have_view)
	{
		if (projection_view_inv_dirty)
		{
			UpdateProjectionViewInv();
		}

		projection_view_inv = this->projection_view_inv;

		return true;
	}
	else
	{
		return false;
	}
}

Vector3f ViewState::Project(const Vector3f &point) const throw()
{
	if (have_projection && have_view)
	{
		return projection * (view * point);
	}
	else if (have_projection)
	{
		return projection * point;
	}
	else if (have_view)
	{
		return view * point;
	}
	else
	{
		return point;
	}
}

Vector3f ViewState::Unproject(const Vector3f &point) const throw()
{
	if (have_projection || have_view)
	{
		if (projection_view_inv_dirty)
		{
			UpdateProjectionViewInv();
		}

		return projection_view_inv * point;
	}
	else
	{
		return point;
	}
}

void ViewState::UpdateProjectionViewInv() const throw()
{
	ROCKET_ASSERT(projection_view_inv_dirty);

	if (have_projection && have_view)
	{
		projection_view_inv = projection * view;
		projection_view_inv.Invert();
	}
	else if (have_projection)
	{
		projection_view_inv = projection;
		projection_view_inv.Invert();
	}
	else if (have_view)
	{
		projection_view_inv = view;
		projection_view_inv.Invert();
	}
	else
	{
		projection_view_inv = Matrix4f::Identity();
	}

	projection_view_inv_dirty = false;
}

}
}
