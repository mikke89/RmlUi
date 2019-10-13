/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2014 Markus Schöngart
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

#ifndef RMLUICORETRANSFORMSTATE_H
#define RMLUICORETRANSFORMSTATE_H

#include "Header.h"
#include "Types.h"

namespace Rml {
namespace Core {


class RMLUICORE_API TransformState
{
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

}
}

#endif
