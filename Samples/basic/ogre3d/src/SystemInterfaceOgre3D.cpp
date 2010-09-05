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

#include "SystemInterfaceOgre3D.h"

SystemInterfaceOgre3D::SystemInterfaceOgre3D()
{
}

SystemInterfaceOgre3D::~SystemInterfaceOgre3D()
{
}

// Gets the number of seconds elapsed since the start of the application.
float SystemInterfaceOgre3D::GetElapsedTime()
{
	return timer.getMilliseconds() * 0.001f;
}

// Logs the specified message.
bool SystemInterfaceOgre3D::LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message)
{
	Ogre::LogMessageLevel message_level;
	switch (type)
	{
		case Rocket::Core::Log::LT_ALWAYS:
		case Rocket::Core::Log::LT_ERROR:
		case Rocket::Core::Log::LT_ASSERT:
		case Rocket::Core::Log::LT_WARNING:
			message_level = Ogre::LML_CRITICAL;
			break;

		default:
			message_level = Ogre::LML_NORMAL;
			break;
	}

	Ogre::LogManager::getSingleton().logMessage(message.CString(), message_level);
	return false;
}
