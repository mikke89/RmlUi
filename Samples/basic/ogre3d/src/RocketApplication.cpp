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

#include "RocketApplication.h"
#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>
#include "RenderInterfaceOgre3D.h"
#include "RocketFrameListener.h"
#include "SystemInterfaceOgre3D.h"
#include <direct.h>

RocketApplication::RocketApplication()
{
	context = NULL;
	ogre_system = NULL;
	ogre_renderer = NULL;

	// Switch the working directory to Ogre's bin directory.
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	char path[MAX_PATH];
	rocket_path = getcwd(path, MAX_PATH);
	rocket_path += "/../../";

	// Normalise the path. This path is used to specify the resource location (see line 56 below).
	_fullpath(path, rocket_path.CString(), MAX_PATH);
	rocket_path = Rocket::Core::String(path).Replace("\\", "/");

	// The sample path is the path to the Ogre3D sample directory. All resources are loaded
	// relative to this path.
	sample_path = getcwd(path, MAX_PATH);
	sample_path += "/../Samples/basic/ogre3d/";
#if OGRE_DEBUG_MODE
	chdir((Ogre::String(getenv("OGRE_HOME")) + "\\bin\\debug\\").c_str());
#else
	chdir((Ogre::String(getenv("OGRE_HOME")) + "\\bin\\release\\").c_str());
#endif
#endif
}

RocketApplication::~RocketApplication()
{
}

void RocketApplication::createScene()
{
	Ogre::ResourceGroupManager::getSingleton().createResourceGroup("Rocket");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(rocket_path.Replace("\\", "/").CString(), "FileSystem", "Rocket");

	// Rocket initialisation.
	ogre_renderer = new RenderInterfaceOgre3D(mWindow->getWidth(), mWindow->getHeight());
	Rocket::Core::SetRenderInterface(ogre_renderer);

	ogre_system = new SystemInterfaceOgre3D();
	Rocket::Core::SetSystemInterface(ogre_system);

	Rocket::Core::Initialise();
	Rocket::Controls::Initialise();

	// Load the fonts from the path to the sample directory.
	Rocket::Core::FontDatabase::LoadFontFace(sample_path + "../../assets/Delicious-Roman.otf");
	Rocket::Core::FontDatabase::LoadFontFace(sample_path + "../../assets/Delicious-Bold.otf");
	Rocket::Core::FontDatabase::LoadFontFace(sample_path + "../../assets/Delicious-Italic.otf");
	Rocket::Core::FontDatabase::LoadFontFace(sample_path + "../../assets/Delicious-BoldItalic.otf");

	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(mWindow->getWidth(), mWindow->getHeight()));
	Rocket::Debugger::Initialise(context);

	// Load the mouse cursor and release the caller's reference.
	Rocket::Core::ElementDocument* cursor = context->LoadMouseCursor(sample_path + "../../assets/cursor.rml");
	if (cursor)
		cursor->RemoveReference();

	Rocket::Core::ElementDocument* document = context->LoadDocument(sample_path + "../../assets/demo.rml");
	if (document)
	{
		document->Show();
		document->RemoveReference();
	}

	// Add the application as a listener to Ogre's render queue so we can render during the overlay.
	mSceneMgr->addRenderQueueListener(this);
}

void RocketApplication::destroyScene()
{
	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	delete ogre_system;
	ogre_system = NULL;

	delete ogre_renderer;
	ogre_renderer = NULL;
}

void RocketApplication::createFrameListener()
{
	// Create the RocketFrameListener.
	mFrameListener = new RocketFrameListener(mWindow, mCamera, context);
	mRoot->addFrameListener(mFrameListener);

	// Show the frame stats overlay.
	mFrameListener->showDebugOverlay(true);
}

// Called from Ogre before a queue group is rendered.
void RocketApplication::renderQueueStarted(uint8 queueGroupId, const Ogre::String& invocation, bool& ROCKET_UNUSED_PARAMETER(skipThisInvocation))
{
	ROCKET_UNUSED(skipThisInvocation);

	if (queueGroupId == Ogre::RENDER_QUEUE_OVERLAY && Ogre::Root::getSingleton().getRenderSystem()->_getViewport()->getOverlaysEnabled())
	{
		context->Update();

		ConfigureRenderSystem();
		context->Render();
	}
}

// Called from Ogre after a queue group is rendered.
void RocketApplication::renderQueueEnded(uint8 ROCKET_UNUSED_PARAMETER(queueGroupId), const Ogre::String& ROCKET_UNUSED_PARAMETER(invocation), bool& ROCKET_UNUSED_PARAMETER(repeatThisInvocation))
{
	ROCKET_UNUSED(queueGroupId);
	ROCKET_UNUSED(invocation);
	ROCKET_UNUSED(repeatThisInvocation);
}

// Configures Ogre's rendering system for rendering Rocket.
void RocketApplication::ConfigureRenderSystem()
{
	Ogre::RenderSystem* render_system = Ogre::Root::getSingleton().getRenderSystem();

	// Set up the projection and view matrices.
	Ogre::Matrix4 projection_matrix;
	BuildProjectionMatrix(projection_matrix);
	render_system->_setProjectionMatrix(projection_matrix);
	render_system->_setViewMatrix(Ogre::Matrix4::IDENTITY);

	// Disable lighting, as all of Rocket's geometry is unlit.
	render_system->setLightingEnabled(false);
	// Disable depth-buffering; all of the geometry is already depth-sorted.
	render_system->_setDepthBufferParams(false, false);
	// Rocket generates anti-clockwise geometry, so enable clockwise-culling.
	render_system->_setCullingMode(Ogre::CULL_CLOCKWISE);
	// Disable fogging.
	render_system->_setFog(Ogre::FOG_NONE);
	// Enable writing to all four channels.
	render_system->_setColourBufferWriteEnabled(true, true, true, true);
	// Unbind any vertex or fragment programs bound previously by the application.
	render_system->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
	render_system->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);

	// Set texture settings to clamp along both axes.
	Ogre::TextureUnitState::UVWAddressingMode addressing_mode;
	addressing_mode.u = Ogre::TextureUnitState::TAM_CLAMP;
	addressing_mode.v = Ogre::TextureUnitState::TAM_CLAMP;
	addressing_mode.w = Ogre::TextureUnitState::TAM_CLAMP;
	render_system->_setTextureAddressingMode(0, addressing_mode);

	// Set the texture coordinates for unit 0 to be read from unit 0.
	render_system->_setTextureCoordSet(0, 0);
	// Disable texture coordinate calculation.
	render_system->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);
	// Enable linear filtering; images should be rendering 1 texel == 1 pixel, so point filtering could be used
	// except in the case of scaling tiled decorators.
	render_system->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_POINT);
	// Disable texture coordinate transforms.
	render_system->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);
	// Reject pixels with an alpha of 0.
	render_system->_setAlphaRejectSettings(Ogre::CMPF_GREATER, 0, false);
	// Disable all texture units but the first.
	render_system->_disableTextureUnitsFrom(1);

	// Enable simple alpha blending.
	render_system->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

	// Disable depth bias.
	render_system->_setDepthBias(0, 0);
}

// Builds an OpenGL-style orthographic projection matrix.
void RocketApplication::BuildProjectionMatrix(Ogre::Matrix4& projection_matrix)
{
	float z_near = -1;
	float z_far = 1;

	projection_matrix = Ogre::Matrix4::ZERO;

	// Set up matrices.
	projection_matrix[0][0] = 2.0f / mWindow->getWidth();
	projection_matrix[0][3]= -1.0000000f;
	projection_matrix[1][1]= -2.0f / mWindow->getHeight();
	projection_matrix[1][3]= 1.0000000f;
	projection_matrix[2][2]= -2.0f / (z_far - z_near);
	projection_matrix[3][3]= 1.0000000f;
}
