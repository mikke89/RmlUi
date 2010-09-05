/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef ROCKETINVADERSDECORATORINSTANCERDEFENDER_H
#define ROCKETINVADERSDECORATORINSTANCERDEFENDER_H

#include <Rocket/Core/DecoratorInstancer.h>

/**
	@author Robert Curry
 */

class DecoratorInstancerDefender : public Rocket::Core::DecoratorInstancer
{
public:
	DecoratorInstancerDefender();
	virtual ~DecoratorInstancerDefender();

	/// Instances a decorator given the property tag and attributes from the RCSS file.
	/// @param[in] name The type of decorator desired. For example, "background-decorator: simple;" is declared as type "simple".
	/// @param[in] properties All RCSS properties associated with the decorator.
	/// @return The decorator if it was instanced successful, NULL if an error occured.
	Rocket::Core::Decorator* InstanceDecorator(const Rocket::Core::String& name, const Rocket::Core::PropertyDictionary& properties);
	/// Releases the given decorator.
	/// @param[in] decorator Decorator to release. This is guaranteed to have been constructed by this instancer.
	void ReleaseDecorator(Rocket::Core::Decorator* decorator);

	/// Releases the instancer.
	void Release();
};

#endif
