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
#include "../../Include/Rocket/Core/Transform.h"
#include "../../Include/Rocket/Core/TransformPrimitive.h"
#include "../../Include/Rocket/Core/ViewState.h"

namespace Rocket {
namespace Core {

// Default constructor, initializes an identity transform
Transform::Transform()
	: primitives()
{
}

Transform::Transform(const Transform& other)
	: primitives()
{
	primitives.reserve(other.primitives.size());
	Primitives::const_iterator i = other.primitives.begin();
	Primitives::const_iterator end = other.primitives.end();
	for (; i != end; ++i)
	{
		try
		{
			AddPrimitive(**i);
		}
		catch(...)
		{
			ClearPrimitives();
			throw;
		}
	}
}

Transform::~Transform()
{
	ClearPrimitives();
}

// Swap the content of two Transfrom instances
void Transform::Swap(Transform& other)
{
	primitives.swap(other.primitives);
}

// Assignment operato
const Transform& Transform::operator=(const Transform& other)
{
	Transform result(other);
	Swap(result);
	return *this;
}

// Remove all Primitives from this Transform
void Transform::ClearPrimitives()
{
	Primitives::iterator i = primitives.begin();
	Primitives::iterator end = primitives.end();
	for (; i != end; ++i)
	{
		try
		{
			delete *i;
			*i = 0;
		}
		catch(...)
		{
		}
	}
	primitives.clear();
}

// Add a Primitive to this Transform
void Transform::AddPrimitive(const Transforms::Primitive& p)
{
	Transforms::Primitive* q = p.Clone();
	if (!q)
	{
		throw std::bad_alloc();
	}
	try
	{
		primitives.push_back(q);
	}
	catch (...)
	{
		delete q;
		throw;
	}
}

}
}
