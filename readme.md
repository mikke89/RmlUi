# RmlUi - The HTML/CSS User Interface Library Evolved

![RmlUi](https://github.com/mikke89/RmlUiDoc/raw/cc01edd834b003df6c649967bfd552bb0acc3d1e/assets/rmlui.png)

RmlUi - now with added boosters taking control of the rocket, targeting *your* games and applications.

---

[![Chat on Gitter](https://badges.gitter.im/RmlUi/community.svg)](https://gitter.im/RmlUi/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) [![Build Status](https://travis-ci.com/mikke89/RmlUi.svg?branch=master)](https://travis-ci.com/mikke89/RmlUi) [![Build status](https://ci.appveyor.com/api/projects/status/x95oi8mrb001pqhh/branch/master?svg=true)](https://ci.appveyor.com/project/mikke89/rmlui/branch/master)

RmlUi is the C++ user interface package based on the HTML and CSS standards, designed as a complete solution for any project's interface needs. It is a fork of the [libRocket](https://github.com/libRocket/libRocket) project, introducing new features, bug fixes, and performance improvements. 

RmlUi uses the time-tested open standards XHTML1.0 and CSS2.0 while borrowing features from HTML5 and CSS3, and extends them with features suited towards real-time applications. Because of this, you don't have to learn a whole new proprietary technology like other libraries in this space.

Documentation is located at https://mikke89.github.io/RmlUiDoc/

## Features

- Cross platform architecture: Windows, macOS, Linux, iOS, etc.
- Dynamic layout system.
- Full animation and transform support.
- Efficient application-wide styling, with a custom-built templating engine.
- Fully featured control set: buttons, sliders, drop-downs, etc.
- Runtime visual debugging suite.
- Easily integrated and extensible with Lua scripting.

## Extensible

- Abstracted interfaces for plugging in to any game engine.
- Decorator engine allowing custom application-specific effects that can be applied to any element.
- Generic event system that binds seamlessly into existing projects.


## RmlUi features

RmlUi introduces several features over the [original libRocket branch](https://github.com/libRocket/libRocket). While some of the new features are briefly documented here, take a look at the [official RmlUi documentation](https://mikke89.github.io/RmlUiDoc/) for more details.


## Transform property

Based on the work of @shoemark, with additional fixes.

Use `perspective`, `perspective-origin`, `transform` and `transform-origin` in RCSS, roughly equivalent to their respective CSS properties.

```CSS
perspective: 1000px;
perspective-origin: 20px 50%;
transform: rotateX(10) skew(-10, 15) translateZ(100px);
transform-origin: left top 0;
```

All transform properties and their argument types are as follows:
```
perspective,  length1
matrix,       abs_numbers6
matrix3d,     abs_numbers16
translateX,   length1
translateY,   length1
translateZ,   length1
translate,    length2
translate3d,  length3
scaleX,       number1
scaleY,       number1
scaleZ,       number1
scale,        number2
scale,        number1
scale3d,      number3
rotateX,      angle1
rotateY,      angle1
rotateZ,      angle1
rotate,       angle1
rotate3d,     length3angle1
skewX,        angle1
skewY,        angle1
skew,         angle2
```

Angles are in degrees by default.





## Animations


Most RCSS properties can be animated, this includes properties representing lengths, colors, or transforms. From C++, an animation can be started on an Element by calling

```C++
bool Element::Animate(const String& property_name, const Property& target_value, float duration, Tween tween = Tween{}, int num_iterations = 1, bool alternate_direction = true, float delay = 0.0f, const Property* start_value = nullptr);
```

Additional animation keys can be added, extending the duration of the animation, by calling

```C++
bool Element::AddAnimationKey(const String& property_name, const Property& target_value, float duration, Tween tween = Tween{});
```

C++ example usage:

```C++
auto p1 = Transform::MakeProperty({ Transforms::Rotate2D{10.f}, Transforms::TranslateX{100.f} });
auto p2 = Transform::MakeProperty({ Transforms::Scale2D{3.f} });
el->Animate("transform", p1, 1.8f, Tween{ Tween::Elastic, Tween::InOut }, -1, true);
el->AddAnimationKey("transform", p2, 1.3f, Tween{ Tween::Elastic, Tween::InOut });
```


Animations can also be specified entirely in RCSS, with keyframes.
```CSS
animation: <duration> <delay> <tweening-function> <num_iterations|infinite> <alternate> <paused> <keyframes-name>;
```
All values, except `<duration>` and `<kyframes-name>`, are optional. Delay must be specified after duration, otherwise values can be given in any order. Keyframes are specified as in CSS, see example below. Multiple animations can be specified on the same element by using a comma-separated list.

Tweening functions (or in CSS lingo, `animation-timing-function`s) specify how the animated value progresses during the animation cycle. A tweening function in RCSS is specified as `<name>-in`, `<name>-out`, or `<name>-in-out`, with one of the following names,
```
back
bounce
circular
cubic
elastic
exponential
linear
quadratic
quartic
quintic
sine
```

RCSS example usage:

```CSS
@keyframes my-progress-bar
{
	0%, 30% {
		background-color: #d99;
	}
	50% {
		background-color: #9d9;
	}
	to { 
		background-color: #f9f;
		width: 100%;
	}
}
#my_element
{
	width: 25px;
	animation: 2s cubic-in-out infinite alternate my-progress-bar;
}
```

Internally, animations apply their properties on the local style of the element. Thus, mixing RML style attributes and animations should be avoided on the same element.

Animations currently support full interpolation of transforms, largely following the CSS specifications. Additionally, interpolation is supported for colors, numbers, lengths, and percentages.

Animations are very powerful coupled with transforms. See the animation sample project for more examples and details. There are also some [video demonstrations](https://mikke89.github.io/RmlUiDoc/pages/rmlui_features.html) of these features in the documentation.


## Transitions

Transitions apply an animation between two property values on an element when its property changes. Transitions are implemented in RCSS similar to how they operate in CSS. However, in RCSS, they only apply when a class or pseudo-class is added to or removed from an element.

```CSS
transition: <space-separated-list-of-properties|all|none> <duration> <delay> <tweening-function>;
```
The property list specifies the properties to be animated. Delay and tweening-function are optional. Delay must be specified after duration, otherwise values can be given in any order. Multiple transitions can be specified on the same element by using a comma-separated list. The tweening function is specified as in the `animation` RCSS property.


Example usage:

```CSS
#transition_test {
	transition: padding-left background-color transform 1.6s elastic-out;
	transform: scale(1.0);
	background-color: #c66;
}
#transition_test:hover {
	padding-left: 60px;
	transform: scale(1.5);
	background-color: #ddb700;
} 
```

See the animation sample project for more examples and details.


## Density-independent pixel (dp)

The `dp` unit behaves like `px` except that its size can be set globally to scale relative to pixels. This makes it easy to achieve a scalable user interface. Set the ratio globally on the context by calling:

```C++
float dp_ratio = 1.5f;
context->SetDensityIndependentPixelRatio(dp_ratio);
```

Usage example in RCSS:
```CSS
div#header 
{
	width: 800dp;
	height: 50dp;
	font-size: 20dp;
}
```


## Pointer events

Set the element property to disregard mouse input events on this and descending elements.
```CSS
pointer-events: none;
```
Default is `auto`.


## Image-color property

Non-standard RCSS property which multiplies a color with images in `<img>` tags and image decorators. Useful for `:hover`-events and for applying transparency.
```CSS
image-color: rgba(255, 160, 160, 200);
icon-decorator: image;
icon-image: background.png 34px 0px 66px 28px;
```


## Inline-block

Unlike the original branch, elements with
```CSS
display: inline-block;
```
will shrink to the width of their content, like in CSS.



## Border shorthand

Enables the `border` property shorthand.
```CSS
border: 4px #e99;
```


## Various changes

- The namespace has changed from `Rocket` to `Rml`, and include path from `<Rocket/...>` to `<RmlUi/...>`.
- The slider on the `input.range` element can be dragged from anywhere in the element.
- The `:checked` pseudo class can be used to style the selected item in drop-down lists.


## License (MIT)
 
 Copyright (c) 2019 The RmlUi Team, and contributors\
 Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors
 
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
