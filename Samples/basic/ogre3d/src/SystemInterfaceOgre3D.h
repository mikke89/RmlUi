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

#ifndef SYSTEMINTERFACE3D_H
#define SYSTEMINTERFACE3D_H

#include <Rocket/Core/SystemInterface.h>
#include <Ogre/Ogre.h>

/**
	A sample system interface for Rocket into Ogre3D.

	@author Peter Curry
 */

class SystemInterfaceOgre3D : public Rocket::Core::SystemInterface
{
	public:
		SystemInterfaceOgre3D();
		virtual ~SystemInterfaceOgre3D();

		/// Gets the number of seconds elapsed since the start of the application.
		virtual float GetElapsedTime();

		/// Logs the specified message.
		virtual bool LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message);

	private:
		Ogre::Timer timer;
};

#endif
