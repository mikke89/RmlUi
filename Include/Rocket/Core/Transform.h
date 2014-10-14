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

#ifndef ROCKETCORETRANSFORM_H
#define ROCKETCORETRANSFORM_H

#include "Header.h"
#include "ReferenceCountable.h"

namespace Rocket {
namespace Core {

class ViewState;
namespace Transforms { class Primitive; }

/**
	The Transform class holds the information parsed from an element's
	`transform' property.  It is one of the primitive types that a Variant
	can assume.  The method `ComputeFinalTransform()' computes the
	transformation matrix that is to be applied to the current
	projection/view matrix in order to render the associated element.

	@author Markus Schöngart
	@see Rocket::Core::Variant
 */

class ROCKETCORE_API Transform : public ReferenceCountable
{
public:
	/// Default constructor, initializes an identity transform
	Transform();

	/// Copy constructor
	Transform(const Transform& other);

	/// Destructor
	~Transform();

	/// Swap the content of two Transform instances
	void Swap(Transform& other);

	/// Assignment operator
	const Transform& operator=(const Transform& other);

	/// Remove all Primitives from this Transform
	void ClearPrimitives();
	/// Add a Primitive to this Transform
	void AddPrimitive(const Transforms::Primitive& p);
	/// Return the number of Primitives in this Transform
	int GetNumPrimitives() const throw()
		{ return primitives.size(); }
	/// Return the i-th Primitive in this Transform
	const Transforms::Primitive& GetPrimitive(int i) const throw()
		{ return *primitives[i]; }

protected:
	void OnReferenceDeactivate()
		{ delete this; }

private:
	typedef std::vector< Transforms::Primitive * > Primitives;
	Primitives primitives;
};

}
}

#endif
