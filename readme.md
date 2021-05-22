# RmlUi - The HTML/CSS User Interface Library Evolved

![RmlUi](https://github.com/mikke89/RmlUiDoc/raw/cc01edd834b003df6c649967bfd552bb0acc3d1e/assets/rmlui.png)

RmlUi - now with added boosters taking control of the rocket, targeting *your* games and applications.

---

[![Chat on Gitter](https://badges.gitter.im/RmlUi/community.svg)](https://gitter.im/RmlUi/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) ![Build](https://github.com/mikke89/RmlUi/workflows/Build/badge.svg) [![Build status](https://ci.appveyor.com/api/projects/status/x95oi8mrb001pqhh/branch/master?svg=true)](https://ci.appveyor.com/project/mikke89/rmlui/branch/master) [![Build Status](https://travis-ci.com/mikke89/RmlUi.svg?branch=master)](https://travis-ci.com/mikke89/RmlUi)

RmlUi is the C++ user interface package based on the HTML and CSS standards, designed as a complete solution for any project's interface needs. It is a fork of the [libRocket](https://github.com/libRocket/libRocket) project, introducing new features, bug fixes, and performance improvements. 

RmlUi aims at being a light-weight and performant library with its own layouting engine and few dependencies. In essence, RmlUi takes your HTML/CSS-like source files and turns them into vertices, indices and draw commands, and then you bring your own renderer to draw them. And of course there is full access to the element hierarchy/DOM, event handling, and all the interactivity and customizability you would expect. All of this directly from C++, or optionally from scripting languages using plugins. The core library compiles down to fractions of the size it takes to integrate a fully fledged web browser as other libraries do in this space. 

RmlUi is based around the XHTML1 and CSS2 standards while borrowing features from HTML5 and CSS3, and extends them with features suited towards real-time applications. Take a look at the [conformance](#conformance) and [enhancements](#enhancements) sections below for details.

Documentation is located at https://mikke89.github.io/RmlUiDoc/

## Features

- Cross platform architecture: Windows, macOS, Linux, iOS, etc.
- Dynamic layout system.
- Full animation and transform support.
- Efficient application-wide styling, with a custom-built templating engine.
- Fully featured control set: buttons, sliders, drop-downs, etc.
- Runtime visual debugging suite.

## Extensible

- Abstracted interfaces for plugging in to any game engine.
- Decorator engine allowing custom application-specific effects that can be applied to any element.
- Generic event system that binds seamlessly into existing projects.
- Easily integrated and extensible with Lua scripting.

## Controllable

- The user controls their own update loop, calling into RmlUi as desired.
- The library strictly runs as a result of calls to its API, never in the background.
- Input handling and rendering is performed by the user.
- The library generates vertices, indices, and textures for the user to render how they like.
- File handling and the font engine can optionally be fully replaced by the user.


## Integrating RmlUi

Here are the general steps to integrate the library into a C++ application, have a look at the [documentation](https://mikke89.github.io/RmlUiDoc/) for details.

1. Build RmlUi using CMake and your favorite compiler, fetch the Windows library binaries, or consume [the library using Conan](https://conan.io/center/rmlui).
2. Link it up to your application.
3. Implement the abstract [system interface](Include/RmlUi/Core/SystemInterface.h) and [render interface](Include/RmlUi/Core/RenderInterface.h).
4. Initialize RmlUi with the interfaces, create a context, provide font files, and load a document.
5. Call into the context's update and render methods in a loop, and submit input events.
6. Compile and run!

Several [samples](Samples/) demonstrate everything from basic integration to more complex use of the library, feel free to have a look for inspiration.

## Dependencies

- [FreeType](https://www.freetype.org/). However, it can be fully replaced by a custom [font engine](Include/RmlUi/Core/FontEngineInterface.h).
- The standard library.

In addition, a C++14 compatible compiler is required.

## Conformance

RmlUi aims to support the most common and familiar features from HTML and CSS, while keeping the library light and performant. We do not aim to be fully compliant with CSS or HTML, in particular when it conflicts with lightness and performance. Users are generally expected to author documents specifically for RmlUi, but any experience and skills from web design should be transferable.

RmlUi supports most of CSS2 with some CSS3 features such as

- Animations and transitions
- Transforms (with full interpolation support)
- Media queries
- Border radius

and many of the common HTML elements including `<input>`,  `<textarea>`, and `<select>`.

For details, see
- [RCSS Property index](https://mikke89.github.io/RmlUiDoc/pages/rcss/property_index.html) for all supported properties and differences from CSS.
- [RML Element index](https://mikke89.github.io/RmlUiDoc/pages/rml/element_index.html) for all supported elements.

## Enhancements

RmlUi adds features and enhancements over CSS and HTML where it makes sense, most notably the following.

- [Data binding (model-view-controller)](https://mikke89.github.io/RmlUiDoc/pages/data_bindings.html). Synchronization between application data and user interface.
- [Decorators](https://mikke89.github.io/RmlUiDoc/pages/rcss/decorators.html). Full control over the styling of [all elements](https://mikke89.github.io/RmlUiDoc/pages/style_guide.html).
- [Sprite sheets](https://mikke89.github.io/RmlUiDoc/pages/rcss/sprite_sheets.html). Define and use sprites with easy high DPI support.
- [Templates](https://mikke89.github.io/RmlUiDoc/pages/rml/templates.html). Making windows look consistent.
- [Localization](https://mikke89.github.io/RmlUiDoc/pages/localisation.html). Translate any text in the document.


## Example document

This example demonstrates a basic document with [data bindings](https://mikke89.github.io/RmlUiDoc/pages/data_bindings.html), which is loaded and displayed using the C++ code below.

#### Document

`hello_world.rml`

```html
<rml>
<head>
	<title>Hello world</title>
	<link type="text/rcss" href="rml.rcss"/>
	<link type="text/rcss" href="window.rcss"/>
</head>
<body data-model="animals">
	<h1>RmlUi</h1>
	<p>Hello <span id="world">world</span>!</p>
	<p data-if="show_text">The quick brown fox jumps over the lazy {{animal}}.</p>
	<input type="text" data-value="animal"/>
</body>
</rml>
```
The `{{animal}}` text and the `data-if`, `data-value` attributes represent data bindings and will synchronize with the application data.

#### Style sheet

`window.rcss`

```css
body {
	font-family: LatoLatin;
	font-size: 18px;
	color: #02475e;
	background: #fefecc;
	text-align: center;
	padding: 2em 1em;
	position: absolute;
	border: 2px #ccc;
	width: 500px;
	height: 200px;
	margin: auto;
}
		
h1 {
	color: #f6470a;
	font-size: 1.5em;
	font-weight: bold;
	margin-bottom: 0.7em;
}
		
p { 
	margin: 0.7em 0;
}
		
input.text {
	background-color: #fff;
	color: #555;
	border: 2px #999;
	padding: 5px;
	tab-index: auto;
	cursor: text;
	box-sizing: border-box;
	width: 200px;
	font-size: 0.9em;
}
```

RmlUi defines no styles internally, thus the `input` element is styled here, and [`rml.rcss` is included](Samples/assets/rml.rcss) for proper layout of common tags.


#### C++ Initialization and loop

`main.cpp`

```cpp
#include <RmlUi/Core.h>

class MyRenderInterface : public Rml::RenderInterface
{
	// RmlUi sends vertices, indices and draw commands through this interface for your
	// application to render how you'd like.
	/* ... */
}

class MySystemInterface : public Rml::SystemInterface
{	
	// RmlUi requests the current time and provides various utilities through this interface.
	/* ... */
}

struct ApplicationData {
	bool show_text = true;
	Rml::String animal = "dog";
} my_data;

int main(int argc, char** argv)
{
	// Initialize the window and graphics API being used, along with your game or application.

	/* ... */

	MyRenderInterface render_interface;
	MySystemInterface system_interface;

	// Install the custom interfaces.
	Rml::SetRenderInterface(&render_interface);
	Rml::SetSystemInterface(&system_interface);

	// Now we can initialize RmlUi.
	Rml::Initialise();

	// Create a context to display documents within.
	Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));

	// Tell RmlUi to load the given fonts.
	Rml::LoadFontFace("LatoLatin-Regular.ttf");
	// Fonts can be registered as fallback fonts, as in this case to display emojis.
	Rml::LoadFontFace("NotoEmoji-Regular.ttf", true);

	// Set up data bindings to synchronize application data.
	if (Rml::DataModelConstructor constructor = context->CreateDataModel("animals"))
	{
		constructor.Bind("show_text", &my_data.show_text);
		constructor.Bind("animal", &my_data.animal);
	}

	// Now we are ready to load our document.
	Rml::ElementDocument* document = context->LoadDocument("hello_world.rml");
	document->Show();

	// Replace and style some text in the loaded document.
	Rml::Element* element = document->GetElementById("world");
	element->SetInnerRML(reinterpret_cast<const char*>(u8"🌍"));
	element->SetProperty("font-size", "1.5em");

	bool exit_application = false;
	while (!exit_application)
	{
		// We assume here that we have some way of updating and retrieving inputs internally.
		if (my_input->KeyPressed(KEY_ESC))
			exit_application = true;

		// Submit input events such as MouseMove and key events (not shown) to the context.
		if (my_input->MouseMoved())
			context->ProcessMouseMove(mouse_pos.x, mouse_pos.y, 0);

		// Update the context to reflect any changes resulting from input events, animations,
		// modified and added elements, or changed data in data bindings.
		context->Update();

		// Render the user interface. All geometry and other rendering commands are now
		// submitted through the render interface.
		context->Render();
	}

	Rml::Shutdown();

	return 0;
}
```

#### Rendered output

![Hello world document](Samples/assets/hello_world.png)

Users can now edit the text field to change the animal. The data bindings ensure that both the document text as well as the application string `my_data.animal` are automatically modified accordingly.


## Gallery

![Game interface](https://github.com/mikke89/RmlUiDoc/blob/3f319d8464e73b821179ff8d20537013af5b9810/assets/gallery/invader.png)
**Game interface from the 'invader' sample**

![Game menu](https://github.com/mikke89/RmlUiDoc/blob/3f319d8464e73b821179ff8d20537013af5b9810/assets/gallery/menu_screen.png)
**Game menu**

![Form controls](https://github.com/mikke89/RmlUiDoc/blob/3f319d8464e73b821179ff8d20537013af5b9810/assets/gallery/forms.png)
**Form controls from the 'demo' sample**

![Sandbox](https://github.com/mikke89/RmlUiDoc/blob/3f319d8464e73b821179ff8d20537013af5b9810/assets/gallery/sandbox.png)
**Sandbox from the 'demo' sample, try it yourself!**

![Transition](https://github.com/mikke89/RmlUiDoc/blob/3f319d8464e73b821179ff8d20537013af5b9810/assets/gallery/transition.gif)  
**Transitions on mouse hover (entirely in RCSS)**

![Transform](https://github.com/mikke89/RmlUiDoc/blob/3f319d8464e73b821179ff8d20537013af5b9810/assets/gallery/transform.gif)  
**Animated transforms (entirely in RCSS)**

![Lottie animation](https://github.com/mikke89/RmlUiDoc/blob/086385e119f0fc6e196229b785e91ee0252fe4b4/assets/gallery/lottie.gif)  
**Vector animations with the [Lottie plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/lottie.html)**

![SVG image](https://github.com/mikke89/RmlUiDoc/blob/2908fe50acf7861e729ce113eafa8cf7610bf08a/assets/gallery/svg_plugin.png)  
**Vector images with the [SVG plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/svg.html)**

See the **[full gallery](https://mikke89.github.io/RmlUiDoc/pages/gallery.html)** for more screenshots and videos of the library in action.


## License

RmlUi is published under the [MIT license](LICENSE.txt). The library includes third-party source code and assets with their own licenses, as detailed below.

#### RmlUi source code and assets

MIT License

Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors\
Copyright (c) 2019-2021 The RmlUi Team, and contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

#### Third-party source code included in RmlUi Core

See [Include/RmlUi/Core/Containers/LICENSE.txt](Include/RmlUi/Core/Containers/LICENSE.txt) - all MIT licensed.

#### Third-party font assets included in RmlUi Debugger

See [Source/Debugger/LICENSE.txt](Source/Debugger/LICENSE.txt) - SIL Open Font License.

#### Additional sample assets *(in Samples/)*

See
- [Samples/assets/LICENSE.txt](Samples/assets/LICENSE.txt)
- [Samples/basic/bitmapfont/data/LICENSE.txt](Samples/basic/bitmapfont/data/LICENSE.txt)
- [Samples/basic/lottie/data/LICENSE.txt](Samples/basic/lottie/data/LICENSE.txt)
- [Samples/basic/svg/data/LICENSE.txt](Samples/basic/svg/data/LICENSE.txt)

#### Libraries included with the test suite *(in Tests/Dependencies/)*

See [Tests/Dependencies/LICENSE.txt](Tests/Dependencies/LICENSE.txt).

#### Additional test suite assets *(in Tests/Data/VisualTests/)*

See [Tests/Data/VisualTests/LICENSE.txt](Tests/Data/VisualTests/LICENSE.txt).
