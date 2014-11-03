/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 Nuno Silva
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
#include "SystemInterfaceSFML.h"
#include "RenderInterfaceSFML.h"
#include <Rocket/Core/Input.h>
#include <Rocket/Debugger/Debugger.h>
#include "ShellFileInterface.h"

int main(int argc, char **argv)
{
#ifdef ROCKET_PLATFORM_WIN32
        DoAllocConsole();
#endif

        int window_width = 1024;
        int window_height = 768;

	sf::RenderWindow MyWindow(sf::VideoMode(window_width, window_height), "libRocket with SFML");

	RocketSFMLRenderer Renderer;
	RocketSFMLSystemInterface SystemInterface;
	ShellFileInterface FileInterface("../Samples/assets/");

	if(!MyWindow.IsOpened())
		return 1;

	Renderer.SetWindow(&MyWindow);

	Rocket::Core::SetFileInterface(&FileInterface);
	Rocket::Core::SetRenderInterface(&Renderer);
	Rocket::Core::SetSystemInterface(&SystemInterface);

	if(!Rocket::Core::Initialise())
		return 1;

	Rocket::Core::FontDatabase::LoadFontFace("Delicious-Bold.otf");
	Rocket::Core::FontDatabase::LoadFontFace("Delicious-BoldItalic.otf");
	Rocket::Core::FontDatabase::LoadFontFace("Delicious-Italic.otf");
	Rocket::Core::FontDatabase::LoadFontFace("Delicious-Roman.otf");

	Rocket::Core::Context *Context = Rocket::Core::CreateContext("default",
		Rocket::Core::Vector2i(MyWindow.GetWidth(), MyWindow.GetHeight()));

	Rocket::Debugger::Initialise(Context);

	Rocket::Core::ElementDocument *Document = Context->LoadDocument("demo.rml");

	if(Document)
	{
		Document->Show();
		Document->RemoveReference();
	};

	while(MyWindow.IsOpened())
	{
		static sf::Event event;

		MyWindow.Clear();
		Context->Render();
		MyWindow.Display();

		while(MyWindow.GetEvent(event))
		{
			switch(event.Type)
			{
			case sf::Event::Resized:
				Renderer.Resize();
				break;
			case sf::Event::MouseMoved:
				Context->ProcessMouseMove(event.MouseMove.X, event.MouseMove.Y,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::MouseButtonPressed:
				Context->ProcessMouseButtonDown(event.MouseButton.Button,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::MouseButtonReleased:
				Context->ProcessMouseButtonUp(event.MouseButton.Button,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::MouseWheelMoved:
				Context->ProcessMouseWheel(event.MouseWheel.Delta,
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::TextEntered:
				if (event.Text.Unicode > 32)
					Context->ProcessTextInput(event.Text.Unicode);
				break;
			case sf::Event::KeyPressed:
				Context->ProcessKeyDown(SystemInterface.TranslateKey(event.Key.Code),
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::KeyReleased:
				if(event.Key.Code == sf::Key::F8)
				{
					Rocket::Debugger::SetVisible(!Rocket::Debugger::IsVisible());
				};

				Context->ProcessKeyUp(SystemInterface.TranslateKey(event.Key.Code),
					SystemInterface.GetKeyModifiers(&MyWindow));
				break;
			case sf::Event::Closed:
				return 1;
				break;
			};
		};

		Context->Update();
	};

	return 0;
};
