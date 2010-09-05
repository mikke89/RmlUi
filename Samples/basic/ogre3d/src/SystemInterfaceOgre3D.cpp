/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
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
bool SystemInterfaceOgre3D::LogMessage(EMP::Core::Log::Type type, const EMP::Core::String& message)
{
	Ogre::LogMessageLevel message_level;
	switch (type)
	{
		case EMP::Core::Log::LT_ALWAYS:
		case EMP::Core::Log::LT_ERROR:
		case EMP::Core::Log::LT_ASSERT:
		case EMP::Core::Log::LT_WARNING:
			message_level = Ogre::LML_CRITICAL;
			break;

		default:
			message_level = Ogre::LML_NORMAL;
			break;
	}

	Ogre::LogManager::getSingleton().logMessage(message.CString(), message_level);
	return false;
}
