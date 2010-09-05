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

#ifndef ROCKETAPPLICATION_H
#define ROCKETAPPLICATION_H

#include <Rocket/Core/String.h>
#include <ExampleApplication.h>
#include <Ogre.h>

namespace Rocket {
namespace Core {

class Context;

}
}

class SystemInterfaceOgre3D;
class RenderInterfaceOgre3D;

/**
	@author Peter Curry
 */

class RocketApplication : public ExampleApplication, public Ogre::RenderQueueListener
{
	public:
		RocketApplication();
		virtual ~RocketApplication();

	protected:
		void createScene();
		void destroyScene();

		void createFrameListener();

		/// Called from Ogre before a queue group is rendered.
		virtual void renderQueueStarted(uint8 queueGroupId, const Ogre::String& invocation, bool& skipThisInvocation);
		/// Called from Ogre after a queue group is rendered.
        virtual void renderQueueEnded(uint8 queueGroupId, const Ogre::String& invocation, bool& repeatThisInvocation);

	private:
		// Configures Ogre's rendering system for rendering Rocket.
		void ConfigureRenderSystem();
		// Builds an OpenGL-style orthographic projection matrix.
		void BuildProjectionMatrix(Ogre::Matrix4& matrix);

		// Absolute path to the libRocket directory.
		EMP::Core::String rocket_path;
		// Absolute path to the Ogre3d sample directory;
		EMP::Core::String sample_path;

		Rocket::Core::Context* context;

		SystemInterfaceOgre3D* ogre_system;
		RenderInterfaceOgre3D* ogre_renderer;
};

#endif
