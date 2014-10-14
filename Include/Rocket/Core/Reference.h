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

#ifndef ROCKETCOREREFERENCE_H
#define ROCKETCOREREFERENCE_H

#include "Header.h"
#include <algorithm>

namespace Rocket {
namespace Core {

template< class ReferenceCountable >
class ROCKETCORE_API SharedReference;

template< class ReferenceCountable >
class ROCKETCORE_API SharedConstReference;

/**
	A smart pointer class template that manages ReferenceCountables.
	SharedReference allows unrestricted access to the shared object via every reference.
	@author Markus Schöngart
	@see ReferenceCountable
*/
template< class ReferenceCountable >
class ROCKETCORE_API SharedReference
{
public:
	typedef ReferenceCountable ReferenceType;
	typedef SharedReference< ReferenceCountable > ThisType;
	typedef SharedConstReference< ReferenceCountable > ReadOnlyType;

	/// Constructor. Does not increase the object's reference count.
	/// @param[in] object The object to refer to.
	SharedReference(ReferenceType* object = 0) : object(object)
		{ /*if (object) object->AddReference();*/ }
	/// Copy constructor. Increases the object's reference count.
	/// @param[in] other The other Reference.
	SharedReference(const ThisType& other) : object(other.object)
		{ if (object) object->AddReference(); }
	/// Destructor. Decrements the stored object's reference count.
	~SharedReference()
		{ if (object) object->RemoveReference(); }

	/// Swaps the contents of two References.
	void Swap(ThisType& other) throw()
		{ std::swap(object, other.object); }

	/// Assign another referenced object to this smart pointer
	const ThisType& operator=(ReferenceType* ref)
		{ ThisType tmp(ref); Swap(tmp); return *this; }
	/// Assign another referenced object to this smart pointer
	const ThisType& operator=(const ThisType& other)
		{ ThisType tmp(other); Swap(tmp); return *this; }

	const ReferenceType& operator*() const throw()
		{ return *object; }
	ReferenceType& operator*() throw()
		{ return *object; }

	const ReferenceType* operator->() const throw()
		{ return object; }
	ReferenceType* operator->() throw()
		{ return object; }

	friend ReadOnlyType;

private:
	mutable ReferenceType *object;
};

/**
	A smart pointer class template that manages ReferenceCountables.
	SharedConstReference allows read-only access to the shared object via every reference.
	@author Markus Schöngart
	@see ReferenceCountable
*/
template< class ReferenceCountable >
class ROCKETCORE_API SharedConstReference
{
public:
	typedef ReferenceCountable ReferenceType;
	typedef SharedConstReference< ReferenceCountable > ThisType;
	typedef SharedReference< ReferenceCountable > ReadWriteType;

	/// Constructor. Does not increase the object's reference count.
	/// @param[in] object The object to refer to.
	SharedConstReference(ReferenceType* object = 0) : object(object)
		{ /*if (object) object->AddReference();*/ }
	/// Copy constructor. Increases the object's reference count.
	/// @param[in] other The other Reference.
	SharedConstReference(const ThisType& other) : object(other.object)
		{ if (object) object->AddReference(); }
	/// Constructor from read-write reference. Increases the object's reference count.
	/// @param[in] other The other Reference.
	SharedConstReference(const ReadWriteType& other) : object(other.object)
		{ if (object) object->AddReference(); }
	/// Destructor. Decrements the stored object's reference count.
	~SharedConstReference()
		{ if (object) object->RemoveReference(); }

	/// Swaps the contents of two References.
	void Swap(ThisType& other) throw()
		{ std::swap(object, other.object); }

	/// Assign another referenced object to this smart pointer
	const ThisType& operator=(ReferenceType* ref)
		{ ThisType tmp(ref); Swap(tmp); return *this; }
	/// Assign another referenced object to this smart pointer
	const ThisType& operator=(const ThisType& other)
		{ ThisType tmp(other); Swap(tmp); return *this; }

	const ReferenceType& operator*() const throw()
		{ return *object; }

	const ReferenceType* operator->() const throw()
		{ return object; }

private:
	mutable ReferenceType *object;
};

}
}

#endif
