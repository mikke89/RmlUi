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

#ifndef ROCKETAPPLICATION_H
#define ROCKETAPPLICATION_H

#include <Rocket/Core/String.h>
#include <Ogre/ExampleApplication.h>
#include <Ogre/Ogre.h>

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
		Rocket::Core::String rocket_path;
		// Absolute path to the Ogre3d sample directory;
		Rocket::Core::String sample_path;

		Rocket::Core::Context* context;

		SystemInterfaceOgre3D* ogre_system;
		RenderInterfaceOgre3D* ogre_renderer;
};

#endif
