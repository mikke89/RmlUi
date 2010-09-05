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

#ifndef ROCKETINVADERSEVENTMANAGER_H
#define ROCKETINVADERSEVENTMANAGER_H

#include <Rocket/Core/Event.h>

class EventHandler;

/**
	@author Peter Curry
 */

class EventManager
{
public:
	/// Releases all event handlers registered with the manager.
	static void Shutdown();

	/// Registers a new event handler with the manager.
	/// @param[in] handler_name The name of the handler; this must be the same as the window it is handling events for.
	/// @param[in] handler The event handler.
	static void RegisterEventHandler(const Rocket::Core::String& handler_name, EventHandler* handler);

	/// Processes an event coming through from Rocket.
	/// @param[in] event The Rocket event that spawned the application event.
	/// @param[in] value The application-specific event value.
	static void ProcessEvent(Rocket::Core::Event& event, const Rocket::Core::String& value);
	/// Loads a window and binds the event handler for it.
	/// @param[in] window_name The name of the window to load.
	static bool LoadWindow(const Rocket::Core::String& window_name);

private:
	EventManager();
	~EventManager();
};


#endif
