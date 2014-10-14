/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2014 Markus Schöngart
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

#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>
#include <Input.h>
#include <Shell.h>

#include <cmath>
#include <sstream>

class DemoWindow
{
public:
	DemoWindow(const Rocket::Core::String &title, const Rocket::Core::Vector2f &position, Rocket::Core::Context *context)
	{
		document = context->LoadDocument("data/transform.rml");
		if (document != NULL)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->SetProperty("left", Rocket::Core::Property(position.x, Rocket::Core::Property::PX));
			document->SetProperty("top", Rocket::Core::Property(position.y, Rocket::Core::Property::PX));
			document->Show();
		}
	}

	~DemoWindow()
	{
		if (document)
		{
			document->RemoveReference();
			document->Close();
		}
	}

	void SetPerspective(float distance)
	{
		if (document)
		{
			std::stringstream s;
			s << distance;
			document->SetProperty("perspective", s.str().c_str());
		}
	}

	void SetPerspectiveOrigin(float x, float y)
	{
		if (document)
		{
			std::stringstream s;
			s << x * 100 << "%" << " " << y * 100 << "%";
			document->SetProperty("perspective-origin", s.str().c_str());
		}
	}

	void SetRotation(float degrees)
	{
		if (document)
		{
			std::stringstream s;
			s << "rotate3d(0.0, 1.0, 0.0, " << degrees << ")";
			document->SetProperty("transform", s.str().c_str());
		}
	}

private:
	Rocket::Core::ElementDocument *document;
};

Rocket::Core::Context* context = NULL;
ShellRenderInterfaceExtensions *shell_renderer;
DemoWindow* window_1 = NULL;
DemoWindow* window_2 = NULL;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();

	static float deg = 0;
	Rocket::Core::SystemInterface* system_interface = Rocket::Core::GetSystemInterface();
	deg = std::fmod(system_interface->GetElapsedTime() * 30.0f, 360.0f);
	if (window_1)
	{
		window_1->SetRotation(deg);
	}
	if (window_2)
	{
		window_2->SetRotation(deg);
	}
}

#if defined ROCKET_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE ROCKET_UNUSED_PARAMETER(instance_handle), HINSTANCE ROCKET_UNUSED_PARAMETER(previous_instance_handle), char* ROCKET_UNUSED_PARAMETER(command_line), int ROCKET_UNUSED_PARAMETER(command_show))
#else
int main(int ROCKET_UNUSED_PARAMETER(argc), char** ROCKET_UNUSED_PARAMETER(argv))
#endif
{
#ifdef ROCKET_PLATFORM_WIN32
	ROCKET_UNUSED(instance_handle);
	ROCKET_UNUSED(previous_instance_handle);
	ROCKET_UNUSED(command_line);
	ROCKET_UNUSED(command_show);
#else
	ROCKET_UNUSED(argc);
	ROCKET_UNUSED(argv);
#endif

	ShellRenderInterfaceOpenGL opengl_renderer;
	shell_renderer = &opengl_renderer;

	// Generic OS initialisation, creates a window and attaches OpenGL.
	if (!Shell::Initialise("../Samples/basic/transform/") ||
		!Shell::OpenWindow("Transform Sample", shell_renderer, 1024, 768, true))
	{
		Shell::Shutdown();
		return -1;
	}

	// Rocket initialisation.
	Rocket::Core::SetRenderInterface(&opengl_renderer);
	opengl_renderer.SetViewport(1024,768);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(1024, 768));
	if (context == NULL)
	{
		Rocket::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rocket::Controls::Initialise();
	Rocket::Debugger::Initialise(context);
	Input::SetContext(context);
	shell_renderer->SetContext(context);

	Shell::LoadFonts("../../assets/");

	window_1 = new DemoWindow("Orthographic transform", Rocket::Core::Vector2f(81, 200), context);
	if (window_1)
	{
		window_1->SetPerspective(0);
	}
	window_2 = new DemoWindow("Perspective transform", Rocket::Core::Vector2f(593, 200), context);
	if (window_2)
	{
		window_2->SetPerspective(800);
		window_2->SetPerspectiveOrigin(0.5, 0.75);
	}

	Shell::EventLoop(GameLoop);

	delete window_1;
	delete window_2;

	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
