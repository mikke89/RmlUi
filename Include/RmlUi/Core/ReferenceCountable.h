/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef RMLUICOREREFERENCECOUNTABLE_H
#define RMLUICOREREFERENCECOUNTABLE_H

#include "Header.h"

namespace Rml {
namespace Core {

/**
	A base class for any class that wishes to be reference counted.
	@author Robert Curry
*/

class RMLUICORE_API ReferenceCountable
{
public:
	/// Constructor.
	/// @param[in] initial_count The initial reference count of the object.
	ReferenceCountable(int initial_count = 1);

	/// Returns the number of references outstanding against this object.
	int GetReferenceCount();
	/// Increases the reference count. If this pushes the count above 0, OnReferenceActivate() will be called. 
	void AddReference();
	/// Decreases the reference count. If this pushes the count to 0, OnReferenceDeactivate() will be called. 
	void RemoveReference();

	/// Reference countable objects should not be copied
	ReferenceCountable(const ReferenceCountable&) = delete;
	ReferenceCountable& operator=(const ReferenceCountable&) = delete;

	/// If any reference countable objects are still allocated, this function will write a leak report to the log.
	static void DumpLeakReport();

protected:		
	/// Destructor. The reference count must be 0 when this is invoked.
	~ReferenceCountable();

	/// A hook method called when the reference count climbs from 0.
	virtual void OnReferenceActivate();
	/// A hook method called when the reference count drops to 0.
	virtual void OnReferenceDeactivate();

private:
	// The number of references against this object.
	int reference_count;
};

}
}

#endif
