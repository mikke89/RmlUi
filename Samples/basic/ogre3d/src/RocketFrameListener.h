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

#ifndef ROCKETFRAMELISTENER_H
#define ROCKETFRAMELISTENER_H

#include <Rocket/Core/Context.h>
#include <ExampleFrameListener.h>

class RocketApplication;

/**
	@author Peter Curry
 */

class RocketFrameListener : public ExampleFrameListener, public OIS::MouseListener, public OIS::KeyListener
{
	public:
		RocketFrameListener(Ogre::RenderWindow* window, Ogre::Camera* camera, Rocket::Core::Context* context);
		virtual ~RocketFrameListener();

		// FrameListener interface.
		virtual bool frameStarted(const Ogre::FrameEvent& evt);

		// MouseListener interface.
		virtual bool mouseMoved(const OIS::MouseEvent& e);
		virtual bool mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id);
		virtual bool mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID id);

		// KeyListener interface.
		virtual bool keyPressed(const OIS::KeyEvent& e);
		virtual bool keyReleased(const OIS::KeyEvent& e);

	private:
		void BuildKeyMaps();
		int GetKeyModifierState();

		typedef std::map< OIS::KeyCode, Rocket::Core::Input::KeyIdentifier > KeyIdentifierMap;

		Rocket::Core::Context* context;
		bool running;

		KeyIdentifierMap key_identifiers;
};

#endif
