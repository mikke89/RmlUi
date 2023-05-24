* [RmlUi 5.1](#rmlui-51)
* [RmlUi 5.0](#rmlui-50)
* [RmlUi 4.4](#rmlui-44)
* [RmlUi 4.3](#rmlui-43)
* [RmlUi 4.2](#rmlui-42)
* [RmlUi 4.1](#rmlui-41)
* [RmlUi 4.0](#rmlui-40)
* [RmlUi 3.3](#rmlui-33)
* [RmlUi 3.2](#rmlui-32)
* [RmlUi 3.1](#rmlui-31)
* [RmlUi 3.0](#rmlui-30)
* [RmlUi 2.0](#rmlui-20)


## RmlUi 6.0 (WIP)

### Major layout engine improvements

- Make layout more conformant to CSS specification.
  - Rewritten inline layouting.
  - Fixed more than a hundred CSS tests, including ACID1.
- Improve readability and maintainability:
  - Better separation of classes, reduce available state.
  - Make classes better conform to CSS terminology.
  - Improve call-graph, flow from parent to child, avoid mutable calls into ancestors.
- Fix long-standing issues.
  - Allow tables and flexboxes to be absolutely positioned or floated.
  - Allow nested flexboxes: Flex items can now be flex containers themselves. #320
  - Better handling of block-level boxes in inline formatting contexts. #392

More details to be posted later. Expect some possible layout shifts in existing documents, usually due to better CSS conformance. Some new issues are expected during development, please report bugs, and layout changes that are not in compliance with CSS.

### RCSS Properties

- New `display` property values: `flow-root`, `inline-flex`, `inline-table`.
- New `vertical-align` property value: `center`.
- Added support for `letter-spacing` property. #429 (thanks @igorsegallafa)

### Breaking changes

- Possible layout changes, usually due to better CSS conformance.
- Reworked font engine interface, in particular in terms of font metrics and letter-spacing.

Changed `Box` enums and `Property` units as follows:
- `Box::Area` -> `BoxArea` (e.g. `Box::BORDER` -> `BoxArea::Border`, values now in titlecase).
- `Box::Edge` -> `BoxEdge` (e.g. `Box::TOP` -> `BoxEdge::Top`, values now in titlecase).
- `Box::Direction` -> `BoxDirection` (e.g. `Box::VERTICAL` -> `BoxDirection::Vertical`, values now in titlecase).
- `Property::Unit` -> `Unit` (e.g. `Property::PX` -> `Unit::PX`).
- Replaced `Element::ResolveNumericProperty` with `Element::ResolveLength` and `Element::ResolveNumericValue`. Can be used together with `Property::GetNumericValue`.

Removed deprecated functionality:
- Removed the `<datagrid>` and `<dataselect>` elements, related utilities, and associated tutorials. Users are encouraged to replace this functionality by [tables](https://mikke89.github.io/RmlUiDoc/pages/rcss/tables.html), [select boxes](https://mikke89.github.io/RmlUiDoc/pages/rml/forms.html#select), and [data bindings](https://mikke89.github.io/RmlUiDoc/pages/data_bindings.html).


## RmlUi 5.1

### New scrolling features

#### Autoscroll mode
Autoscroll mode, or scrolling with the middle mouse button, is now featured in RmlUi. #422 #423 (thanks @igorsegallafa)

This mode is automatically enabled by the context whenever a middle mouse button press is detected, and there is an element to scroll under the mouse. There is also support for holding the middle mouse button to scroll. 

When autoscroll mode is active, a cursor name is submitted to clients which indicates the state of the autoscroll, so that clients can display an appropriate cursor to the user. These cursor names all start with `rmlui-scroll-`, and take priority over any active `cursor` property. If desired, autoscroll mode can be disabled entirely by simply not submitting middle mouse button presses, or by using another button index when submitting the button to the context. 

See the new [documentation section on scrolling](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/contexts.html#scrolling) for details.

#### Smooth scrolling
Smooth scrolling is now supported in RmlUi, and enabled by default. This makes certain scroll actions animate smoothly towards its destination. Smooth scrolling may become active in the following situations:

- During a call to `Context::ProcessMouseWheel()`.
- When clicking a scrollbar's arrow keys or track.
- When calling any of the `Element::Scroll...()` methods with the `ScrollBehavior::Smooth` enum value.

Smooth scrolling can be disabled or tweaked on the context, as described below.

#### Context interface
Smooth scrolling can be disabled, or tweaked, by calling the following method on a given context:
```cpp
void Context::SetDefaultScrollBehavior(ScrollBehavior scroll_behavior, float speed_factor);
```
Here, smooth scrolling can be disabled by submitting the `ScrollBehavior::Instant` enum value. The scrolling speed can also be tweaked by adjusting the speed factor.

The function signature of `Context::ProcessMouseWheel` has been replaced with the following, to enable scrolling in both dimensions:
```cpp
bool Context::ProcessMouseWheel(Vector2f wheel_delta, int key_modifier_state);
```
The old single axis version is still available for backward compatibility, but considered deprecated and may be removed in the future.

#### Element interface
Added
```cpp
void Element::ScrollTo(Vector2f offset, ScrollBehavior behavior = ScrollBehavior::Instant);
```
which scrolls the element to the given coordinates, with the ability to use smooth scrolling. Similarly, `Element::ScrollIntoView` has been updated with the ability to perform smooth scrolling (instant by default).

#### Scroll events
The `mousescroll` event no longer performs scrolling on an element, and no longer requires a default action. Instead, the responsibility for mouse scrolling has been moved to the context and its scroll controller. However, the `mousescroll` event is still submitted during a mouse wheel action, with the option to cancel the scroll by stopping its propagation. The event is now also submitted before initiating autoscroll mode, with the possibility to cancel it.

### New RCSS features

- New [`overscroll-behavior` property](https://mikke89.github.io/RmlUiDoc/pages/rcss/user_interface.html#overscroll-behavior). An element's closest scrollable ancestor is decided by scroll chaining, which can be controlled using this property. The `contain` value can be used to ensure that mouse wheel scrolling is not propagated outside a given element, regardless of whether its scrollbars are visible.
- Added animation support for decorators. #421 (thanks @0suddenly0)
- Sibling selectors will now also match hidden elements.

### On-demand rendering (power saving mode)

In games, the update and render loop normally run as fast as possible. However, in some applications it is desirable to reduce CPU usage and power consumption when the application is idle. RmlUi now provides the necessary utilities to achieve this. Implemented in #436 (thanks @Thalhammer), see also #331 #417 #430.

Users of RmlUi control their own update loop, however, this feature requires some support from the library side, because the application needs to know e.g. when animations are happening or when a text cursor should blink. In short, to implement this, users can now query the context for `Context::GetNextUpdateDelay()`, which returns the time until the next update loop should be run again.

See the [on-demand rendering documentation](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/contexts.html#on-demand-rendering) for details and examples.

### Text selection interface

Added the ability to set or retrieve the text selection on text fields. #419

In particular, the following methods are now available on `ElementFormControlInput` (`<input>` elements) and `ElementFormControlTextArea` (`<textarea>` elements):

```cpp
// Selects all text.
void Select();
// Selects the text in the given character range.
void SetSelectionRange(int selection_start, int selection_end);
// Retrieves the selection range and text.
void GetSelection(int* selection_start, int* selection_end, String* selected_text) const;
```

See the [form controls documentation](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/element_packages/form.html#text-selection) for details.

### RML and form element improvements

- Add RML support for numeric character reference (Unicode escape sequences). #401 (thanks @dakror)
- Make the `:checked` pseudo class active on the `<select>` element whenever its options list is open, for better styling capabilities.
- Fix max length in text input fields not always clamping the value, such as when pasting text.
- The slider input now only responds to the primary mouse button.
- The slider input is now only draggable from the track or bar, instead of the whole element.
- Fixed input elements not always being correctly setup when changing types.

### Data bindings

- Add new [data-alias attribute](https://mikke89.github.io/RmlUiDoc/pages/data_bindings/views_and_controllers.html#data-alias) to make templates work with outside variables. #432 (thanks @dakror)
- Add method to retrieve the `DataTypeRegister` during model construction. #412 #413 (thanks @LoneBoco)
- Add ability to provide a separate data type register to use when constructing a new data model. Can be useful to provide a distinct type register for each shared library accessing the same context. Alternatively, allows different contexts to share a single type register. #409 (thanks @eugeneko)

### Lua plugin

- Make the Lua plugin compatible with Lua 5.1+ and LuaJIT. #387 (thanks @mrianura)
- Updated to include the new text selection API. #434 #435 (thanks @ShawnCZek)

### Backends

- Vulkan renderer: Fix various Vulkan issues on Linux. #430 (thanks @Thalhammer)
- GL3 renderer: Unbind the vertex array after use to avoid possible crash. #411

### Stability improvements

- Fix a potential crash during plugin shutdown. #415 (thanks @LoneBoco)

### Build improvements

- Improve CMake to better support RmlUi included as a subdirectory of a parent project. #394 #395 #397 (thanks @cathaysia)
- Fix possible name clashes causing build failures during argument-dependent lookup (ADL). #418 #420 (thanks @mwl4)

### Built-in containers

- Replaced custom containers based on chobo-shl with equivalent ones from [itlib](https://github.com/iboB/itlib). #402 (thanks @iboB)

### Breaking changes

- Scrolling behavior changed, increased default mouse wheel scroll length, and enabled smooth scrolling by default. See notes above.
- The `mousescroll` event no longer scrolls an element. Its `wheel_delta` parameter has been renamed to `wheel_delta_y`.
- The signature of `Context::ProcessMouseWheel` has been changed, the old signature is still available but deprecated.


## RmlUi 5.0

### Backends

RmlUi 5.0 introduces the backends concept. This is a complete refactoring of the old sample shell, replacing most of it with a multitude of backends. A backend is a combination of a renderer and a platform, and a light interface tying them together. The shell is now only used for common functions specific to the included samples.

This change is beneficial in several aspects:

- Makes it easier to integrate RmlUi, as users can directly use the renderer and platform suited for their setup.
- Makes it a lot easier to add new backends and maintain existing ones.
- Allows all the samples to run on any backend by choosing the desired backend during CMake configuration.

All samples and tests have been updated to work with the [backends interface](Backends/RmlUi_Backend.h), which is a very light abstraction over all the different backends.

The following renderers and platforms are included:

- A new OpenGL 3 renderer. #261
  - Including Emscripten support so RmlUi even runs in web browsers now.
- A new Vulkan renderer. #236 #328 #360 #385 (thanks @wh1t3lord)
- A new GLFW platform.
- The OpenGL 2 and SDL native renderers ported from the old samples.
- The Win32, X11, SFML, and SDL platforms ported from the old samples.

The old macOS shell has been removed as it used a legacy API that is no longer working on modern Apple devices. Now the samples build again on macOS using one of the windowing libraries such as GLFW or SDL.

See the [Backends section in the readme](readme.md#rmlui-backends) for all the combinations of renderers and platforms, and more details. The backend used for running the samples can be selected by setting the CMake option `SAMPLES_BACKEND` to any of the [supported backends](readme.md#rmlui-backends).

### RCSS selectors

- Implemented the next-sibling `+` and subsequent-sibling `~` [combinators](https://mikke89.github.io/RmlUiDoc/pages/rcss/selectors.html).
- Implemented attribute selectors `[foo]`, `[foo=bar]`, `[foo~=bar]`, `[foo|=bar]`, `[foo^=bar]`, `[foo$=bar]`, `[foo*=bar]`. #240 (thanks @aquawicket)
- Implemented the negation pseudo class `:not()`, including support for selector lists `E:not(s1, s2, ...)`.
- Refactored structural pseudo classes for improved performance.
- Selectors will no longer match any text elements, like in CSS.
- Selectors now correctly consider all paths toward the root, not just the first greedy path.
- Structural selectors are no longer affected by the element's display property, like in CSS.

### RCSS properties

- `max-width` and `max-height` properties now support the `none` keyword.

### Text editing

The `<textarea>` and `<input type="text">` elements have been improved in several aspects.

- Improved cursor navigation between words (Ctrl + Left/Right).
- Selection is now expanded to highlight selected newlines.
- When word-wrap is enabled, words can now be broken to avoid overflow.
- Fixed several issues where the text cursor would be offset from the text editing operations. In particular after word wrapping, or when suppressed characters were present in the text field's value. #313
- Fixed an issue where Windows newline endings (\r\n) would produce an excessive space character.
- Fixed operation of page up/down numpad keys being swapped.
- The input method editor (IME) is now positioned at the caret during text editing on the Windows backend. #303 #305 (thanks @xland)
- Fix slow input handling especially with CJK input on the Win32 backend. #311
- Improve performance on document load and during text editing.

### Elements

- Extend the functionality of `Element::ScrollIntoView`. #353 (thanks @eugeneko)
- `<img>` element: Fix wrong dp-scaling being applied when an image is cloned through a parent element. #310
- `<handle>` element: Fix move targets changing size when placed using the `top`/`right`/`bottom`/`left` properties.

### Data binding

- Transform functions can now be called using C-like calling conventions, in addition to the previous pipe-syntax. Thus, the data expression `3.625 | format(2)` can now be identically expressed as `format(3.625, 2)`.
- Handle null pointers when trying to access data variables. #377 (thanks @Dakror)
- Enable registering a custom data variable definition. #367 (thanks @Dakror)

### Context input

- The hover state of any elements under the mouse will now automatically be updated during `Context::Update()`. #220
- Added `Context::ProcessMouseLeave()` which ensures that the hovered state is removed from all elements and stops the context update from automatically hovering elements.
- When `Context::ProcessMouseMove()` is called next the context update will start updating hover states again.
- Added support for mouse leave events on all backends.

### Layout improvements

- Scroll and slider elements now use containing block's height instead of width to calculate height-relative values. #314 #321 (thanks @nimble0)
- Generate warnings when directly nesting flexboxes and other unsupported elements in a top-level formatting context. #320
- In some situations, changing the `top`/`right`/`bottom`/`left` properties may affect the element's size. These situations are now correctly handled so that the layout is updated when needed.

### Lua plugin

- Add `QuerySelector` and `QuerySelectorAll` to the Lua Element API. #329 (thanks @Dakror)
- Add input-related methods and `dp_ratio` to the Lua Context API. #381 #386 (thanks @shubin)
- Lua objects representing C++ pointers now compare equal if they point to the same object. #330 (thanks @Dakror)
- Add length to proxy for element children. #315 (thanks @nimble0)
- Fix a crash in the Lua plugin during garbage collection. #340 (thanks @slipher)

### Debugger plugin

- Show a more descriptive name for children in the element info window.
- Fixed a crash when the debugger plugin was shutdown manually. #322 #323 (thanks @LoneBoco)

### SVG plugin

- Update texture when the `src` attribute changes. #361

### General improvements

- Small performance improvement when generating font effects.
- Allow empty values in decorators and font-effects.
- RCSS located within document `<style>` tags can now take comments containing xml tags. #341
- Improve in-source function documentation. #334 (thanks @hobyst)
- Invader sample: Rename event listener and instancer for clarity.

### General fixes

- Font textures are no longer being regenerated when encountering new ascii characters, fixes a recent regression.
- Logging a message without an installed system interface will now be written to cout instead of crashing the application.
- Fix several static analyzer warnings and one possible case of undefined behavior.
- SDL renderer: Fix images with no alpha channel not rendering. #239

### Build fixes

- Fix compilation on Emscripten CI. #335 (thanks @hobyst)
- Fix compilation with EASTL. #350 (thanks @eugeneko)

### Breaking changes

- Changed the signature of the keyboard activation in the system interface, it now passes the caret position and line height: `SystemInterface::ActivateKeyboard(Rml::Vector2f caret_position, float line_height)`.
- Mouse and hover behavior may change in some situations as a result of the hover state automatically being updated on `Context::Update()`, even if the mouse is not moved. See above changes to context input.
- Removed the boolean result returned from `Rml::Debugger::Shutdown()`.
- RCSS selectors will no longer match text elements.
- RCSS structural pseudo selectors are no longer affected by the element's display property.
- Data binding: The signature of transform functions has been changed from `Variant& first_argument_and_result, const VariantList& other_arguments -> bool success` to `const VariantList& arguments -> Variant result`.


## RmlUi 4.4

### Fonts

- Support for color emojis 🎉. [#267](https://github.com/mikke89/RmlUi/issues/267)
- Support for loading fonts with multiple included font weights. [#296](https://github.com/mikke89/RmlUi/pull/296) (thanks @MexUK)
- The `font-weight` property now supports numeric values. [#296](https://github.com/mikke89/RmlUi/pull/296) (thanks @MexUK)
- The `opacity` property is now also applied to font effects. [#270](https://github.com/mikke89/RmlUi/issues/270)

### Performance and resource management

- Substantial performance improvement when looking up style rules with class names. Fixes some cases of low performance, see [#293](https://github.com/mikke89/RmlUi/issues/293).
- Reduced memory usage, more than halved the size of `ComputedValues`.
- Added `Rml::ReleaseFontResources` to release unused font textures, cached glyph data, and related resources.
- Release memory pools on `Rml::Shutdown`, or manually through the core API. [#263](https://github.com/mikke89/RmlUi/issues/263) [#265](https://github.com/mikke89/RmlUi/pull/265) (thanks @jack9267)

### Layout

- Fix offsets of relatively positioned elements with percentage positioning. [#262](https://github.com/mikke89/RmlUi/issues/262)
- `select` element: Fix clipping on select box.

### Data binding

- Add `DataModelHandle::DirtyAllVariables()` to mark all variables in the data model as dirty. [#289](https://github.com/mikke89/RmlUi/pull/289) (thanks @EhWhoAmI)

### Cloning

- Fix classes not always copied over to a cloned element. [#264](https://github.com/mikke89/RmlUi/issues/264)
- Drag clones are now positioned correctly when their ancestors use transforms. [#269](https://github.com/mikke89/RmlUi/issues/269)
- Drag clones no longer inherit backgrounds and decoration from the cloned element's document body.

### Samples and plugins

- New sample for integration with SDL2's native renderer. [#252](https://github.com/mikke89/RmlUi/pull/252) (thanks @1bsyl)
- Add `width` and `height` attributes to the `<svg>` element. [#283](https://github.com/mikke89/RmlUi/pull/283) (thanks @EhWhoAmI)

### Build improvements

- CMake: Mark RmlCore dependencies as private. [#274](https://github.com/mikke89/RmlUi/pull/274) (thanks @jonesmz)
- CMake: Allow `lunasvg` library be found when located in builtin tree. [#282](https://github.com/mikke89/RmlUi/pull/282) (thanks @EhWhoAmI)

### Breaking changes

- `FontEngineInterface::GenerateString` now takes an additional argument, `opacity`.
- Computed values are now retrieved by function calls instead of member objects.


## RmlUi 4.3

### Flexbox layout

Support for flexible box layout. [#182](https://github.com/mikke89/RmlUi/issues/182) [#257](https://github.com/mikke89/RmlUi/pull/257)

```css
display: flex;
```

See usage examples, differences from CSS, performance tips, and all the details [in the flexbox documentation](https://mikke89.github.io/RmlUiDoc/pages/rcss/flexboxes.html).

### Elements

- `select` element:
  - Emit `change` event even when the value of the newly selected option is the same.
  - Do not wrap around when using up/down keys on options.
  - Fix unintentional clipping of the scrollbar.
- `tabset` element: Fix index parameter having no effect in ElementTabSet::SetPanel and ElementTabSet::SetTab. [#237](https://github.com/mikke89/RmlUi/issues/237) [#246](https://github.com/mikke89/RmlUi/pull/246) (thanks @nimble0)

### RCSS

- Add (non-standard) property value `clip: always` to force clipping to the element, [see `clip` documentation](https://mikke89.github.io/RmlUiDoc/pages/rcss/visual_effects.html#clip). [#235](https://github.com/mikke89/RmlUi/issues/235) [#251](https://github.com/mikke89/RmlUi/pull/251) (thanks @MatthiasJFM)
- Escape character changed from forward slash to backslash `\` to align with CSS.

### Layout improvements

- See flexbox layout support above.
- Fix an issue where some elements could end up rendered at the wrong offset after scrolling. [#230](https://github.com/mikke89/RmlUi/issues/230)
- Improve behavior for collapsing negative vertical margins.
- Pixel rounding improvements to the clip region.
- Performance improvement: Avoid unnecesssary extra layouting step in some situations when scrollbars are added.

### Samples

- Add clipboard support to X11 samples. [#231](https://github.com/mikke89/RmlUi/pull/231) (thanks @barotto)
- Windows shell: Fix OpenGL error on startup.

### Tests

- Visual tests suite: Add ability to overlay the previous reference capture of the test using the shortcut Ctrl+Q.
- Add support for CSS flexbox tests to the CSS tests converter.

### Lua plugin

- Make Lua plugin API consistently one-indexed instead of zero-indexed. [#237](https://github.com/mikke89/RmlUi/issues/237) [#247](https://github.com/mikke89/RmlUi/pull/247) (thanks @nimble0)
- Fix crash due to double delete in the Lua plugin. [#216](https://github.com/mikke89/RmlUi/issues/216)

### Dependencies

- Update LunaSVG plugin for compatibility with v2.3.0. [#232](https://github.com/mikke89/RmlUi/issues/232)
- Warn when using FreeType 2.11.0 with the MSVC compiler, as this version introduced [an issue](https://gitlab.freedesktop.org/freetype/freetype/-/issues/1092) leading to crashes.

### Build improvements

- Fix log message format string when compiling in debug mode. [#234](https://github.com/mikke89/RmlUi/pull/234) (thanks @barotto)
- Avoid enum name collisions with Windows header macros. [#258](https://github.com/mikke89/RmlUi/issues/258)

### Breaking changes

- The `clip` property is now non-inherited. Users may need to update their RCSS selectors to achieve the same clipping behavior as previously when using this property.
- Minor changes to the clipping behavior when explicitly using the `clip` property, may lead to different results in some circumstances.
- RCSS escape character is now `\` instead of `/`.
- Lua plugin: Some scripts may need to be changed from using zero-based indexing to one-indexing.


## RmlUi 4.2

### Improvements

- Add `Rml::Debugger::Shutdown`. This allows the debugger to be restarted on another host context. [#200](https://github.com/mikke89/RmlUi/issues/200) [#201](https://github.com/mikke89/RmlUi/pull/201) (thanks @Lyatus)
- Improve color blending and animations. [#203](https://github.com/mikke89/RmlUi/pull/203) [#208](https://github.com/mikke89/RmlUi/pull/208) (thanks @jac8888)
- Improve error messages on missing font face.
- Export `Rml::Assert()` in release mode. [#209](https://github.com/mikke89/RmlUi/pull/209) (thanks @kinbei)
- Add `.clang-format`. [#223](https://github.com/mikke89/RmlUi/issues/223)

### Elements

- Fix a crash in some situations where the `input.range` element could result in infinite recursion. [#202](https://github.com/mikke89/RmlUi/issues/202)
- The `input.text` element will no longer copy to clipboard when the selection is empty.
- Checkboxes (`input.checkbox`) no longer require a `value` attribute to properly function. [#214](https://github.com/mikke89/RmlUi/pull/214) (thanks @ZombieRaccoon)
- Fix `handle` element resizing incorrectly when the size target has `box-sizing: border-box`. [#215](https://github.com/mikke89/RmlUi/pull/215) (thanks @nimble0)
- Improve warnings when using unsupported positioning and floating modes on tables. [#221](https://github.com/mikke89/RmlUi/issues/221)

### Samples

- Fix shortcut keys on X11 and macOS. [#210](https://github.com/mikke89/RmlUi/issues/210).
- Fix full reloading shortcut (Ctrl+R) in visual tests suite.

### Other fixes

- Fix minor layout issue in `inline-block`s. [#199](https://github.com/mikke89/RmlUi/pull/199) (thanks @svenvvv)
- Fix an issue in data bindings where text expressions initialized with an empty string would not be evaluated correctly. [#213](https://github.com/mikke89/RmlUi/issues/213)
- Fix an issue in the FreeType font engine where `.woff` files would cause a crash on shutdown. [#217](https://github.com/mikke89/RmlUi/issues/217)
- Fix inline styles not always being applied on a cloned element. [#218](https://github.com/mikke89/RmlUi/issues/218)
- Fix render interface destructor calling virtual functions in some circumstances. [#222](https://github.com/mikke89/RmlUi/issues/222)

### Breaking changes

- Removed built-in conversion functions between UTF-8 and UTF-16 character encodings.
- Slightly modified the lifetime requirements for the render interface for special use cases, see [requirements here](https://github.com/mikke89/RmlUi/blob/aa070e7292302f9e61f3b0b3f60e13aded0561d6/Include/RmlUi/Core/Core.h#L99-L100). Will warn in debug mode on wrong use. [#222](https://github.com/mikke89/RmlUi/issues/222)



## RmlUi 4.1

RmlUi 4.1 is a maintenance release.

- Several CMake fixes so that clients can more easily find and import RmlUi.
- Curly brackets can now be used inside string literals in data expressions. [#190](https://github.com/mikke89/RmlUi/pull/190) (thanks @Dakror).
- Inline events are now attached and detached when `on..` attributes change. [#189](https://github.com/mikke89/RmlUi/pull/189) (thanks @ZombieRaccoon).


## RmlUi 4.0

RmlUi 4.0 comes packed with several valuable new features as well as many fixes, as detailed below. The library has also been restructured to simplify its usage. For users coming from RmlUi 3.x, see [restructuring RmlUi](#restructuring-rmlui) below for details and an upgrade guide.

### Data bindings (model-view-controller)

RmlUi now supports a model-view-controller (MVC) approach through data bindings. This is a powerful approach for making documents respond to data changes, or in reverse, updating data based on user actions.

```html
<div data-model="my_model">
	<h2>{{title}}</h2>
	<p data-if="show_text">The quick brown fox jumps over the lazy {{animal}}.</p>
	<input type="text" data-value="animal"/>
</div>
```

- See [documentation for this feature](https://mikke89.github.io/RmlUiDoc/pages/data_bindings.html) and more examples therein.
- Have a look at the 'databinding' sample for usage examples and an enjoyable little game.
- See discussion in [#83](https://github.com/mikke89/RmlUi/pull/83) and [#25](https://github.com/mikke89/RmlUi/issues/25).

Thanks to contributions from @actboy, @cloudwu, @C-Core, @Dakror, and @Omegapol; and everyone who helped with testing!

### Test suite

Work has started on a complete test suite for RmlUi. The tests have been separated into three projects.

- `Visual tests`. For visually testing the layout engine in particular, with small test documents that can be easily added. Includes features for capturing and comparing tests for easily spotting differences during development. A best-effort conversion script for the [CSS 2.1 tests](https://www.w3.org/Style/CSS/Test/CSS2.1/), which includes thousands of tests, to RML/RCSS is included for testing conformance with the CSS specifications.
- `Unit tests`. To ensure smaller units of the library are working properly.
- `Benchmarks`. Benchmarking various components of the library to keep track of performance increases or regressions for future development, and find any performance hotspots that could need extra attention.

### Layout improvements

- Floating elements, absolutely positioned elements, and inline-block elements (as before) will now all shrink to the width of their contents when their width is set to `auto`.
- Replaced elements (eg. `img` and some `input` elements) now follow the normal CSS sizing rules. That is, padding and borders are no longer subtracted from the width and height of the element by default.
- Replaced elements can now decide whether to provide an intrinsic aspect ratio, such that users can eg. set the width property on `input.text` without affecting their height.
- Replaced elements now try to preserve the aspect ratio when both width and height are auto, and min/max-width/height are set.
- Fixed several situations where overflowing content would not be hidden or scrolled when setting a non-default `overflow` property. [#116](https://github.com/mikke89/RmlUi/issues/116)
- Overflow is now clipped on the padding area instead of the content area.

These changes may result in a differently rendered layout when upgrading to RmlUi 4.0. In particular the first two items. 

- If the shrink-to-fit width is undesired, set a definite width on the related elements, eg. using the `width` property or the `left`/`right` properties.
- Replaced elements may need adjustment of width and height, in this case it may be useful to use the new `box-sizing: border-box` property in some situations.

### Table support

RmlUi now supports tables like in CSS, with some differences and enhancements. RCSS supports flexible sizing of rows and columns (as if using the CSS `fr` unit in grid layout). See the [tables documentation](https://mikke89.github.io/RmlUiDoc/pages/rcss/tables.html) for details. 

```html
<table>
	<tr>
		<td>Name</td>
		<td colspan="2">Items</td>
		<td>Age</td>
	</tr>
	<tr>
		<td>Gimli</td>
		<td>Helmet</td>
		<td>Axe</td>
		<td>139 years</td>
	</tr>
</table>
```

Use the RCSS `display` property to enable table formatting. See the style sheet rules in the tables documentation to use the common HTML tags.

### New RCSS properties

- The `border-radius` property is now supported in RmlUi for drawing rounded backgrounds and borders.
- Implemented the `word-break` RCSS property.
- Implemented the `box-sizing` RCSS property.
- Implemented the `caret-color` RCSS property.
- New RCSS length units: `vw` and `vh`. [#162](https://github.com/mikke89/RmlUi/pull/162) (thanks @Dakror).

### RCSS Media query support

We now support `@media` rules for dynamically or programmatically changing active styles. [#169](https://github.com/mikke89/RmlUi/pull/169) (thanks @Dakror).

See the [media queries documentation](https://mikke89.github.io/RmlUiDoc/pages/rcss/media_queries.html) for usage details and supported media features.

### Improved high DPI support

- Media queries support with `resolution` feature to toggle styles and sprites based on DPI.
- Spritesheets can now define the target DPI scaling by using the `resolution` property.
- Sprites can now be overrided by later `@spritesheet` rules, making it easy to eg. define high DPI versions of sprites.
- Sprites in decorators and `<img>` elements now scale according to the targeted DPI.
- Decorators and `<img>` elements now automatically update when sprites or DPI changes.
- The samples and shell now implement high dpi support (only on Windows 10 for now).

### New RML elements

- Added [Lottie plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/lottie.html) for displaying vector animations using the `<lottie>` element [#134](https://github.com/mikke89/RmlUi/pull/134) (thanks @diamondhat).
- Added [SVG plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/svg.html) for displaying vector images using the `<svg>` element.
- Added `<label>` element for associating a caption with an input element.

### Element improvements

- Implemented `Element::QuerySelector`, `Element::QuerySelectorAll`, and `Element::Closest`. [#164](https://github.com/mikke89/RmlUi/pull/164) (thanks @Dakror).
- The `tab-index: auto` property can now be set on the `<body>` element to enable tabbing back to the document.
- `<select>` elements now react to changes in the `value` attribute.
- Element attributes can now use escaped RML characters, eg. `<p example="&quot;Quoted text&quot;"/>`. [#154](https://github.com/mikke89/RmlUi/pull/154) (thanks @actboy168).
- Tabs and panels in tab sets will no longer set the `display` property to `inline-block`, thus it is now possible to customize the display property.
- `<progress>` element now supports the `max` attribute. Changing its `fill-image` property now actually updates the image.

### Input handling

- Added `Context::IsMouseInteracting()` to determine whether the mouse cursor hovers or otherwise interacts with documents. [#124](https://github.com/mikke89/RmlUi/issues/124)
- Added boolean return values to `Context::ProcessMouse...()` functions to determine whether the mouse is interacting with documents.
- Fixed some situations where the return value of `Context::Process...()` was wrong.

### Lua plugin

Improved Lua plugin in several aspects.

- Added detailed [documentation for the Lua plugin](https://mikke89.github.io/RmlUiDoc/pages/lua_manual.html). [RmlUiDocs#4](https://github.com/mikke89/RmlUiDoc/pull/4) (thanks @IronicallySerious)
- Remove overriding globals, use standard pairs/ipars implementation, make array indices 1-based. [#95](https://github.com/mikke89/RmlUi/issues/95) [#137](https://github.com/mikke89/RmlUi/pull/137) (thanks @actboy168)
- Improve compatibility with debuggers and fix compatibility with Lua 5.4. [#136](https://github.com/mikke89/RmlUi/pull/136) [#138](https://github.com/mikke89/RmlUi/pull/138) (thanks @actboy168)
- Lua error reporting improvements, including stack trace. [#140](https://github.com/mikke89/RmlUi/pull/140) [#151](https://github.com/mikke89/RmlUi/pull/151) (thanks @actboy168)
- Add support for declaring data bindings. [#146](https://github.com/mikke89/RmlUi/pull/146)  [#150](https://github.com/mikke89/RmlUi/pull/150) (thanks @cloudwu and @actboy168)

### Samples

- New `databinding` sample for demonstrating data bindings.
- New `lottie` sample for demonstrating Lottie animations with the Lottie plugin.
- New `svg` sample for demonstrating the SVG plugin.
- The use of `datagrid` in sample projects has now been replaced with data bindings. This includes the `treeview` sample and the high scores document in the `invader` sample. Tutorials have not been updated.
- The options document in the `luainvader` sample now demonstrate data bindings combined with Lua scripts.
- Improved the SFML2 sample [#106](https://github.com/mikke89/RmlUi/pull/106) and [#103](https://github.com/mikke89/RmlUi/issues/103) (thanks @hachmeister).

### Other features and improvements

- Added `Rml::GetTextureSourceList()` function to list all image sources loaded in all documents. [#131](https://github.com/mikke89/RmlUi/issues/131)
- Added `ElementDocument::ReloadStyleSheet()` function to reload styles without affecting the document tree. [#159](https://github.com/mikke89/RmlUi/pull/159) [#171](https://github.com/mikke89/RmlUi/pull/171) (thanks @Lyatus and @Dakror)
- RCSS and scripts are now always loaded in declared order [#144](https://github.com/mikke89/RmlUi/pull/144) (thanks @actboy168).
- A custom configuration can now be used by RmlUi. In this way it is possible to replace several types including containers to other STL-compatible containers (such as [EASTL](https://github.com/electronicarts/EASTL)), or to STL containers with custom allocators. See the `CUSTOM_CONFIGURATION` [CMake option](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/building_with_cmake.html#cmake-options). [#110](https://github.com/mikke89/RmlUi/pull/110) (thanks @rokups).
- Added ability to change the default base tag in documents [#112](https://github.com/mikke89/RmlUi/pull/112)  (thanks @aquawicket).
- Debugger improvements: Sort property names alphabetically. Fix a bug where the outlines would draw underneath the document.
- Improved performance when using fonts with kerning.
- Added `unsigned int` and `uint64_t` to Variant. [#166](https://github.com/mikke89/RmlUi/pull/166) [#176](https://github.com/mikke89/RmlUi/pull/176) (thanks @Omegapol and @Dakror).
- Sprite sheets can now be declared anonymous, no name needed.

### Bug fixes

- Fix some situations where `text-decoration` would not be rendered. [#119](https://github.com/mikke89/RmlUi/issues/119).
- Fix a bug where font textures were leaked on `Rml::Shutdown()`. [#133](https://github.com/mikke89/RmlUi/issues/133)
- Fixed building with MinGW, and added it to the CI to avoid future breaks. [#108](https://github.com/mikke89/RmlUi/pull/108) (thanks @cloudwu).
- Fixed building when targeting Windows UWP apps. [#178](https://github.com/mikke89/RmlUi/pull/178) (thanks @rokups).
- Fixed several compilation issues and warnings. [#118](https://github.com/mikke89/RmlUi/issues/118) [#97](https://github.com/mikke89/RmlUi/pull/97) [#157](https://github.com/mikke89/RmlUi/pull/157) (thanks @SpaceCat-Chan and @LWSS).
- Fixed a bug where the `transition: all ...;` property would not parse correctly.
- Fix \<textarea\> getting an unnecessary horizontal scrollbar. [#122](https://github.com/mikke89/RmlUi/issues/122)
- Fix text position changing in input fields when selecting text and font has kerning.
- Fix text-decoration not always being regenerated. [#119](https://github.com/mikke89/RmlUi/issues/119)
- Fix tabbing navigation when tabable elements are direct children of body.
- Fix tabbing navigation in reverse direction from body.
- Fix missing attributes when cloning elements. [#177](https://github.com/mikke89/RmlUi/pull/177) (thanks @Dakror).

### Restructuring RmlUi

RmlUi has been restructured to simplify its usage. This involves breaking changes but should benefit everyone using the library in the future. See discussion in [#58](https://github.com/mikke89/RmlUi/issues/58).

- The old `Controls` plugin is now gone. But fear not! It has been merged into the `Core` project.
- The old `Rml::Core` and `Rml::Controls` namespaces have been removed, their contents are now located directly in the `Rml` namespace.
- The old `Controls` public header files have been moved to `<RmlUi/Core/Elements/...>`.
- The old `Controls` source files and private header files have been moved to `Source/Core/Elements/...`.
- The `Debugger` plugin remains as before at the same location and same namespace `Rml::Debugger`.

The Lua plugins have been changed to reflect the above changes.

- The old Lua plugins `RmlCoreLua` and `RmlControlsLua` have been merged into a single library `RmlLua`.
- The public header files are now located at `<RmlUi/Lua/...>`.
- The Lua plugin is now initialized by calling `Rml::Lua::Initialise()` located in `<RmlUi/Lua/Lua.h>`.
- Separated the Lua interpreter functions from initialization and the Lua plugin.
- Renamed macros in the Lua plugin, they now start with `RMLUI_`.

#### Upgrade guide

- Remove the call to `Rml::Controls::Initialise()`, this is no longer needed.
- Replace all inclusions of `<RmlUi/Controls.h>` with `<RmlUi/Core.h>` unless it is already included, or include individual header files.
- Rename all inclusions of `<RmlUi/Controls/...>` to `<RmlUi/Core/Elements/...>`.
- Replace all occurrences of `Rml::Core` with `Rml`.
- Replace all occurrences of `Rml::Controls` with `Rml`.
- Look for forward declarations in `namespace Rml { namespace Core { ... } }` and `namespace Rml { namespace Controls { ... } }`. Replace with `namespace Rml { ... }`.
- Remove the linkage to the `RmlControls` library.
- For users of the Lua plugin:
  - Replace RmlUi's Lua header files with `<RmlUi/Lua.h>` or individual header files in `<RmlUi/Lua/...>`.
  - Replace the old initialization calls with `Rml::Lua::Initialise()`. Previously this was `Rml::Core::Lua::Interpreter::Initialise()` and `Rml::Controls::Lua::RegisterTypes(...)`.
  - Link with the library `RmlLua`, remove `RmlCoreLua` and `RmlControlsLua`.
- Welcome to RmlUi 4.0 :)

#### Related internal changes.

- Refactored the two `WidgetSlider` classes to avoid duplicate names in Core and Controls.
- Refactored `TransformPrimitive.h` by moving utility functions that should only be used internally to an internal header file.
- Renamed header guard macros.

### Deprecated functionality

- The `datagrid` element and related functionality has been deprecated in favor of [data bindings](https://mikke89.github.io/RmlUiDoc/pages/data_bindings.html) and [RCSS tables](https://mikke89.github.io/RmlUiDoc/pages/rcss/tables.html).
- The `<progressbar>` tag name has been deprecated in favor of `<progress>`. For now they work identically, but usage of `<progressbar>`  will raise a warning, expect future removal of this tag.

### Breaking changes

- Namespaces and plugin names changed! See the restructuring changes above.
- Using `{{` and `}}` inside RML documents is now reserved for usage with data bindings.
- Attributes starting with `data-` are now reserved for RmlUi.
- The changes to the layout engine may result in changes to the rendered layout in some situations, see above for more details.
- The `BaseXMLParser` class has some minor interface changes.
- Tab set elements `tab` and `panel` should now have their `display` property set in the RCSS document, use `display: inline-block` for the same behavior as before.
- For custom, replaced elements: `Element::GetIntrinsicDimensions()` now additionally takes an intrinsic ratio parameter.
- The `fill-image` property should now be applied to the `<progress>` element instead of its inner `<fill>` element.
- The function `ElementDocument::LoadScript` is now changed to handle internal and external scripts separately. [#144](https://github.com/mikke89/RmlUi/pull/144)
- For custom decorators: Textures from filenames should now first be loaded through the `DecoratorInstancerInterface` and then submitted to the `Decorator`.


## RmlUi 3.3

###  Rml `select` element improvements

- Prevent scrolling in the parent window when scrolling inside the selection box.
- Close the selection box when scrolling in the parent document.
- The selection box will now limit its height to the available space within the context's window dimensions, and position itself either below or above the `select` element as appropriate. [#91](https://github.com/mikke89/RmlUi/issues/91)

### Cleaning up header files

An effort has been made for header files to include what they use, and nothing else. This effort has measurably improved compile times, especially when not using precompiled headers.

This change also makes it easier to include only parts of the library headers in the user application for improved compile times. That is, instead of including the whole core library using `#include <RmlUi/Core.h>`, one can specify which ones are needed such as `#include <RmlUi/Core/Element.h>`.

### CMake precompiled header support

The library now makes use of CMake's precompiled header support (requires CMake 3.16 or higher), which can optionally be disabled. In Visual Studio, compilation times are improved by almost 50% when enabled.

### Changes

- The `style` attribute no longer requires a semi-colon `;` after the final property.
- The sample projects now universally use the `F8` key to toggle the RmlUi debugger on all platforms.
- Add an upper limit to the number of possible custom properties and events. This change will reduce the number of dynamic allocations in some cases.
- Build improvements and several warnings fixed. Compiles cleanly with `-Wall -Wextra` and on MSVC with `/W4`.
- The sample projects now find their assets when building and running the sample with Visual Studio's native CMake support and default settings. This also applies when targeting Windows Subsystem for Linux (WSL).
- The mouse cursor API is now implemented on the X11 shell.
- RmlUi is now C++20 compatible (C++14 is still the minimum requirement).

### Bug fixes

- Fix font textures not released when calling Core::ReleaseTextures [#84](https://github.com/mikke89/RmlUi/issues/84).
- Re-implement `Rml::Core::ReleaseCompiledGeometries()` [#84](https://github.com/mikke89/RmlUi/issues/84).
- Property `white-space: nowrap` no longer disables horizontal scrollbars on overflow [#94](https://github.com/mikke89/RmlUi/issues/94).
- Changes to font effects are now properly applied whenever the `font-effect` property is changed [#98](https://github.com/mikke89/RmlUi/issues/98).
- Fix structural pseudo-selectors only being applied if written with parenthesis [#30](https://github.com/mikke89/RmlUi/issues/30#issuecomment-597648310).


## RmlUi 3.2

### Animating keyword properties

Keyword properties can now be animated. Keywords are always interpolated in discrete steps, and normally applied half-way to the next keyframe. The single exception to this rule is for the `visibility` property. As in the CSS specifications, this property always applies the `visible` keyword during transition when present either in the previous or next keyframe.

Thus, the following can produce a fade-out animation, removing visibility of the element at animation end (thanks to @uniquejack for the example).
```css
@keyframes fadeout {
	from {
		opacity: 1;
	}
	to {
		opacity: 0;
		visibility: hidden;
	}
}
.fadeout {
	animation: 1.2s cubic-in fadeout;
}
```

### Changes

- Animated properties are now removed when an animation completes.
- Update robin_hood unordered_map to 3.5.0 (thanks @jhasse). [#75](https://github.com/mikke89/RmlUi/issues/75)

### Bug fixes

- Range input element: Change event reports the correct value instead of normalized (thanks @andreasschultes). [#72](https://github.com/mikke89/RmlUi/issues/72) [#73](https://github.com/mikke89/RmlUi/issues/73).
- Fix wrong cast in elapsed time in SDL2 sample. [#71](https://github.com/mikke89/RmlUi/issues/71).
- Avoid infinite recursion on Variant construction/assignment with unsupported types. [#70](https://github.com/mikke89/RmlUi/issues/70).
- Fix warnings issued by the MinGW compiler (thanks @jhasse).


## RmlUi 3.1

### Progress bar

A new `progressbar` element is introduced for visually displaying progress or relative values. The element can take the following attributes.

- `value`. Number `[0, 1]`. The fraction of the progress bar that is filled where 1 means completely filled.
- `direction`. Determines the direction in which the filled part expands. One of:
   - `top | right (default) | bottom | left | clockwise | counter-clockwise`
- `start-edge`. Only applies to 'clockwise' or 'counter-clockwise' directions. Defines which edge the
circle should start expanding from. Possible values:
   - `top (default) | right | bottom | left`

The element is only available with the `RmlControls` library.

**Styling**

The progressbar generates a non-dom `fill` element beneath it which can be used to style the filled part of the bar. The `fill` element can use normal properties such as `background-color`, `border`, and `decorator` to style it, or use the new `fill-image`-property to set an image which will be clipped according to the progress bar's `value`. 

The `fill-image` property is the only way to style circular progress bars (`clockwise` and `counter-clockwise` directions). The `fill` element is still available but it will always be fixed in size independent of the `value` attribute.

**New RCSS property**

- `fill-image`. String, non-inherited. Must be the name of a sprite or the path to an image.

**Examples**

The following RCSS styles three different progress bars.
```css
@spritesheet progress_bars
{
	src: my_progress_bars.tga;
	progress:        103px 267px 80px 34px;
	progress-fill-l: 110px 302px  6px 34px;
	progress-fill-c: 140px 302px  6px 34px;
	progress-fill-r: 170px 302px  6px 34px;
	gauge:      0px 271px 100px 86px;
	gauge-fill: 0px 356px 100px 86px;
}
.progress_horizontal { 
	decorator: image( progress );
	width: 80px;
	height: 34px;
}
.progress_horizontal fill {
	decorator: tiled-horizontal( progress-fill-l, progress-fill-c, progress-fill-r );
	margin: 0 7px;
	/* padding ensures that the decorator has a minimum width when the value is zero */
	padding-left: 14px;
}
.progress_vertical {
	width: 30px;
	height: 80px;
	background-color: #E3E4E1;
	border: 4px #A90909;
}
.progress_vertical fill {
	border: 3px #4D9137;
	background-color: #7AE857;
}
.gauge { 
	decorator: image( gauge );
	width: 100px;
	height: 86px;
}
.gauge fill { 
	fill-image: gauge-fill;
}
```
Now, they can be used in RML as follows.
```html
<progressbar class="progress_horizontal" value="0.75"/>
<progressbar class="progress_vertical" direction="top" value="0.6"/>
<progressbar class="gauge" direction="clockwise" start-edge="bottom" value="0.3"/>
```


### New font effects

Two new font effects have been added.

**Glow effect**

Renders a blurred outline around the text. 

The `glow` effect is declared as:
```css
font-effect: glow( <width-outline> <width-blur> <offset-x> <offset-y> <color> );
```
Both the outline pass and the subsequent blur pass can be controlled independently. Additionally, an offset can be applied which makes the effect suitable for generating drop shadows as well.

**Blur effect**

Renders a Gaussian blurred copy of the text.

The `blur` effect is declared as:
```css
font-effect: blur( <width> <color> );
```
Note that, the blur effect will not replace the original text. To only show the blurred version of the text, set the `color` property of the original text to `transparent`.

**Example usage**

```css
/* Declares a glow effect. */
h1
{
	font-effect: glow( 3px #ee9 );
}

/* The glow effect can also create nice looking shadows. */
p.glow_shadow
{
	color: #ed5;
	font-effect: glow(2px 4px 2px 3px #644);
}

/* Renders a blurred version of the text, hides the original text. */
h1
{
	color: transparent;
	font-effect: blur(3px #ed5);
}
```

See the `demo` sample for additional usage examples and results.



### CMake options

New CMake option added.

- `DISABLE_RTTI_AND_EXCEPTIONS` will try to configure the compiler to disable RTTI language support and exceptions. All internal use of RTTI (eg. dynamic_cast) will then be replaced by a custom solution. If set, users of the library should then `#define RMLUI_USE_CUSTOM_RTTI` before including the library.



### Breaking changes

- For users that implement custom font effects, there are some minor changes to the font effect interface and the convolution filter.



## RmlUi 3.0


RmlUi 3.0 is the biggest change yet, featuring a substantial amount of new features and bug fixes. One of the main efforts in RmlUi 3.0 has been on improving the performance of the library. Users should see a noticable performance increase when upgrading.


### Performance

One of the main efforts in RmlUi 3.0 has been on improving the performance of the library. Some noteable changes include:

- The update loop has been reworked to avoid doing unnecessary, repeated calculations whenever the document or style is changed. Instead of immediately updating properties on any affected elements, most of this work is done during the Context::Update call in a more carefully chosen order. Note that for this reason, when querying the Rocket API for properties such as size or position, this information may not be up-to-date with changes since the last Context::Update, such as newly added elements or classes. If this information is needed immediately, a call to ElementDocument::UpdateDocument can be made before such queries at a performance penalty.
- Several containers have been replaced, such as std::map to [robin_hood::unordered_flat_map](https://github.com/martinus/robin-hood-hashing).
- Reduced number of allocations and unnecessary recursive calls.
- Internally, the concept of computed values has been introduced. Computed values take the properties of an element and computes them as far as possible without introducing the layouting.

All of these changes, in addition to many smaller optimizations, results in a more than **25x** measured performance increase for creation and destruction of a large number of elements. A benchmark is included with the samples.


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

- `<src>`: image source url or sprite name
- `<orientation>`: none (default) \| flip-horizontal \| flip-vertical \| rotate-180
- `<fit>`: fill (default) \| contain \| cover \| scale-none \| scale-down
- `<align-x>`: left \| center (default) \| right \| \<length-percentage\>
- `<align-y>`: top \| center (default) \| bottom \| \<length-percentage\>

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
- Can toggle whether elements are selected in user context.
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
- Transform rotations can now be interpolated without decomposing when their rotation axes align. When the axes do not align, interpolation will be performed via decomposition (quaternion interpolation) as before. With this addition, transform interpolation should be fully compatible with the CSS specifications.
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


### Events

There are some changes to events in RmlUi, however, for most users, existing code should still work as before.

There is now a distinction between actions executed in event listeners, and default actions for events:

- Event listeners are attached to an element as before. Events follow the normal phases: capture (root -> target), target, and bubble (target -> root). Each event listener is always attached to the target phase, and is additionally attached to either the bubble phase (default) or capture phase. Listeners are executed in the order they are added to the element. Each event type specifies whether it executes the bubble phase or not, see below for details.
- Default actions are primarily for actions performed internally in the library. They are executed in the function `virtual void Element::ProcessDefaultAction(Event& event)`. However, any object that derives from `Element` can override the default behavior and add new behavior. The default actions are always executed after all event listeners, and only propagated according to the phases set in their `default_action_phase` value which is defined for each event type. If an event is interrupted with `Event::StopPropagation()`, then the default actions are not performed.


Each event type now has an associated EventId as well as a specification defined as follows:

- `interruptible`: Whether the event can be cancelled by calling `Event::StopPropagation()`.
- `bubbles`: Whether the event executes the bubble phase. If true, all three phases: capture, target, and bubble, are executed. If false, only capture and target phases are executed.
- `default_action_phase`: One of: None, Target, TargetAndBubble. Specifies during which phases the default action is executed, if any. That is, the phase for which `Element::ProcessDefaultAction()` is called. See above for details.

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
- All event listeners on the current element will always be called after calling `StopPropagation()`. When propagating to the next element, the event is stopped. This behavior is consistent with the standard DOM events model. The event can be stopped immediately with `StopImmediatePropagation()`.
- `Element::DispatchEvent` can now optionally take an `EventId` instead of a `String`.
- The `resize` event now only applies to the document size, not individual elements.
- The `scrollchange` event has been replaced by a function call. To capture scroll changes, instead use the `scroll` event.
- The `textinput` event now sends a `String` in UTF-8 instead of a UCS-2 character, possibly with multiple characters. The parameter key name is changed from "data" to "text".


### Other features

- `Context::ProcessMouseWheel` now takes a float value for the `wheel_delta` property, thereby enabling continuous/smooth scrolling for input devices with such support. The default scroll length for unity value of `wheel_delta` is now three times the default line-height multiplied by the current dp-ratio.
- The system interface now has two new functions for setting and getting text to and from the clipboard: `virtual void SystemInterface::SetClipboardText(const Core::String& text)` and `virtual void SystemInterface::GetClipboardText(Core::String& text)`.
- The `text-decoration` property can now also be used with `overline` and `line-through`.
- The text input and text area elements can be navigated and edited word for word by holding the Ctrl key. Can now also navigate by using Ctrl+Home/End and Page up/down. Furthermore, select all by Ctrl+A and select word by double click.
- Double clicks are now submitted only when they're inside a small radius of the first click.
- The `<img>` element can now take sprite names in the `sprite` attribute. For images the `src` attribute can be used as before.
- The `sliderbar` on the `range` input element can now use margins to offset it from the track.


### Breaking changes

Breaking changes since RmlUi 2.0.

- RmlUi now requires a C++14-compatible compiler (previously C++11).
- Rml::Core::String has been replaced by std::string, thus, interfacing with the library now requires you to change your string types. This change was motivated by a small performance gain, additionally, it should make it easier to interface with the library especially for users already using std::string in their codebase. Furthermore, strings should be considered as encoded in UTF-8.
- To load fonts, use `Rml::Core::LoadFontFace` instead of `Rml::Core::FontDatabase::LoadFontFace`.
- Querying the property of an element for size, position and similar may not work as expected right after changes to the document or style. This change is made for performance reasons, see the description under *performance* for reasoning and a workaround.
- The Controls::DataGrid "min-rows" property has been removed.
- Removed RenderInterface::GetPixelsPerInch, instead the pixels per inch value has been fixed to 96 PPI, as per CSS specs. To achieve a scalable user interface, instead use the 'dp' unit.
- The `<img>` element's `coords` attribute is now replaced by a `rect` attribute specified like for sprites.
- Removed 'top' and 'bottom' from z-index property.
- Angles need to be declared in either 'deg' or 'rad'. Unit-less numbers do not work.
- See changes to the declaration of decorators and font-effects above.
- See changes to the render interface regarding transforms above.
- See changes to the event system above.
- The focus flag in `ElementDocument::Show` has been changed, with a new enum name and new options, see above.
- The tiled decorators (`image`, `tiled-horizontal`, `tiled-vertical`, and `tiled-box`) no longer support the old repeat modes.
- Also, see removal of manual reference counting above.




## RmlUi 2.0

RmlUi 2.0 is the first release after the [original libRocket branch](https://github.com/libRocket/libRocket).

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


### Various features

- The slider on the `input.range` element can be dragged from anywhere in the element.
- The `:checked` pseudo class can be used to style the selected item in drop-down lists.


### Breaking changes

- The namespace has changed from `Rocket` to `Rml`, include path from `<Rocket/...>` to `<RmlUi/...>`, and macro prefix from `ROCKET_` to `RMLUI_`.
- `Rml::Core::SystemInterface::GetElapsedTime()` now returns `double` instead of `float`.
```cpp
virtual double GetElapsedTime();
```
- The `font-size` property no longer accepts a unit-less `<number>`, instead add the `px` unit for equivalent behavior. The new behavior is consistent with CSS.
- The old functionality for setting and drawing mouse cursors has been replaced by a new function call to the system interface, thereby allowing the user to set the system cursor.
- Python support has been removed.
