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

#ifndef RMLUICOREEVENTITERATORS_H
#define RMLUICOREEVENTITERATORS_H

#include "../../Include/RmlUi/Core/Element.h"

namespace Rml {
namespace Core {

/**
	An STL unary functor for dispatching an event to a Element.

	@author Peter Curry
 */

class RmlEventFunctor
{
public:
	RmlEventFunctor(EventId id, const Dictionary& parameters) : id(id), parameters(&parameters) {}

	void operator()(Element* element)
	{
		element->DispatchEvent(id, *parameters);
	}

private:
	EventId id;
	const Dictionary* parameters;
};

/**
	An STL unary functor for setting or clearing a pseudo-class on a Element.

	@author Pete
 */

class PseudoClassFunctor
{
	public:
		PseudoClassFunctor(const String& pseudo_class, bool set) : pseudo_class(pseudo_class)
		{
			this->set = set;
		}

		void operator()(Element* element)
		{
			element->SetPseudoClass(pseudo_class, set);
		}

	private:
		String pseudo_class;
		bool set;
};

}
}

#endif
