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

#ifdef RMLUI_PLATFORM_WIN32
#include <windows.h>
#endif

float multiplier = 1.f;

void updateView(sf::RenderWindow& window, sf::View& view)
{
	view.reset(sf::FloatRect(0.f, 0.f, window.getSize().x * multiplier, window.getSize().y * multiplier));
	window.setView(view);
}

int main(int /*argc*/, char** /*argv*/)
{
#ifdef RMLUI_PLATFORM_WIN32
	AllocConsole();
#endif

	int window_width = 1024;
	int window_height = 768;

	sf::RenderWindow MyWindow(sf::VideoMode(window_width, window_height), "RmlUi with SFML2");
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
	Rml::String root = Shell::FindSamplesRoot();
	ShellFileInterface FileInterface(root);

	if (!MyWindow.isOpen())
		return 1;

	sf::View view(sf::FloatRect(0.f, 0.f, (float)MyWindow.getSize().x, (float)MyWindow.getSize().y));
	MyWindow.setView(view);

	Renderer.SetWindow(&MyWindow);

	Rml::SetFileInterface(&FileInterface);
	Rml::SetRenderInterface(&Renderer);
	Rml::SetSystemInterface(&SystemInterface);


	if (!Rml::Initialise())
		return 1;

	struct FontFace {
		Rml::String filename;
		bool fallback_face;
	};
	FontFace font_faces[] = {
		{ "LatoLatin-Regular.ttf",    false },
		{ "LatoLatin-Italic.ttf",     false },
		{ "LatoLatin-Bold.ttf",       false },
		{ "LatoLatin-BoldItalic.ttf", false },
		{ "NotoEmoji-Regular.ttf",    true  },
	};

	for (const FontFace& face : font_faces)
	{
		Rml::LoadFontFace("assets/" + face.filename, face.fallback_face);
	}
	
	Rml::Context* Context = Rml::CreateContext("default",
		Rml::Vector2i(MyWindow.getSize().x, MyWindow.getSize().y));

	Rml::Debugger::Initialise(Context);

	Rml::ElementDocument* Document = Context->LoadDocument("assets/demo.rml");

	if (Document)
	{
		Document->Show();
		fprintf(stdout, "\nDocument loaded");
	}
	else
	{
		fprintf(stdout, "\nDocument is nullptr");
	}

	while (MyWindow.isOpen())
	{
		static sf::Event event;

		MyWindow.clear();

		sf::CircleShape circle(50.f);
		circle.setPosition(100.f, 100.f);
		circle.setFillColor(sf::Color::Blue);
		circle.setOutlineColor(sf::Color::Red);
		circle.setOutlineThickness(10.f);

		MyWindow.draw(circle);

		Context->Render();
		MyWindow.display();

		while (MyWindow.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Resized:
				updateView(MyWindow, view);
				break;
			case sf::Event::MouseMoved:
				Context->ProcessMouseMove(event.mouseMove.x, event.mouseMove.y,
					SystemInterface.GetKeyModifiers());
				break;
			case sf::Event::MouseButtonPressed:
				Context->ProcessMouseButtonDown(event.mouseButton.button,
					SystemInterface.GetKeyModifiers());
				break;
			case sf::Event::MouseButtonReleased:
				Context->ProcessMouseButtonUp(event.mouseButton.button,
					SystemInterface.GetKeyModifiers());
				break;
			case sf::Event::MouseWheelMoved:
				Context->ProcessMouseWheel(float(-event.mouseWheel.delta),
					SystemInterface.GetKeyModifiers());
				break;
			case sf::Event::TextEntered:
				if (event.text.unicode > 32)
					Context->ProcessTextInput(Rml::Character(event.text.unicode));
				break;
			case sf::Event::KeyPressed:
				Context->ProcessKeyDown(SystemInterface.TranslateKey(event.key.code),
					SystemInterface.GetKeyModifiers());
				break;
			case sf::Event::KeyReleased:
				switch (event.key.code)
				{
				case sf::Keyboard::Num1:
					multiplier = 2.f;
					updateView(MyWindow, view);
					break;
				case sf::Keyboard::Num2:
					multiplier = 1.f;
					updateView(MyWindow, view);
					break;
				case sf::Keyboard::Num3:
					multiplier = .5f;
					updateView(MyWindow, view);
					break;
				case sf::Keyboard::F8:
					Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
					break;
				case sf::Keyboard::Escape:
					MyWindow.close();
					break;
				default:
					break;
				}

				Context->ProcessKeyUp(SystemInterface.TranslateKey(event.key.code),
					SystemInterface.GetKeyModifiers());
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

	Rml::Shutdown();

	return 0;
}
