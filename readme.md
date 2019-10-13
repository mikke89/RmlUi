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


## RmlUi 3.0 features

### Performance

The main effort in RmlUi 3.0 has been on improving the performance of the library. Users should see a substantial performance increase when upgrading.

- The update loop has been reworked to avoid doing unnecessary, repeated calculations whenever the document or style is changed. Instead of immediately updating properties on any affected elements, most of this work is done during the Context::Update call in a more carefully chosen order. Note that for this reason, when querying the Rocket API for properties such as size or position, this information may not be up-to-date with changes since the last Context::Update, such as newly added elements or classes. If this information is needed immediately, a call to ElementDocument::UpdateDocument can be made before such queries at a performance penalty.
- Several containers have been replaced, such as std::map to [robin_hood::unordered_flat_map](https://github.com/martinus/robin-hood-hashing).
- Reduced number of allocations and unnecessary recursive calls.
- Internally, the concept of computed values has been introduced. Computed values take the properties of an element and computes them as far as possible without introducing the layouting.
- And many more, smaller optimizations, resulting in a more than **25x** measured performance increase for creation and destruction of a large number of elements. A benchmark is included with the samples.


### Sprite sheets

The RCSS at-rule `@spritesheet` can be used to declare a sprite sheet. A sprite sheet consists of a single image and multiple sprites each specifying a region of the image. Sprites can in turn be used in decorators.

A sprite sheet can be declared in RCSS as in the following example.
```css
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
The first property `src` provides the filename of the image for the sprite sheet. Every other property specifies a sprite as `<name>: <rectangle>`. A sprite's name applies globally to all included style sheets in a given document, and must be unique. A rectangle is declared as `x y width height`, each of which must be in `px` units. Here, `x` and `y` refers to the position in the image with the origin placed at the top-left corner, and `width` and `height` extends the rectangle right and down.

The sprite name can be used in decorators, such as:
```css
decorator: tiled-horizontal( title-bar-l, title-bar-c, title-bar-r );
```
This creates a tiled decorator where the `title-bar-l` and `title-bar-r` sprites occupies the left and right parts of the element at their native size, while `title-bar-c` occupies the center and is stretched horizontally as the element is stretched.


### Decorators

The new RCSS `decorator` property replaces the old decorator declarations in libRocket. A decorator is declared by the name of the decorator type and its properties in parenthesis. Some examples follow.

```css
/* declares an image decorater by a sprite name */
decorator: image( icon-invader );

/* declares a tiled-box decorater by several sprites */
decorator: tiled-box(
	window-tl, window-t, window-tr, 
	window-l, window-c, window-r,
	window-bl, window-b, window-br
);

 /* declares an image decorator by the url of an image */
decorator: image( invader.tga );
```

The `decorator` property follows the normal cascading rules, is non-inherited, and has the default value `none` which specifies no decorator on the element. The decorator looks for a sprite with the same name first. If none exists, then it treats it as a file name for an image. Decorators can now be set on the element's style, although we recommend declaring them in style sheets for performance reasons.

Furthermore, multiple decorators can be specified on any element by a comma-separated list of decorators.
```css
/* declares two decorators on the same element, the first will be rendered on top of the latter */
decorator: image( icon-invader ), tiled-horizontal( title-bar-l, title-bar-c, title-bar-r );
```

When creating a custom decorator, you can provide a shorthand property named `decorator` which will be used to parse the text inside the parenthesis of the property declaration. This allows specifying the decorator with inline properties as in the above examples.


### Decorator at-rule

Note: This part is experimental. If it turns out there are very few use-cases for this feature, it may be removed in the future. Feedback is welcome.

The `@decorator` at-rule in RCSS can be used to declare a decorator when the shorthand syntax given above is not sufficient. It is best served with an example, we use the custom `starfield` decorator type from the invaders sample. In the style sheet, we can populate it with properties as follows.

```css
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
```css
decorator: stars;
```
Note the lack of parenthesis which means it is a decorator name and not a type with shorthand properties declared.


### Ninepatch decorator

The new `ninepatch` decorator splits a sprite into a 3x3 grid of patches. The corners of the ninepatch are rendered at their native size, while the inner patches are stretched so that the whole element is filled. In a sense, it can be considered a simplified and more performant version of the `tiled-box` decorator.

The decorator is specified by two sprites, defining an outer and inner rectangle:
```css
@spritesheet my-button {
	src: button.png;
	button-outer: 247px  0px 159px 45px;
	button-inner: 259px 19px 135px  1px;
}
```
The inner rectangle defines the parts of the sprite that will be stretched when the element is resized. 

The `ninepatch` decorator is applied as follows:
```css
decorator: ninepatch( button-outer, button-inner );
```
The two sprites must be located in the same sprite sheet. Only sprites are supported by the ninepatch decorator, image urls cannot be used.

Furthermore, the ninepatch decorator can have the rendered size of its edges specified manually.
```css
decorator: ninepatch( button-outer, button-inner, 19px 12px 25px 12px );
```
The edge sizes are specified in the common `top-right-bottom-left` box order. The box shorthands are also available, e.g. a single value will be replicated to all. Percent and numbers can also be used, they will scale relative to the native size of the given edge multiplied by the current dp ratio. Thus, setting
```css
decorator: ninepatch( button-outer, button-inner, 1.0 );
```
is a simple approach to scale the decorators with higher dp ratios. For crisper graphics, increase the sprite sheet's pixel size at the edges and lower the rendered edge size number correspondingly.


### Gradient decorator

A `gradient` decorator has been implemented with support for horizontal and vertical color gradients (thanks to @viciious). Example usage:

```css
decorator: gradient( direction start-color stop-color );

direction: horizontal|vertical;
start-color: #ff00ff;
stop-color: #00ff00;
```


### Tiled decorators orientation

The orientation of each tile in the tiled decorators, `image`, `tiled-horizontal`, `tiled-vertical`, and `tiled-box`, can be rotated and flipped (thanks to @viciious). The new keywords are:
```
none, flip-horizontal, flip-vertical, rotate-180
```

Example usage:

```css
decorator: tiled-horizontal( header-l, header-c, header-l flip-horizontal );
```


### Image decorator fit modes and alignment

The image decorator now supports fit modes and alignment for scaling and positioning the image within its current element.

The full RCSS specification for the `image` decorator is now
```css
decorator: image( <src> <orientation> <fit> <align-x> <align-y> );
```
where
```
  <src>: image source url or sprite name
  <orientation>: none (default) | flip-horizontal | flip-vertical | rotate-180
  <fit>: fill (default) | contain | cover | scale-none | scale-down
  <align-x>: left | center (default) | right | <length-percentage>
  <align-y>: top | center (default) | bottom | <length-percentage>
```

Values must be specified in the given order, any unspecified properties will be left at their default values. See the 'demo' sample for usage examples.


### Font-effects

The new RCSS `font-effect` property replaces the old font-effect declarations in libRocket. A font-effect is declared similar to a decorator, by the name of the font-effect type and its properties in parenthesis. Some examples follow.

```css
/* declares an outline font-effect with width 5px and color #f66 */
font-effect: outline( 5px #f66 );

/* declares a shadow font-effect with 2px offset in both x- and y-direction, and the given color */
font-effect: shadow( 2px 2px #333 );
```

The `font-effect` property follows the normal cascading rules, is inherited, and has the default value `none` which specifies no font-effect on the element. Unlike in libRocket, font-effects can now be set on the element's style, although we recommend declaring them in style sheets for performance reasons.

Furthermore, multiple font-effects can be specified on any element by a comma-separated list of font-effects.
```css
/* declares two font-effects on the same element */
font-effect: shadow(3px 3px green), outline(2px black);
```

When creating a custom font-effect, you can provide a shorthand property named `font-effect` which will be used to parse the text inside the parenthesis of the property declaration. This allows specifying the font-effect with inline properties as in the above examples.

There is currently no equivalent of the `@decorator` at-rule for font-effects. If there is a desire for such a feature, please provide some feedback.

### RCSS Selectors

The child combinator `>` is now introduced in RCSS, which can be used as in CSS to select a child of another element.
```css
p.green_theme > button { image-color: #0f0; }
```
Here, any `button` elements which have a parent `p.green_theme` will have their image color set to green. 

Furthermore, the universal selector `*` can now be used in RCSS. This selector matches any element.
```css
div.red_theme > * > p { color: #f00; }
```
Here, `p` grandchildren of `div.red_theme` will have their color set to red. The universal selector can also be used in combination with other selectors, such as `*.great#content:hover`.

### Debugger improvements

The debugger has been improved in several aspects:

- Live updating of values. Can now see the effect of animations and other property changes.
- Can now toggle drawing of element dimension box, and live update of values.
- Can toggle pseudo classes on the selected element.
- Added the ability to clear the log.
- Support for transforms. The element's dimension box is drawn with the transform applied.

### Removal of manual reference counting

All manual reference counting has been removed in favor of smart pointers. There is no longer a need to manually decrement the reference count, such as `element->RemoveReference()` as before. This change also establishes a clear ownership of objects. For the user-facing API, this means raw pointers are non-owning, while unique and shared pointers declare ownership. Internally, there may still be uniquely owning raw pointers, as this is a work-in-progress.

#### Core API

The Core API takes raw pointers as before such as for its interfaces. With the new semantics, this means the library retains a non-owning reference. Thus, all construction and destruction of such objects is the responsibility of the user. Typically, the objects must stay alive until after `Core::Shutdown` is called. Each relevant function is commented with its lifetime requirements.

As an example, the system interface can be constructed into a unique pointer.
```cpp
auto system_interface = std::make_unique<MySystemInterface>();
Rml::Core::SetSystemInterface(system_interface.get());
Rml::Core::Initialise();
...
Rml::Core::Shutdown();
system_interface.reset();
```
Or simply from a stack object.
```cpp
MySystemInterface system_interface;
Rml::Core::SetSystemInterface(&system_interface);
Rml::Core::Initialise();
...
Rml::Core::Shutdown();
```

#### Element API

When constructing new elements, there is again no longer a need to decrement the reference count as before. Instead, the element is returned with a unique ownership
```cpp
ElementPtr ElementDocument::CreateElement(const String& name);
```
where `ElementPtr` is a unique pointer and an alias as follows.
```cpp
using ElementPtr = std::unique_ptr<Element, Releaser<Element>>;
```
Note that, the custom deleter `Releaser` is there to ensure the element is released from the `ElementInstancer` in which it was created.

After having called `ElementDocument::CreateElement`, the element can be moved into the list of children of another element.
```cpp
ElementPtr new_child = document->CreateElement("div");
element->AppendChild( std::move(new_child) );
```
Since we moved `new_child`, we cannot use the pointer anymore. Instead, `Element::AppendChild` returns a non-owning raw pointer to the appended child which can be used. Furthermore, the new element can be constructed in-place, e.g.
```cpp
Element* new_child = element->AppendChild( document->CreateElement("div") );
```
and now `new_child` can safely be used until the element is destroyed.

There are aliases to the smart pointers which are used internally for consistency with the library's naming scheme.
```cpp
template<typename T> using UniquePtr = std::unique_ptr<T>;
template<typename T> using SharedPtr = std::shared_ptr<T>;
```

### Improved transforms

The inner workings of transforms have been completely revised, resulting in increased performance, simplified API, closer compliance to the CSS specs, and reduced complexity of the relevant parts of the library.

Some relevant changes for users:
- Removed the need for users to set the view and projection matrices they use outside the library.
- Replaced the `PushTransform()` and `PopTransform()` render interface functions with `SetTransform()`, which is only called when the transform matrix needs to change and never called if there are no `transform` properties present.
- The `perspective` property now applies to the element's children, as in CSS.
- The transform function `perspective()` behaves like in CSS. It applies a perspective projection to the current element.
- Chaining transforms and perspectives now provides more expected results. However, as opposed to CSS we don't flatten transforms.
- Have a look at the updated transforms sample for some fun with 3d boxes.


### Focus flags, autofocus

It is now possible to autofocus on elements when showing a document. By default, the first element with the property `tab-index: auto;` as well as the attribute `autofocus` set, will receive focus.

The focus behavior as well as the modal state can be controlled with two new separate flags.
```cpp
ElementDocument::Show(ModalFlag modal_flag = ModalFlag::None, FocusFlag focus_flag = FocusFlag::Auto);
```

The flags are specified as follows:
```cpp
/**
	 ModalFlag used for controlling the modal state of the document.
		None:  Remove modal state.
		Modal: Set modal state, other documents cannot receive focus.
		Keep:  Modal state unchanged.

	FocusFlag used for displaying the document.
		None:     No focus.
		Document: Focus the document.
		Keep:     Focus the element in the document which last had focus.
		Auto:     Focus the first tab element with the 'autofocus' attribute or else the document.
*/
enum class ModalFlag { None, Modal, Keep };
enum class FocusFlag { None, Document, Keep, Auto };
```


### Font engine and interface

The RmlUi font engine has seen a major overhaul.

- The default font engine has been abstracted away, thereby allowing users to implement their own font engine (thanks to @viciious). See `FontEngineInterface.h` and the CMake flag `NO_FONT_INTERFACE_DEFAULT` for details.
- `font-charset` RCSS property is gone: The font interface now loads new characters as needed. Fallback fonts can be set so that unknown characters are loaded from them.
- The API and internals are now using UTF-8 strings directly, the old UCS-2 strings are ditched completely. All `String`s in RmlUi should be considered as encoded in UTF-8.
- Text string are no longer limited to 16 bit code points, thus grayscale emojis are supported, have a look at the `demo` sample for some examples.
- The internals of the default font engine has had a major overhaul, simplifying a lot of code, and removing the BitmapFont provider.
- Instead, a custom font engine interface has been implemented for bitmap fonts in the `bitmapfont` sample, serving as a quick example of how to create your own font interface. The sample should work even without the FreeType dependency.


### CMake options

Three new CMake options added.

- `NO_FONT_INTERFACE_DEFAULT` removes the default font engine, thereby allowing users to completely remove the FreeType dependency. If set, a custom font engine must be created and set through `Rml::Core::SetFontEngineInterface` before initialization. See the `bitmapfont` sample for an example implementation of a custom font engine.
- `NO_THIRDPARTY_CONTAINERS`: RmlUi now comes bundled with some third-party container libraries for improved performance. For users that would rather use the `std` counter-parts, this option is available. The option replaces the containers via a preprocessor definition. If the library is compiled with this option, then users of the library *must* specify `#define RMLUI_NO_THIRDPARTY_CONTAINERS` before including the library.
- `ENABLE_TRACY_PROFILING`: RmlUi has parts of the library tagged with markers for profiling with [Tracy Profiler](https://bitbucket.org/wolfpld/tracy/src/master/). This enables a visual inspection of bottlenecks and slowdowns on individual frames. To compile the library with profiling support, add the Tracy Profiler library to `/Dependencies/tracy/`, enable this option, and compile.  Follow the Tracy Profiler instructions to build and connect the separate viewer. As users may want to only use profiling for specific compilation targets, then instead one can `#define RMLUI_ENABLE_PROFILING` for the given target.


### Other features

- `Context::ProcessMouseWheel` now takes a float value for the `wheel_delta` property, thereby enabling continuous/smooth scrolling for input devices with such support. The default scroll length for unity value of `wheel_delta` is now three times the default line-height multiplied by the current dp-ratio.
- The system interface now has two new functions for setting and getting text to and from the clipboard: `virtual void SystemInterface::SetClipboardText(const Core::String& text)` and `virtual void SystemInterface::GetClipboardText(Core::String& text)`.
- The `text-decoration` property can now also be used with `overline` and `line-through`.
- The text input and text area elements can be navigated word for word by holding the 'ctrl' key.
- The `<img>` element can now take sprite names in its `src` attribute.


### Breaking changes

Breaking changes since RmlUi v2.0.

- RmlUi now requires a C++14-compatible compiler (previously C++11).
- Rml::Core::String has been replaced by std::string, thus, interfacing with the library now requires you to change your string types. This change was motivated by a small performance gain, additionally, it should make it easier to interface with the library especially for users already using std::string in their codebase. Furthermore, strings should be considered as encoded in UTF-8.
- To load fonts, use `Rml::Core::LoadFontFace` instead of `Rml::Core::FontDatabase::LoadFontFace`.
- Querying the property of an element for size, position and similar may not work as expected right after changes to the document or style. This change is made for performance reasons, see the description under *performance* for reasoning and a workaround.
- The Controls::DataGrid "min-rows" property has been removed.
- Removed RenderInterface::GetPixelsPerInch, instead the pixels per inch value has been fixed to 96 PPI, as per CSS specs. To achieve a scalable user interface, instead use the 'dp' unit.
- Removed 'top' and 'bottom' from z-index property.
- Angles need to be declared in either 'deg' or 'rad'. Unit-less numbers do not work.
- See changes to the declaration of decorators and font-effects above.
- See changes to the render interface regarding transforms above.
- The focus flag in `ElementDocument::Show` has been changed, with a new enum name and new options, see above.
- The tiled decorators (`image`, `tiled-horizontal`, `tiled-vertical`, and `tiled-box`) no longer support the old repeat modes.
- Also, see removal of manual reference counting above.


#### Events

There are some changes to events in RmlUi, however, for most users, existing code should still work as before.

There is now a distinction between actions executed in event listeners, and default actions for events:

- Event listeners are attached to an element as before. Events follow the normal phases: capture (root -> target), target, and bubble (target -> root). Each event listener can be either attached to the bubble phase (default) or capture phase. The target phase always executes if reached. Listeners are executed in the order they are added to the element. Each event type specifies whether it executes the bubble phase or not, see below for details.
- Default actions are primarily for actions performed internally in the library. They are executed in the function `virtual void Element::ProcessDefaultAction(Event& event)`. However, any object that derives from `Element` can override the default behavior and add new behavior. The default actions follow the normal event phases, but are only executed in the phase according to their `default_action_phase` which is defined for each event type. If an event is cancelled with `Event::StopPropagation()`, then the default action is not performed unless already executed.


Each event type now has an associated EventId as well as a specification defined as follows:

- `interruptible`: Whether the event can be cancelled by calling `Event::StopPropagation()`.
- `bubbles`: Whether the event executes the bubble phase. If true, all three phases: capture, target, and bubble, are executed. If false, only capture and target phases are executed.
- `default_action_phase`: One of: None, Target, Bubble, TargetAndBubble. Specifies during which phases the default action is executed, if any. That is, the phase for which `Element::ProcessDefaultAction` is called. See above for details.

See `EventSpecification.cpp` for details of each event type. For example, the event type `click` has the following specification:
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
- All event listeners on the current element will always be called after calling `StopPropagation()`. However, the default action on the current element will be prevented. When propagating to the next element, the event is stopped. This behavior is consistent with the standard DOM events model. The event can be stopped immediately with `StopImmediatePropagation()`.
- `Element::DispatchEvent` can now optionally take an `EventId` instead of a `String`.
- The `resize` event now only applies to the document size, not individual elements.
- The `scrollchange` event has been replaced by a function call. To capture scroll changes, instead use the `scroll` event.



## RmlUi 2.0 features

RmlUi introduces several features over the [original libRocket branch](https://github.com/libRocket/libRocket). While some of the new features are briefly documented here, take a look at the [official RmlUi documentation](https://mikke89.github.io/RmlUiDoc/) for more details.

### Transform property

Based on the work of @shoemark, with additional fixes.

Use `perspective`, `perspective-origin`, `transform` and `transform-origin` in RCSS, roughly equivalent to their respective CSS properties.

```css
perspective: 1000px;
perspective-origin: 20px 50%;
transform: rotateX(10deg) skew(-10deg, 15deg) translateZ(100px);
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
rotate3d,     number3angle1
skewX,        angle1
skewY,        angle1
skew,         angle2
```

Angles take units of 'deg' or 'rad'.





### Animations


Most RCSS properties can be animated, this includes properties representing lengths, colors, or transforms. From C++, an animation can be started on an Element by calling

```cpp
bool Element::Animate(const String& property_name, const Property& target_value, float duration, Tween tween = Tween{}, int num_iterations = 1, bool alternate_direction = true, float delay = 0.0f, const Property* start_value = nullptr);
```

Additional animation keys can be added, extending the duration of the animation, by calling

```cpp
bool Element::AddAnimationKey(const String& property_name, const Property& target_value, float duration, Tween tween = Tween{});
```

C++ example usage:

```cpp
auto p1 = Transform::MakeProperty({ Transforms::Rotate2D{10.f}, Transforms::TranslateX{100.f} });
auto p2 = Transform::MakeProperty({ Transforms::Scale2D{3.f} });
el->Animate("transform", p1, 1.8f, Tween{ Tween::Elastic, Tween::InOut }, -1, true);
el->AddAnimationKey("transform", p2, 1.3f, Tween{ Tween::Elastic, Tween::InOut });
```


Animations can also be specified entirely in RCSS, with keyframes.
```css
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

```css
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

Animations are very powerful coupled with transforms. See the animation sample project for more examples and details. There are also some [video demonstrations](https://mikke89.github.io/RmlUiDoc/pages/rcss/animations_transitions_transforms.html) of these features in the documentation.


### Transitions

Transitions apply an animation between two property values on an element when its property changes. Transitions are implemented in RCSS similar to how they operate in CSS. However, in RCSS, they only apply when a class or pseudo-class is added to or removed from an element.

```css
transition: <space-separated-list-of-properties|all|none> <duration> <delay> <tweening-function>;
```
The property list specifies the properties to be animated. Delay and tweening-function are optional. Delay must be specified after duration, otherwise values can be given in any order. Multiple transitions can be specified on the same element by using a comma-separated list. The tweening function is specified as in the `animation` RCSS property.


Example usage:

```css
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

```cpp
float dp_ratio = 1.5f;
context->SetDensityIndependentPixelRatio(dp_ratio);
```

Usage example in RCSS:
```css
div#header 
{
	width: 800dp;
	height: 50dp;
	font-size: 20dp;
}
```


### Pointer events

Set the element property to disregard mouse input events on this and descending elements.
```css
pointer-events: none;
```
Default is `auto`.


### Image-color property

Non-standard RCSS property which multiplies a color with images in `<img>` tags and image decorators. Useful for `:hover`-events and for applying transparency.
```css
image-color: rgba(255, 160, 160, 200);
icon-decorator: image;
icon-image: background.png 34px 0px 66px 28px;
```


### Inline-block

Unlike the original branch, elements with
```css
display: inline-block;
```
will shrink to the width of their content, like in CSS.



### Border shorthand

Enables the `border` property shorthand.
```css
border: 4px #e99;
```


### Various changes

- The namespace has changed from `Rocket` to `Rml`, and include path from `<Rocket/...>` to `<RmlUi/...>`.
- The slider on the `input.range` element can be dragged from anywhere in the element.
- The `:checked` pseudo class can be used to style the selected item in drop-down lists.


## License (MIT)
 
 Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors\
 Copyright (c) 2019 The RmlUi Team, and contributors
 
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
 
