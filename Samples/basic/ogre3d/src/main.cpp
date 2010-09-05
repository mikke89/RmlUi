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

#include "RocketApplication.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE EMP_UNUSED(instance_handle), HINSTANCE EMP_UNUSED(previous_instance_handle), char* EMP_UNUSED(command_line), int EMP_UNUSED(command_show))
#else
int main(int EMP_UNUSED(argc), char** EMP_UNUSED(argv))
#endif
{
	RocketApplication application;
	try
	{
		application.go();
	}
	catch (Exception& e)
	{
		#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
	        MessageBox(NULL, e.getFullDescription().c_str(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		#else
			fprintf(stderr, "An exception has occurred: %s\n", e.getFullDescription().c_str());
		#endif
	}

	return 0;
}
