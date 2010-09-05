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

#ifndef SYSTEMINTERFACE_H
#define SYSTEMINTERFACE_H

#include <Rocket/Core/SystemInterface.h>

/**
	Sample custom Rocket system interface for writing log messages to file.
	@author Lloyd Weehuizen
 */

class SystemInterface : public Rocket::Core::SystemInterface
{
public:
	SystemInterface();
	virtual ~SystemInterface();

	/// Get the number of seconds elapsed since the start of the application.
	/// @return Elapsed time, in seconds.
	virtual float GetElapsedTime();

	/// Log the specified message.
	/// @param[in] type Type of log message, ERROR, WARNING, etc.
	/// @param[in] message Message to log.
	/// @return True to continue execution, false to break into the debugger.
	virtual bool LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message);

private:
	FILE* fp;
};

#endif
