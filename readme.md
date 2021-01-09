# RmlUi - The HTML/CSS User Interface Library Evolved

![RmlUi](https://github.com/mikke89/RmlUiDoc/raw/cc01edd834b003df6c649967bfd552bb0acc3d1e/assets/rmlui.png)

RmlUi - now with added boosters taking control of the rocket, targeting *your* games and applications.

---

[![Chat on Gitter](https://badges.gitter.im/RmlUi/community.svg)](https://gitter.im/RmlUi/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) ![Build](https://github.com/mikke89/RmlUi/workflows/Build/badge.svg) [![Build status](https://ci.appveyor.com/api/projects/status/x95oi8mrb001pqhh/branch/master?svg=true)](https://ci.appveyor.com/project/mikke89/rmlui/branch/master) [![Build Status](https://travis-ci.com/mikke89/RmlUi.svg?branch=master)](https://travis-ci.com/mikke89/RmlUi)

RmlUi is the C++ user interface package based on the HTML and CSS standards, designed as a complete solution for any project's interface needs. It is a fork of the [libRocket](https://github.com/libRocket/libRocket) project, introducing new features, bug fixes, and performance improvements. 

RmlUi uses the time-tested open standards XHTML1 and CSS2 while borrowing features from HTML5 and CSS3, and extends them with features suited towards real-time applications. Because of this, you don't have to learn a whole new proprietary technology like other libraries in this space. Please have a look at the supported [RCSS properties](https://mikke89.github.io/RmlUiDoc/pages/rcss/property_index.html) and [RML elements](https://mikke89.github.io/RmlUiDoc/pages/rml/element_index.html).

Documentation is located at https://mikke89.github.io/RmlUiDoc/

***Note:*** RmlUi 4.0 currently in development is a restructuring of RmlUi 3.x. This includes changes to the namespaces, plugins, and include headers. Take a look at the [changelog](changelog.md#restructuring-rmlui) for a summary of changes and an upgrade guide.

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

1. Build RmlUi using CMake and your favorite compiler, or fetch the Windows library binaries.
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


## Example: Basic document

In this example a document is created using a templated window. The template is optional but can aid in achieving a consistent look by sharing it between multiple documents.

#### Document

`hello_world.rml`

```html
<rml>
<head>
	<title>Hello world</title>
	<link type="text/template" href="window.rml" />
	<style>
		body
		{
			width: 200px;
			height: 100px;
			margin: auto;
		}
	</style>
</head>
<body template="window">
	Hello world!
</body>
</rml>
```

#### Window template

`window.rml`

```html
<template name="window" content="content">
<head>
	<link type="text/rcss" href="rml.rcss"/>
	<link type="text/rcss" href="window.rcss"/>
</head>
<body>
	<div id="title_header">RmlUi</div>
	<div id="content"/>
</body>
</template>
```
No styles are defined internally, thus `rml.rcss` can be included for [styling the standard elements](Samples/assets/rml.rcss).

#### Style sheet

`window.rcss`

```css
body
{
	font-family: Delicious;
	font-weight: normal;
	font-style: normal;
	font-size: 15px;
	color: #6f42c1;
	background: #f6f8fa;
	text-align: center;
	padding: 2em 3em;
	border: 2px #ccc;
}

#title_header
{
	color: #9a42c5;
	font-size: 1.5em;
	font-weight: bold;
	padding-bottom: 1em;
}
```

#### Rendered output

![Hello world document](Samples/assets/hello_world.png)



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
- [Samples/basic/lottie/data/LICENSE.txt](Samples/basic/lottie/data/LICENSE.txt)
- [Samples/basic/bitmapfont/data/LICENSE.txt](Samples/basic/bitmapfont/data/LICENSE.txt)

#### Libraries included with the test suite *(in Tests/Dependencies/)*

See [Tests/Dependencies/LICENSE.txt](Tests/Dependencies/LICENSE.txt).

#### Additional test suite assets *(in Tests/Data/VisualTests/)*

See [Tests/Data/VisualTests/LICENSE.txt](Tests/Data/VisualTests/LICENSE.txt).
