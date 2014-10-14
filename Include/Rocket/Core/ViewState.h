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

#ifndef ROCKETCOREVIEWSTATE_H
#define ROCKETCOREVIEWSTATE_H

#include "Header.h"
#include "Types.h"

namespace Rocket {
namespace Core {

/**
	A ViewState captures the current global projection and view matrices (`camera settings').

	@author Markus Schöngart
 */

class ROCKETCORE_API ViewState
{
public:
	ViewState();

	/// Stores a new projection matrix
	void SetProjection(const Matrix4f *projection) throw();

	/// Stores a new view matrix
	void SetView(const Matrix4f *view) throw();

	/// Retrieves the cancellation matrix (projection * view)⁻¹
	bool GetProjectionViewInv(Matrix4f& projection_view_inv) const throw();

	/// Calculates the clip space coordinates ([-1; 1]³) of a 3D vertex in world space.
	/// @param[in] point The point in world space coordinates.
	/// @return The clip space coordinates of the point.
	Vector3f Project(const Vector3f &point) const throw();
	/// Calculates the world space coordinates of a 3D vertex in clip space ([-1; 1]³).
	/// @param[in] point The point in clip space coordinates.
	/// @return The world space coordinates of the point.
	Vector3f Unproject(const Vector3f &point) const throw();

private:
	// Flags for stored values
	bool have_projection;
	bool have_view;

	// Flags for cached values
	mutable bool projection_view_inv_dirty;

	// Stored values
	Matrix4f projection;
	Matrix4f view;

	// Cached values
	mutable Matrix4f projection_view_inv;
	void UpdateProjectionViewInv() const throw();
};

}
}

#endif
