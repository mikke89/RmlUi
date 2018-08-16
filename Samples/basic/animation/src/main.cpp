/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2018 Michael Ragazzon
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
		document = context->LoadDocument("basic/animation/data/animation.rml");
		if (document != NULL)
		{
			document->GetElementById("title")->SetInnerRML(title);
			document->SetProperty("left", Rocket::Core::Property(position.x, Rocket::Core::Property::PX));
			document->SetProperty("top", Rocket::Core::Property(position.y, Rocket::Core::Property::PX));

			auto el = document->GetElementById("high_score");
			el->Animate("margin-left", Rocket::Core::Property(0.f, Rocket::Core::Property::PX), 0.3f, 10, true);
			el->Animate("margin-left", Rocket::Core::Property(100.f, Rocket::Core::Property::PX), 0.6f);

			el = document->GetElementById("exit");
			el->Animate("margin-left", Rocket::Core::Property(100.f, Rocket::Core::Property::PX), 8.0f, -1, true);

			el = document->GetElementById("help");
			el->Animate("image-color", Rocket::Core::Property(Rocket::Core::Colourb(128, 255, 255, 255), Rocket::Core::Property::COLOUR), 0.3f, -1, false);
			el->Animate("image-color", Rocket::Core::Property(Rocket::Core::Colourb(128, 128, 255, 255), Rocket::Core::Property::COLOUR), 0.3f);
			el->Animate("image-color", Rocket::Core::Property(Rocket::Core::Colourb(0, 128, 128, 255), Rocket::Core::Property::COLOUR), 0.3f);
			el->Animate("image-color", Rocket::Core::Property(Rocket::Core::Colourb(64, 128, 255, 0), Rocket::Core::Property::COLOUR), 0.9f);
			el->Animate("image-color", Rocket::Core::Property(Rocket::Core::Colourb(255, 255, 255, 255), Rocket::Core::Property::COLOUR), 0.3f);

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

	Rocket::Core::ElementDocument * GetDocument() {
		return document;
	}

private:
	Rocket::Core::ElementDocument *document;
};


Rocket::Core::Context* context = NULL;
ShellRenderInterfaceExtensions *shell_renderer;
DemoWindow* window = NULL;

void GameLoop()
{
	context->Update();

	shell_renderer->PrepareRenderBuffer();
	context->Render();
	shell_renderer->PresentRenderBuffer();

	//auto el = window->GetDocument()->GetElementById("exit");
	//auto f = el->GetProperty<int>("margin-left");
	//static int f_prev = 0.0f;
	//int df = f - f_prev;
	//f_prev = f;
	//if(df != 0)
	//	Rocket::Core::Log::Message(Rocket::Core::Log::LT_INFO, "Animation f = %d,  df = %d", f, df);
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
	if (!Shell::Initialise("../../Samples/") ||
		!Shell::OpenWindow("Animation Sample", shell_renderer, 1024, 768, true))
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

	Shell::LoadFonts("assets/");

	window = new DemoWindow("Animation sample", Rocket::Core::Vector2f(81, 200), context);


	Shell::EventLoop(GameLoop);

	delete window;

	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
