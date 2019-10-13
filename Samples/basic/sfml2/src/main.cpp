/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 Nuno Silva
 * Copyright (c) 2019 The RmlUi Team, and contributors
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

/*
 * Modifed 2013 by Megavolt2013 in order to get the SFML2 sample working
 * with the revised libraries of SFML2
 * Please check the comments starting with NOTE in the files "main.cpp" and
 * "RenderInterfaceSFML.h" if you have trouble building this sample
 */

// NOTE: uncomment this only when you want to use the
// OpenGL Extension Wrangler Library (GLEW)
//#include <GL/glew.h>

#include <RmlUi/Core.h>
#include "SystemInterfaceSFML.h"
#include "RenderInterfaceSFML.h"
#include <RmlUi/Core/Input.h>
#include <RmlUi/Debugger/Debugger.h>
#include <Shell.h>
#include <ShellFileInterface.h>

int main(int argc, char **argv)
{
#ifdef RMLUI_PLATFORM_WIN32
        AllocConsole();
#endif

        int window_width = 1024;
        int window_height = 768;

	sf::RenderWindow MyWindow(sf::VideoMode(window_width, window_height), "RmlUi with SFML2", sf::Style::Close);
	MyWindow.setVerticalSyncEnabled(true);

#ifdef ENABLE_GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  /* Problem: glewInit failed, something is seriously wrong. */
	  fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	  //...
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

	RmlUiSFMLRenderer Renderer;
	RmlUiSFMLSystemInterface SystemInterface;

	// NOTE: if fonts and rml are not found you'll probably have to adjust
	// the path information in the string
	Rml::Core::String root = Shell::FindSamplesRoot();
	ShellFileInterface FileInterface(root);

	if(!MyWindow.isOpen())
		return 1;

	Renderer.SetWindow(&MyWindow);

	Rml::Core::SetFileInterface(&FileInterface);
	Rml::Core::SetRenderInterface(&Renderer);
	Rml::Core::SetSystemInterface(&SystemInterface);


	if(!Rml::Core::Initialise())
		return 1;

	Rml::Core::LoadFontFace("assets/Delicious-Bold.otf");
	Rml::Core::LoadFontFace("assets/Delicious-BoldItalic.otf");
	Rml::Core::LoadFontFace("assets/Delicious-Italic.otf");
	Rml::Core::LoadFontFace("assets/Delicious-Roman.otf");

	Rml::Core::Context *Context = Rml::Core::CreateContext("default",
		Rml::Core::Vector2i(MyWindow.getSize().x, MyWindow.getSize().y));

	Rml::Debugger::Initialise(Context);

	Rml::Core::ElementDocument *Document = Context->LoadDocument("assets/demo.rml");

	if(Document)
	{
		Document->Show();
		fprintf(stdout, "\nDocument loaded");
	}
	else
	{
		fprintf(stdout, "\nDocument is nullptr");
	}

	while(MyWindow.isOpen())
	{
		static sf::Event event;

		MyWindow.clear();
		Context->Render();
		MyWindow.display();

		while(MyWindow.pollEvent(event))
		{
			switch(event.type)
			{
			case sf::Event::Resized:
				Renderer.Resize();
				break;
			case sf::Event::MouseMoved:
				Context->ProcessMouseMove(event.mouseMove.x, event.mouseMove.y,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::MouseButtonPressed:
				Context->ProcessMouseButtonDown(event.mouseButton.button,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::MouseButtonReleased:
				Context->ProcessMouseButtonUp(event.mouseButton.button,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::MouseWheelMoved:
				Context->ProcessMouseWheel(-event.mouseWheel.delta,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::TextEntered:
				if (event.text.unicode > 32)
					Context->ProcessTextInput(event.text.unicode);
				break;
			case sf::Event::KeyPressed:
				Context->ProcessKeyDown(SystemInterface.TranslateKey(event.key.code),
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::KeyReleased:
				if(event.key.code == sf::Keyboard::F8)
				{
					Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
				};

				if(event.key.code == sf::Keyboard::Escape) {
					MyWindow.close();
				}

				Context->ProcessKeyUp(SystemInterface.TranslateKey(event.key.code),
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::Closed:
				MyWindow.close();
				break;
			default:
				break;
			};
		};

		Context->Update();
	};

	Rml::Core::Shutdown();

	return 0;
};
