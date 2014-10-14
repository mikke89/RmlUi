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

#ifndef ROCKETCORETRANSFORMSTATE_H
#define ROCKETCORETRANSFORMSTATE_H

#include "Header.h"
#include "Types.h"

namespace Rocket {
namespace Core {

/**
	A TransformState captures an element's current perspective and transform settings.

	@author Markus Schöngart
 */

class ROCKETCORE_API TransformState
{
public:
	struct Perspective
	{
		/// Calculates the projection matrix.
		Matrix4f GetProjection() const throw();

		/// Calculates the clip space coordinates ([-1; 1]³) of a 3D vertex in world space.
		/// @param[in] point The point in world space coordinates.
		/// @return The clip space coordinates of the point.
		Vector3f Project(const Vector3f &point) const throw();
		/// Calculates the world space coordinates of a 3D vertex in clip space ([-1; 1]³).
		/// @param[in] point The point in clip space coordinates.
		/// @return The world space coordinates of the point.
		Vector3f Unproject(const Vector3f &point) const throw();
	
		float		distance;	// The CSS `perspective:' value
		Vector2i	view_size;
		Vector2f	vanish;		// The vanishing point, in [0; 1]²; Only relevant if distance > 0
	};

	struct LocalPerspective
	{
		/// Calculates the projection matrix.
		Matrix4f GetProjection() const throw();

		/// Calculates the clip space coordinates ([-1; 1]³) of a 3D vertex in world space.
		/// @param[in] point The point in world space coordinates.
		/// @return The clip space coordinates of the point.
		Vector3f Project(const Vector3f &point) const throw();
		/// Calculates the world space coordinates of a 3D vertex in clip space ([-1; 1]³).
		/// @param[in] point The point in clip space coordinates.
		/// @return The world space coordinates of the point.
		Vector3f Unproject(const Vector3f &point) const throw();

		float		distance;	// The CSS `perspective:' value
		Vector2i	view_size;
	};

	TransformState();

	/// Stores a new perspective value
	void SetPerspective(const Perspective *perspective) throw();
	/// Returns the perspective value
	bool GetPerspective(Perspective *perspective) const throw();

	/// Stores a new local perspective value
	void SetLocalPerspective(const LocalPerspective *local_perspective) throw();
	/// Returns the local perspective value
	bool GetLocalPerspective(LocalPerspective *local_perspective) const throw();

	/// Stores a new transform matrix
	void SetTransform(const Matrix4f *transform) throw();
	/// Returns the stored transform matrix
	bool GetTransform(Matrix4f *transform) const throw();

	/// Stores a new recursive parent transform.
	void SetParentRecursiveTransform(const Matrix4f *parent_recursive_transform) throw();
	/// Returns the stored recursive parent transform matrix
	bool GetParentRecursiveTransform(Matrix4f *transform) const throw();

	/// Transforms a 3D point by the `parent transform' and `transform' matrices stored in this TransformState.
	/// @param[in] point The point in world space coordinates.
	/// @return The transformed point in world space coordinates.
	Vector3f Transform(const Vector3f &point) const throw();
	/// Transforms a 3D point by the inverse `parent transform' and `transform' matrices stored in this TransformState.
	/// @param[in] point The point in world space coordinates.
	/// @return The transformed point in world space coordinates.
	Vector3f Untransform(const Vector3f &point) const throw();

	/// Returns the parent's recursive transform multiplied by this transform.
	bool GetRecursiveTransform(Matrix4f *recursive_transform) const throw();

private:
	// Flags for stored values
	bool have_perspective;
	bool have_local_perspective;
	bool have_parent_recursive_transform;
	bool have_transform;

	// Stored values
	float perspective, local_perspective;
	Vector2i view_size;
	Vector2f vanish;
	Matrix4f parent_recursive_transform;
	Matrix4f transform;
};

}
}

#endif
