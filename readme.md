# RmlUi - The HTML/CSS User Interface Library Evolved

![RmlUi](https://github.com/mikke89/RmlUiDoc/raw/cc01edd834b003df6c649967bfd552bb0acc3d1e/assets/rmlui.png)

RmlUi - now with added boosters taking control of the rocket, targeting *your* games and applications.

---

[![Build Status](https://travis-ci.com/mikke89/RmlUi.svg?branch=performance)](https://travis-ci.com/mikke89/RmlUi) [![Build status](https://ci.appveyor.com/api/projects/status/x95oi8mrb001pqhh/branch/performance?svg=true)](https://ci.appveyor.com/project/mikke89/rmlui/branch/performance)

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


## RmlUi 3.0 features

### Performance

The main effort in RmlUi 3.0 has been on improving the performance of the library. Users should see a substantial performance increase when upgrading.

- The update loop has been reworked to avoid doing unnecessary, repeated calculations whenever the document or style is changed. Instead of immediately updating properties on any affected elements, most of this work is done during the Context::Update call in a more carefully chosen order. Note that for this reason, when querying the Rocket API for properties such as size or position, this information may not be up-to-date with changes since the last Context::Update, such as newly added elements or classes. If this information is needed immediately, a call to ElementDocument::UpdateDocument can be made before such queries at a performance penalty.
- Several containers have been replaced, such as std::map to [robin_hood::unordered_flat_map](https://github.com/martinus/robin-hood-hashing).
- Reduced number of allocations and unnecessary recursive calls.
- And many more, smaller optimizations, resulting in more than 5x performance increase for creation and destruction of a large number of elements. A benchmark for this is located in the animation sample for now.


### Sprite sheets

The RCSS at-rule `@spritesheet` can be used to declare a sprite sheet. A sprite sheet consists of a single image and multiple sprites each specifying a region of the image. Sprites can in turn be used in decorators.

A sprite sheet can be declared in RCSS as in the following example.
```CSS
@spritesheet theme 
{
	src: invader.tga;
	
	title-bar-l: 147px 0px 82px 85px;
	title-bar-c: 229px 0px  1px 85px;
	title-bar-r: 231px 0px 15px 85px;
	
	icon-invader: 179px 152px 51px 39px;
	icon-game:    230px 152px 51px 39px;
	icon-score:   434px 152px 51px 39px;
	icon-help:    128px 152px 51px 39px;
}
```
The first property `src` provides the filename of the image for the sprite sheet. Every other property specifies a sprite as `<name>: <rectangle>`. A sprite's name applies globally to the style sheet, and must be unique. A rectangle is declared as `x y width height`, each of which must be in `px` units. Here, `x` and `y` refers to the position in the image with the origin placed at the top-left corner, and `width` and `height` extends the rectangle right and down.

The sprite name can be used in decorators, such as:
```CSS
decorator: tiled-horizontal( title-bar-l, title-bar-c, title-bar-r );
```
This creates a tiled decorator where the `title-bar-l` and `title-bar-r` sprites occupies the left and right parts of the element at their native size, while `title-bar-c` occupies the center and is stretched horizontally as the element is stretched.


### Decorators

The new RCSS `decorator` property replaces the old decorator declarations in libRocket. A decorator is declared by the name of the decorator type and its properties in parenthesis. Some examples follow.

```CSS
/* declares an image decorater by a sprite name */
decorator: image( icon-invader );

/* declares a tiled-box decorater by several sprites */
decorator: tiled-box(
	window-tl, window-t, window-tr, 
	window-l, window-c, window-r,
	window-bl, window-b, window-br
);

 /* declares an image decorator by the url of an image, and a rectangle inside it */
decorator: image( invader.tga 5px 10px 30px 30px );
```

The `decorator` property follows the normal cascading rules, is non-inherited, and has the default value `none` which specifies no decorator on the element. Unlike in libRocket, decorators can now be set on the element's style, although we recommend declaring them in style sheets for performance reasons.

Furthermore, multiple decorators can be specified on any element by a comma-separated list of decorators.
```CSS
/* declares two decorators on the same element, the first will be rendered on top of the latter */
decorator: image( icon-invader ), tiled-horizontal( title-bar-l, title-bar-c, title-bar-r );
```

When creating a custom decorator, you can provide a shorthand property named `decorator` which will be used to parse the text inside the parenthesis of the property declaration. This allows specifying the decorator with inline properties as in the above examples.


### Decorator at-rule

Note: This part is experimental. If it turns out there are very few use-cases for this feature, it may be removed in the future. Feedback is welcome.

The `@decorator` at-rule in RCSS can be used to declare a decorator when the shorthand syntax given above is not sufficient. It is best served with an example, we use the custom `starfield` decorator type from the invaders sample. In the style sheet, we can populate it with properties as follows.

```CSS
@decorator stars : starfield {
	num-layers: 5;
	top-colour: #fffc;
	bottom-colour: #fff3;
	top-speed: 80.0;
	bottom-speed: 20.0;
	top-density: 8;
	bottom-density: 20;
}
```
And then use it in a decorator.
```CSS
decorator: stars;
```
Note the lack of parenthesis which means it is a decorator name and not a type with shorthand properties declared.



### Other changes

- `Context::ProcessMouseWheel` now takes a float value for the `wheel_delta` property, thereby enabling continuous/smooth scrolling for input devices with such support. The default scroll length for unity value of `wheel_delta` is now three times the default line-height multiplied by the current dp-ratio.
- The system interface now has two new functions for setting and getting text to and from the clipboard: `virtual void SystemInterface::SetClipboardText(const Core::WString& text)` and `virtual void SystemInterface::GetClipboardText(Core::WString& text)`.
- The debugger now has the ability to clear the log. Additionally, the displayed element information updates when the element changes.


### Breaking changes

Breaking changes since RmlUi v2.0.

- Rml::Core::String has been replaced by std::string, thus, interfacing with the library now requires you to change your string types. This change was motivated by a small performance gain, additionally, it should make it easier to interface with the library especially for users already using std::string in their codebase. Similarly, Rml::Core::WString is now an alias for std::wstring.
- Querying the property of an element for size, position and similar may not work as expected right after changes to the document or style. This change is made for performance reasons, see the description under *performance* for reasoning and a workaround.
- The Controls::DataGrid "min-rows" property has been replaced by an attribute of the same name.
- Removed RenderInterface::GetPixelsPerInch, instead the pixels per inch value has been fixed to 96 PPI, as per CSS specs. To achieve a scalable user interface, instead use the 'dp' unit.
- Removed 'top' and 'bottom' from z-index property.
- See changes to the declaration of decorators above.


#### Events

There are some changes to events in RmlUi, however, for most users, existing code should still work as before.

There is now a distinction between actions executed in event listeners, and default actions for events:

- Event listeners are attached to an element as before. Events follow the normal phases: capture (root -> target), target, and bubble (target -> root). Each event listener can be either attached to the bubble phase (default) or capture phase. The target phase always executes if reached. Listeners are executed in the order they are added to the element. Each event type specifies whether it executes the bubble phase or not, see below for details.
- Default actions are primarily for actions performed internally in the library. They are executed in the function `virtual void Element::ProcessDefaultAction(Event& event)`. However, any object that derives from `Element` can override the default behavior and add new behavior. The default actions follow the normal event phases, but are only executed in the phase according to their `default_action_phase` which is defined for each event type. If an event is cancelled with `Event::StopPropagation()`, then the default action is not performed unless already executed.


Each event type now has an associated EventId as well as a specification defined as follows:

- `interruptible`: Whether the event can be cancelled by calling `Event::StopPropagation()`.
- `bubbles`: Whether the event executes the bubble phase. If true, all three phases: capture, target, and bubble, are executed. If false, only capture and target phases are executed.
- `default_action_phase`: One of: None, Target, Bubble, TargetAndBubble. Specifies during which phases the default action is executed, if any. That is, the phase for which `Element::ProcessDefaultAction` is called. See above for details.

For example, the event type `click` has the following specification:
```
id: EventId::Click
type: "click"
interruptible: true
bubbles: true
default_action_phase: TargetAndBubble
```

Whenever an event listener is added or event is dispatched, and the provided event type does not already have a specification, the default specification
`interruptible: true, bubbles: true, default_action_phase: None` is added for that event type. To provide a custom specification for a new event, first call the method:
```
EventId Rml::Core::RegisterEventType(const String& type, bool interruptible, bool bubbles, DefaultActionPhase default_action_phase)
```
After this call, any usage of this type will use the provided specification by default. The returned EventId can be used to dispatch events instead of the type string.

Various changes:
- All event listeners on the current element will always be called after calling StopPropagation(). However, the default action on the current element will be prevented. When propagating to the next element, the event is stopped. This behavior is consistent with the standard DOM events model.
- `Element::DispatchEvent` can now optionally take an `EventId` instead of a `String`.
- The `resize` event now only applies to the document size, not individual elements.
- The `scrollchange` event has been replaced by a function call. To capture scroll changes, instead use the `scroll` event.



## RmlUi 2.0 features

RmlUi introduces several features over the [original libRocket branch](https://github.com/libRocket/libRocket). While the [official RmlUi documentation](https://mikke89.github.io/RmlUiDoc/) is being updated with new documentation, some of the new features are also briefly documented here. Pull requests are welcome for improving the documentation at the [RmlUi documentation repository](https://github.com/mikke89/RmlUiDoc).

### Transform property

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





### Animations


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


### Transitions

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


### Density-independent pixel (dp)

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


### Pointer events

Set the element property to disregard mouse input events on this and descending elements.
```CSS
pointer-events: none;
```
Default is `auto`.


### Image-color property

Non-standard RCSS property which multiplies a color with images in `<img>` tags and image decorators. Useful for `:hover`-events and for applying transparency.
```CSS
image-color: rgba(255, 160, 160, 200);
icon-decorator: image;
icon-image: background.png 34px 0px 66px 28px;
```


### Inline-block

Unlike the original branch, elements with
```CSS
display: inline-block;
```
will shrink to the width of their content, like in CSS.



### Border shorthand

Enables the `border` property shorthand.
```CSS
border: 4px #e99;
```


### Various changes

The slider on the `input.range` element can be dragged from anywhere in the element. Additionally, the `:checked` pseudo class can be used to style the selected item in drop-down lists.


## License (MIT)
 
 Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors
 Copyright (c) 2019 The RmlUi Team, and contributors\
 
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
 