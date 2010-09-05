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

#ifndef SYSTEMINTERFACE3D_H
#define SYSTEMINTERFACE3D_H

#include <Rocket/Core/SystemInterface.h>
#include <Ogre.h>

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
		virtual bool LogMessage(EMP::Core::Log::Type type, const EMP::Core::String& message);

	private:
		Ogre::Timer timer;
};

#endif
