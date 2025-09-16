* [RmlUi 6.1](#rmlui-61)
* [RmlUi 6.0](#rmlui-60)
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

## RmlUi 6.1

### Prevent single pixel gaps between elements

This release addresses the issue of 1px gaps appearing between fractionally sized elements when placed border-to-border. This was particularly pronounced in DPI-scaled layouts, as that often leads to fractionally sized elements.

The solution involves rounding the rendered size of elements based on their absolute positions to ensure that the bottom/right of one element matches the top/left of the next element. This implies that the rendered size of a fractional element may vary by up to one pixel. This generally matches how web browsers behave. Floating-point precision issues may still cause rare gaps, but the improvements should cover almost all cases. See the [commit message](https://github.com/mikke89/RmlUi/commit/b197f985b328d5493af3190e27d4290bb496ff1d) for details. Resolves #438, thanks to @mwl4 for the extensive initiative and proof of concept.

In particular, this fixes several situations with single pixel gaps and overlaps:

- Gap of 1px between border or backgrounds of neighboring elements.
- Overlap of 1px between border or backgrounds of neighboring elements.
- Table cell backgrounds overlap the table border by 1px.
- Gap between nested elements in a flex container.
- Clipping area offset by 1px compared to the border area.

![Single pixel gap fix examples - before and after comparisons](https://github.com/user-attachments/assets/f1b29382-4686-4fea-a4dc-ea9628669b80)

### Handle element

The `<handle>` element has received several major improvements.

- The handle now retains the anchoring that applies to the target element, even after moving or sizing it. #637
  - If an element has all of its inset (top/right/bottom/left) properties set, this determines the size, and anchors to all edges. Previously, we would break the anchoring and just declare its new position or size. Now, positioning and sizing is performed in a way that retains this anchoring. Similarly, this applies to every combination of anchoring.
  - Thus, when first sizing and moving the target and then resizing its container, the element can now still resize itself to match the new dimensions.
- The `edge_margin` attribute is introduced to constrain the target placement to the edges of its containing block. #631 
  - Applies to both position and size targets.
  - This attribute can take any length or percentage, which specifies the minimum distance between the target and the edges of its containing block. Each side can be specified individually, and negative values are allowed. See the [documentation](https://mikke89.github.io/RmlUiDoc/pages/rml/controls.html#handle) for details.
  - Defaults to `0px`, which means that handle targets will now be constrained exactly to the edges of their containing block.
- Fix several issues where the element jolts some distance at drag start:
  - When the target's containing block has a border.
  - When the target is set to relative positioning and offset from the top-left corner.

### New decorator: `text`

Implement a new decorator to render text as a background on elements. This can be particularly helpful when using icon fonts, and even allows using such fonts for generated elements. #348 #655 #679

```
decorator: text("Hello 🌎 world!" blue bottom right);
```

The font face will be inherited from the element it is being applied to. However, it can be colored independently. Further, the text can be freely aligned within the element using lengths, percentages, or keywords. Unicode numerical references are supported with the HTML syntax, e.g. `&#x1F30E;`.

![Text decorator examples](https://github.com/mikke89/RmlUiDoc/blob/b050d5d0b316c961cd05ed37cdd3dda1b809d80e/assets/images/decorators/text.png)

### Flexbox layout improvements

- Apply automatic minimum size of flex items in column mode with auto size. #658
- Performance improvement: Skip calculating hypothetical cross size when not needed. Avoids a potentially expensive formatting step in some situations. #658 
- Fix the hypothetical width of replaced elements (such as images) in column direction layout. #666
- Fix hitting an assertion due to negative flex item size in some situations when the edge size is fractional. #657

### Data binding

- Allow custom getter/setter on scoped enum. #693 #699 (thanks @AmaiKinono)
- Ternary expressions are now implemented with jumps so that only one branch is evaluated. This makes it possible to e.g. avoid invalid array access in case of an empty array. #740 (thanks @rminderhoud and @exjam)
- Fix an issue where the `FamilyId` would have the same value for different types across shared library boundaries, which could lead to a crash or other unexpected behavior. 

### Animations

- Add interpolation of color stop lists, which enables animation of color and position of stops in gradient decorators. #667
- Improve warning message when trying to animate box shadows. #688

### RCSS Values

- Support `hsl` and `hsla` colors. E.g. `color: hsl(30, 80%, 50%)`. #674 (thanks @AmaiKinono)

### Input elements

- Implement the ability to style the progress of a `range` input. #736 (thanks @viseztrance)
  - A new [`sliderprogress` child element](https://mikke89.github.io/RmlUiDoc/pages/style_guide.html) is added for this purpose.\
  ![Range input with styled progress bar](https://github.com/user-attachments/assets/aa1ecea7-6fc1-4bc5-99a0-5bbc969e190e)
- Improve navigation of `<select>` elements when using controller/keyboard navigation. #565 #566 (thanks @Paril)
  - Scroll to the selected options as one is moving up or down the list.
  - Scroll to the selected option when opening up the selection box.
  - Add the ability to programmatically [show or hide](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/element_packages/form.html#drop-down-select-box) the selection box.
- Fix some layout and behavior issues of the `<select>` element. 
  - Fix issues related to specifying the height of the select arrow element. 
  - Fix an issue where the selection box would scroll to the top-left corner when the document layout is updated.
- Fix an issue where wrapping a `<select>` element inside a `<label>` element would prevent mouse clicks from being able to select a new option. #494
- Fix an issue where the contents of the `<input type="text">` and `<textarea>` elements could sometimes inadvertently scroll to a new place after a layout update.
- Handle multi-byte characters in `<input type="password">` fields. #735

### Elements

- Add support for the [`:scope` pseudo selector](https://mikke89.github.io/RmlUiDoc/pages/rcss/selectors.html#pseudo-selectors) when calling into the `Element` DOM query methods, i.e. `Element::QuerySelector[All]`, `Element::Matches`, and `Element::Closest`. #578 (thanks @Paril)
- Add [`Element::Contains` DOM method](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/elements.html#dom-interface).
- Allow `Element::ScrollIntoView` to only scroll in the nearest scroll container, instead of all ancestor scroll containers, by using the new `ScrollParentage::Closest` scroll option.
- Fix an issue where scrollbars could appear or disappear one frame after they should have changed visibility.

### Documents

- Expose `ElementDocument::FindNextTabElement` publicly.
- Disallow focusing into an unloaded document to prevent a potential situation with dangling pointers. #730

### Font engine

- Add the ability to select a font face from a font collection, using its face index passed to `Rml::LoadFontFace`. #720 (thanks @leduyquang753)
- Fix rare placement of glyphs appearing below the baseline in some fonts, by using the bitmap bearing instead of the glyph metrics.
- The [HarfBuzz font engine](./Samples/basic/harfbuzz) now uses kerning from HarfBuzz instead of FreeType. #639 (thanks @TriangulumDesire)

### RML Parsing and layouting

- Fix RML parsing of extra hyphen in closing comment, i.e. `--->` instead of `-->`. #681
- Fix a crash during layouting with word break enabled, when the first character of a token is multi-byte and does not fit on the line. #753 (thanks @and3md)

### Rendering

- Fix incorrect clipping when using multiple contexts of different dimensions. #677 #680 (thanks @s1sw)
- Defer texture loading until the texture becomes visible.

### Backends

- Update the SFML backend to support SFML 3, in addition to the existing SFML 2 support.
  - By default, SFML 3 is preferred before SFML 2 during CMake configuration. To override the automatic selection, set the CMake variable `RMLUI_SFML_VERSION_MAJOR` to the desired version (2 or 3).
- Update all SDL backends to support SDL 3, in addition to the existing SDL 2 support.
  - By default, SDL 3 is preferred before SDL 2 during CMake configuration. To override the automatic selection, set the CMake variable `RMLUI_SDL_VERSION_MAJOR` to the desired version (2 or 3).
- SDL 3-specific improvements:
  - Enable high DPI support.
  - Enable positioning of the input method editor (IME) to the text cursor.
- Improvements to both SDL 2 and SDL 3:
  - Keyboard is activated and deactivated when focusing text input fields.
  - Text input events are only submitted when text input fields are focused.
- `SDL_GL2`-specific improvements:
  - GLEW is no longer required, and no longer linked to.
  - Use OpenGL directly instead of the SDL renderer, just like the `SDL_GL3` renderer.
- OpenGL 3 renderer-specific improvements:
  - Added the ability to set an offset with the call to `SetViewport()`. #724 (thanks @viseztrance)
  - Added `RMLUI_NUM_MSAA_SAMPLES` as a customizable macro for the number of MSAA samples to use in RmlUi framebuffers.
  - Added utility functions `GetTransform()` and `ResetProgram()` to more easily enable client projects to render with their own shaders.

### Plugins

- Log warnings when SVG or Lottie files cannot be rendered. #687
- Support for LunaSVG 3.0 with the SVG plugin.

### Unit testing

- Enable shell renderer with environment variable `RMLUI_TESTS_USE_SHELL=1` instead of a compile definition.

### Resource management

- Avoid memory allocations during global initialization. #689
  - Instead, explicitly start lifetime of globals during the call to `Rml::Initialise`.
  - Thus, there should no longer be any memory allocations occurring before `main()` when linking to RmlUi.
  - We now give a warning if there are objects in user space that refer to any RmlUi resources at the end of `Rml::Shutdown`, as this prevents the library from cleaning up memory pools.
    - We make an exemption for `Rml::EventListener` as those are commonly kept around until after `Rml::Shutdown` which is considered reasonable.
- Add manual release of render managers, `Rml::ReleaseRenderManagers`, to allow the render interface to be destroyed before `Rml::Shutdown`. #703

### Building

- Remove `OpenGL::GL` dependency for GL3 backends. #684 (thanks @std-microblock)
- Fix dependency check signature in RmlUiConfig causing failure to find dependencies. #721 #722 (thanks @mpersano) 
- Log to console by default when building on MinGW. #757 (thanks @trexxet)
- Fix a missing header include in the GL3 renderer, causing a compilation error on Visual Studio 17.12.
- Fix a build issue on certain Visual Studio 2017 setups by using `std::enable_if_t` consistently. #734
- Fix a build issue on Android with C++ 23 enabled due to mismatching std-namespace usage and C vs. C++ math headers.
- Fix unit tests and missing sample data when building with Emscripten.
- Libraries and archives will now be placed in the top-level binary directory, unless overridden by users or parent projects. This matches the existing runtime output directory.

### Readme

- Improve readme code examples. #683 (thanks @std-microblock)

### Breaking changes

- Layouts may see 1px shifts in various places due to the improvements to prevent single pixel gaps.
- The target of the `<handle>` element will no longer move outside its containing block by default, see above for how to override this behavior.
- Changed the signature of `MeshUtilities::GenerateBackground` and  `MeshUtilities::GenerateBackgroundBorder`.
  - They now take the new `RenderBox` class as input. The `Element::GetRenderBox` method can be used to construct it.
- Changed `ComputedValues::border_radius` to return an array instead of `Vector4f`.
- `Rml::ReleaseMemoryPools` is no longer exposed publicly. This function is automatically called during shutdown and should not be used manually.
- SDL backends: The SDL platform's `InputEventHandler` function now takes an additional parameter `window`. 


## RmlUi 6.0

* [Advanced rendering features](#advanced-rendering-features)
  * [New features](#new-features)
  * [Screenshots](#screenshots)
  * [Major overhaul of the render interface](#major-overhaul-of-the-render-interface)
  * [Backward compatible render interface adapter](#backward-compatible-render-interface-adapter)
  * [Render manager and resources](#render-manager-and-resources)
  * [Limitations](#limitations)
* [Major layout engine improvements](#major-layout-engine-improvements)
  * [Detailed layout improvements](#detailed-layout-improvements)
  * [Layout comparisons](#layout-comparisons)
  * [General layout improvements](#general-layout-improvements)
* [CMake modernization](#cmake-modernization)
  * [New target names](#new-target-names)
  * [New library filenames](#new-library-filenames)
  * [New option names](#new-option-names)
  * [New exported definitions](#new-exported-definitions)
  * [CMake presets](#cmake-presets)
* [Spatial navigation](#spatial-navigation)
* [Text shaping and font engine](#text-shaping-and-font-engine)
* [Elements](#elements)
* [Text input widget](#text-input-widget)
* [Utilities](#utilities)
* [Data bindings](#data-bindings)
* [Debugger plugin](#debugger-plugin)
* [Lua plugin](#lua-plugin)
* [System interface](#system-interface)
* [General improvements](#general-improvements)
* [General fixes](#general-fixes)
* [Build improvements](#build-improvements)
* [Backends](#backends)
* [Breaking changes](#breaking-changes)

### Advanced rendering features

This one has been a long time in the making, now the time has come for one of the biggest additions to the library. Advanced rendering effects are now available, including filters with blur support, box-shadow, advanced gradients, shaders, masks, and clipping of rounded borders.

The original issue is found in #307 and the pull request is #594. Thanks to everyone who provided feedback. Resolves #249, #253, #307, #597, and even addresses #1.

#### New features

New properties:

- `filter`:  Apply a rendering effect to the current element (including its children).
  - Supported filters: `blur`, `drop-shadow`, `hue-rotate`, `brightness`, `contrast`, `grayscale`, `invert`, `opacity`, `sepia`. That is, all filters supported in CSS.
- `backdrop-filter`: Apply a filter to anything that is rendered below the current element.
- `mask-image`: Can be combined with any decorator, including images and gradients, to mask out any part of the current element (and its children) by multiplying their alpha channels.
- `box-shadow`: With full support for offset, blur, spread, and insets.

New decorators:

- `shader`: A generic decorator to pass a string to your renderer.
- `linear-gradient`, `repeating-linear-gradient`
- `radial-gradient`, `repeating-radial-gradient`
- `conic-gradient`, `repeating-conic-gradient`

The gradients support most of the CSS features and syntax, including angle and `to <direction>` syntax for direction, multiple color stops, locations in length or percentage units, and up to two locations per color. Please see the [decorators documentation](https://mikke89.github.io/RmlUiDoc/pages/rcss/decorators.html#rmlui-decorators) for details.

- The new rendering interface includes support for shaders, which enable the above decorators. Parsing is done in the library, but the backend renderer is the one implementing the actual shader code.

- All filters and gradient decorators have full support for interpolation, that is, they can be animated. This is not yet implemented for box shadows.

- Decorators can now take an extra keyword `<paint-area>` which is one of `border-box | padding-box | content-box`. The keyword indicates which area of the element the decorator should apply to. All built-in decorators are modified to support this property. For example: `decorator: linear-gradient(to top right, yellow, blue) border-box`.

- [Custom filters](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/filters.html#custom-filters) can be created by users by deriving from `Filter` and `FilterInstancer`, analogous to how custom decorators are created.

- Improved element clipping behavior. Handles more complicated cases, including nested transforms with hidden overflow, and clipping to the curved edge of elements with border-radius. This requires clip mask support in the renderer.

The [documentation](https://mikke89.github.io/RmlUiDoc/) has been updated to reflect the new features, including the new decorators and properties, with examples and screenshots. The new features are also demonstrated in the new `effects` sample, so please check that out.

For now, only the OpenGL 3 renderer implements all new rendering features. All other backends have been updated to work with the updated render interface but with their old feature set. For users with custom backends, please see the updated [render interface documentation](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/render.html) in particular. Here, you will also find a table detailing exactly which functions must be implemented to support particular RCSS features.

Here are some quick RCSS examples taken from the documentation.

```css
decorator: linear-gradient(to bottom, #00f3, #0001, #00f3), linear-gradient(to top right, red, blue);
decorator: radial-gradient(circle farthest-side at center, #ff6b6b, #fec84d, #4ecdc4);
decorator: repeating-conic-gradient(from 90deg, #ffd700, #f06, #ffd700 180deg);
decorator: shader("my_shader") border-box;

filter: brightness(1.2) hue-rotate(90deg) drop-shadow(#f33f 30px 20px 5px);
backdrop-filter: blur(10px);

box-shadow: #f008 40px 30px 0px 0px, #00f8 -40px -30px 0px 0px;
mask-image: conic-gradient(from 45deg, black, transparent, black), image("star.png" cover);
```

#### Screenshots

Collage of advanced effects: 

![Advanced effects demonstration](https://github.com/user-attachments/assets/71840d6f-903e-45fe-9e34-a02ed1ddae07)

Masking principles and demonstration:

![Mask imaging and demonstration](https://github.com/mikke89/RmlUiDoc/blob/3854487d65b94ddb6e932ae02f5cef85365003f6/assets/images/mask-image.png?raw=true)

Improved clipping behavior of nested and transformed elements (also showing improved layouting of positioned boxes):

![Clipping comparison of nested and transformed elements](https://github.com/user-attachments/assets/c0ce1123-8c9d-4fbf-8bab-41e63cc332c7)

Improved clipping with border-radius:

![Clipping comparison of elements with border-radius](https://github.com/user-attachments/assets/b944c69a-1b40-4932-9c62-2d4365154d5c)

Demonstration of the `effects` sample: 

[Effects sample video](https://github.com/mikke89/RmlUi/assets/5490330/bdc0422d-867d-4090-9d48-e7159e3adc18)

#### Major overhaul of the render interface

The render interface has been simplified to ease implementation of basic rendering functionality, while extended to enable the new advanced rendering effects. The new effects are fully opt-in, and can be enabled iteratively to support the features that are most desired for your project. See the updated [render interface documentation](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/render.html) for how to implement the new functionality. The documentation includes a table detailing which functions must be implemented to support specific RCSS features.

Highlighted changes:

- Now using safer, modern types (like span).
- A clear separation between required functions for basic functionality, and optional features for advanced rendering effects. The required functions are now pure virtual.
- All colors are now submitted as 8-bit sRGBA (like before), but with premultiplied alpha (new). Existing renderers should modify their blending modes accordingly. This change is central to correct blending of partially transparent layers.
- All geometry is now compiled before it can be rendered, which helps to simplify the interface.
  - Now the pointers to the geometry data (vertices and indices) are guaranteed to be available and immutable until the same geometry is released. Thus, users can simply store the views to this data, and reuse that during rendering, which should help considerably for users migrating from the immediate rendering function.
- The scissor region should now be applied separately from any active transform. Previously, we would have to manually redirect the scissor to a stencil operation, that is no longer the case. Instead, the clipping with transform is now handled by the library, and directed to the clip mask functionality of the render interface as appropriate.
- Expanded functionality to enable the new rendering effects, including layered rendering, rendering to texture, rendering with filters and shaders.
- Textures are no longer part of the compiled geometry, compiled geometry can be rendered with different textures or shaders.

#### Backward compatible render interface adapter

The render interface changes will require updates for all users writing their own render interface implementation. To smooth the transition, there is a fully backward-compatible adapter for old render interfaces, please see [RenderInterfaceCompatibility.h](Include/RmlUi/Core/RenderInterfaceCompatibility.h).

1. In your legacy RenderInterface implementation, derive from `Rml::RenderInterfaceCompatibility` instead of
   `Rml::RenderInterface`.
   ```cpp
       #include <RmlUi/Core/RenderInterfaceCompatibility.h>
       class MyRenderInterface : public Rml::RenderInterfaceCompatibility { ... };
	```
2. Use the adapted interface when setting the RmlUi render interface.
   ```cpp
       Rml::SetRenderInterface(my_render_interface.GetAdaptedInterface());
   ```

That's it, and your old renderer should now still work!

It can also be useful to take a closer look at the adapter before migrating your own renderer to the new interface, to see which changes are necessary. Naturally, this adapter won't support any of the new rendering features.

For demonstration purposes, there are two built-in backends implementing the adapter: [`BackwardCompatible_GLFW_GL2`](./Backends/RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL2.cpp) and [`BackwardCompatible_GLFW_GL3`](./Backends/RmlUi_BackwardCompatible/RmlUi_Renderer_BackwardCompatible_GL3.cpp). Each of the backends use a direct copy of their respective render interface from RmlUi 5.1, only with the above instructions applied. Please take a look if you want to see some examples on how to use this adapter.

#### Render manager and resources

A new RenderManager is introduced to manage resources and other rendering state. Users don't normally have to interact with this, but for contributors, and for more advanced usages, such as custom decorators, this implies several changes.

The RenderManager can be considered a wrapper around the render interface. All internal calls to the render interface should now go through this class.

Resources from the render interface are now wrapped as unique render resources, which are move-only types that automatically clean up after themselves when they go out of scope. This considerably helps resource management. This also implies changes to many central rendering types.

- `Mesh`: A new type holding indices and vertices. Can be constructed directly or from MeshUtilities (previously GeometryUtilities).
- `Geometry`: Is now a unique resource holding a compiled geometry handle, and constructed from a Mesh, taking ownership of the mesh's data.
- `Texture`: Now simply a non-owning view and can be freely copied. The underlying file texture is owned by the render manager, and held throughout the manager's lifetime.
- `CallbackTexture`: In contrast, this is a unique render resource, automatically released when out of scope.

See [this commit message](https://github.com/mikke89/RmlUi/commit/a452f26951f9450d484496cccdfad9c94b3fd294) for more details.

#### Limitations

Filters will only render based on geometry that is visible on-screen. Thus, some filters may be cut off. As an example, an element that is partly clipped with a drop-shadow may have its drop-shadow also clipped, even if it is fully visible. On the other hand, box shadows should always be rendered properly, as they are rendered off-screen and stored in a texture.

### Major layout engine improvements

- Make layout more conformant to CSS specification.
  - Rewritten inline layout engine.
  - Fixed more than a hundred CSS tests, including ACID1.
- Improve readability and maintainability:
  - Better separation of classes, reduce available state.
  - Make classes better conform to CSS terminology.
  - Improve call-graph, flow from parent to child, avoid mutable calls into ancestors.
- Fix long-standing issues.
  - Allow tables and flexboxes to be absolutely positioned or floated.
  - Allow nested flexboxes: Flex items can now be flex containers themselves. #320
  - Better handling of block-level boxes in inline formatting contexts. #392

#### Detailed layout improvements

Here is a more detailed change list resulting from the rewritten inline formatting engine, and some related changes.

- Corrected the baseline of fonts, they should now line up better.
- Inline layout now properly uses font metrics as appropriate, even without any text contents.
  - In particular, vertical alignment now considers font ascent and descent.
  - This might make some lines taller. In particular, inline-level boxes that are placed on the baseline will now make space for the font descent below the baseline.
- Improved baselines for inline-block boxes.
- Content height of inline boxes no longer depend on their `line-height` property, only font metrics.
- Block formatting contexts (BFC) now work like in CSS: Floated boxes share space and interact within the same BFC, and never outside of it.
  - Certain properties cause the element to establish a new BFC, such as overflow != visible, and the new `display: flow-root` value.
  - Milestone: We now pass ACID1!
- Relative positioning now works in other formatting contexts and situations. #471
  - Including for inline, flex, table, and floated elements, in addition to block boxes like before.
  - Also, nested relative elements are now correctly positioned.
- Containing blocks are now determined more like in CSS, particularly for absolute elements.
  - Only elements which are positioned, or with local transform or perspective, establishes an absolute containing block.
- An overflowing element's scroll region no longer has its padding added to the region.
  - Elements are checked for overflowing the padding box instead of the content box, before enabling scrollbars.
  - The border box of floats will now be considered for overflow, instead of their margin box.
- Fix some replaced elements (e.g. textarea) not rendering correctly in several situations, such as when set to block display, floated, or absolutely positioned.
- Improve shrink-to-fit width, now includes floating children when determining width.
- Margins of absolutely positioned elements now better account for inset (top/right/bottom/left) properties.
- Support for new [`display`](https://mikke89.github.io/RmlUiDoc/pages/rcss/visual_formatting_model.html#display) values: `flow-root`, `inline-flex`, `inline-table`.
- Support for the value [`vertical-align: center`](https://www.w3.org/TR/css-inline-3/#valdef-baseline-shift-center).
- Stacking contexts are now established in a way that more closely aligns with CSS.
- Improve the paint order of elements.
  - Render all stacking context children after the current element's background and decoration. This change is consistent with the CSS paint order. Additionally, it leads to simpler code and less state change, particularly when combined with the advanced rendering effects.

Please see the list of breaking changes and solutions at the end of the changelog. 

#### Layout comparisons

Here are some screenshots demonstrating the layout improvements.

![inline-formatting-01-mix](https://github.com/user-attachments/assets/f27ce0ef-2150-4545-9af2-eca65f1fc02a)

The above example demonstrates a variety of inline formatting details, with nested elements and borders ([fiddle](https://jsfiddle.net/kmouse/etpnu6rb/55/)). We now match nicely with web browsers in such situations. The old behavior has several issues, in particular the elements are not aligned correctly and the border is broken off too early. Note that Firefox in these examples uses a different font, so expect some differences for that reason.

![inline-formatting-04-mix](https://github.com/user-attachments/assets/f547e44d-9a1b-4053-b2e2-4d2efaf9cd5b)

The above shows tests for line splits and borders in particular ([fiddle](https://jsfiddle.net/kmouse/nvscbmoy/5/)). The old behavior is almost comically broken. The new behavior has for the most part been written from scratch following the CSS specifications, and turns out to nicely match up with Firefox.

![inline-formatting-aligned-subtree](https://github.com/user-attachments/assets/c275519d-6e9d-4927-8965-b048ccc615f7)

Finally, this example tests vertical alignment of inline boxes, and particularly the concept of aligned subtrees ([fiddle](https://jsfiddle.net/kmouse/h3c5muyL/7/)). Again, we now nicely align with Firefox. The old behavior looks like it just gave up in the middle. I included Chrome here too, since I find it interesting how different it behaves compared to Firefox. In fact, I found a lot of these differences while testing various nuances of inline formatting. In this case, I am quite convinced that Firefox (and we) are doing the right thing and Chrome is not following the specifications.

#### General layout improvements

- Scale pixels-per-inch (PPI) units based on the context's dp-ratio. #468 (thanks @Dakror)
- Make flex containers and tables with fixed width work in shrink-to-fit contexts. #520
- Compute shrink-to-fit width for flex boxes. #559 #577 (thanks @alml)
- Add the `space-evenly` value to flex box properties `justify-content` and `align-content`. #585 (thanks @LucidSigma)
- Implement the [`gap` property](https://mikke89.github.io/RmlUiDoc/pages/rcss/flexboxes.html#gap) for flexboxes. #621 (thanks @ChrisFloofyKitsune)
- Flexbox: Consider intrinsic sizes when determining flex base size, fixes an assertion error. #640

### CMake modernization

The CMake code has been fully rewritten with modern practices. Special thanks to @hobyst who laid the groundwork for this change, with a solid foundation and great guidelines. #198 #446 #551 #606 (thanks @hobyst)

While modernizing our CMake code, it was clear that we also needed to change our naming conventions. This leads to quite significant breaking changes for building and linking, but the result should make the library a lot easier to work with and link to.

We now try to support all setups including:

1. Adding the library as a subdirectory directly from CMake.
2. Building the library "in-source" without installing.
3. Building and installing the library.
4. Using pre-built Windows binaries.

It should be a lot easier now to simply point to the built library or sources, and have everything link correctly.

And naturally, we will continue to support package managers however we can, and that is still considered the default recommendation. However, for the most part we rely on contributors to keep supporting this. Please help out with your favorite package manager if you see missing versions, or room for improvements.

Large parts of the CI workflows have also been rewritten to accommodate these changes. Most of the Windows building and packaging procedures have been moved from Appveyor to GitHub Actions, which streamlines our testing and also helps speed up the CI builds.

The [build documentation](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/building_with_cmake.html) has been updated to reflect all the new changes.

#### New target names

We now export the following targets:

| Target          | Old target  | Description                                                     |
|-----------------|-------------|-----------------------------------------------------------------|
| RmlUi::RmlUi    |             | Includes all sub-libraries of the project, as listed just below |
| RmlUi::Core     | RmlCore     | The main library                                                |
| RmlUi::Debugger | RmlDebugger | The debugger library                                            |
| RmlUi::Lua      | RmlLua      | The Lua plugin (when enabled)                                   |

When including RmlUi as a subdirectory, the targets are constructed as aliases. When using pre-built or installed binaries, they are constructed using imported targets, which are available through the exported build targets.

The internal target names have also been changed, although they are typically only needed when exploring or developing the library. They are all lowercase and contain the prefix `rmlui_` to avoid colliding with names in any parent projects. Some examples are: `rmlui_core`, `rmlui_debugger`, `rmlui_sample_invaders`, `rmlui_tutorial_drag`, `rmlui_unit_tests`, and `rmlui_visual_tests`.

#### New library filenames

The library binaries have also changed names. These names would be suffixed by e.g. `.dll` on Windows, and so on.

| Library          | Old library   | Description                   |
|------------------|---------------|-------------------------------|
| `rmlui`          | `RmlCore`     | The core (main) library       |
| `rmlui_debugger` | `RmlDebugger` | The debugger library          |
| `rmlui_lua`      | `RmlLua`      | The Lua plugin (when enabled) |

#### New option names

We have a new set of CMake naming conventions for the library:

- Use `RMLUI_` prefix to make all options specific to this project easily identifiable, and avoid colliding with any parent project variables.
- Do not include negations (such as "not" and "disable"), to avoid situations with double negation.
- Do not include a verb prefix (such as "enable" and "build"), as these are often superfluous.

The following table lists all the new option names.

| Option                             | Default value | Old related option             | Comment                                                                                                                                 |
|------------------------------------|---------------|--------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| RMLUI_BACKEND                      | auto          | SAMPLES_BACKEND                |                                                                                                                                         |
| RMLUI_COMPILER_OPTIONS             | ON            |                                | Automatically sets recommended compiler flags                                                                                           |
| RMLUI_CUSTOM_CONFIGURATION         | OFF           | CUSTOM_CONFIGURATION           |                                                                                                                                         |
| RMLUI_CUSTOM_CONFIGURATION_FILE    |               | CUSTOM_CONFIGURATION_FILE      |                                                                                                                                         |
| RMLUI_CUSTOM_INCLUDE_DIRS          |               | CUSTOM_INCLUDE_DIRS            |                                                                                                                                         |
| RMLUI_CUSTOM_LINK_LIBRARIES        |               | CUSTOM_LINK_LIBRARIES          |                                                                                                                                         |
| RMLUI_CUSTOM_RTTI                  | OFF           | DISABLE_RTTI_AND_EXCEPTIONS    | No longer modifies compiler flags - only enables RmlUi's custom RTTI solution so that the user can disable language RTTI and exceptions |
| RMLUI_FONT_ENGINE                  | freetype      | NO_FONT_INTERFACE_DEFAULT      | Now takes a string with one of the options: `none`, `freetype`                                                                          |
| RMLUI_HARFBUZZ_SAMPLE              | OFF           |                                |                                                                                                                                         |
| RMLUI_INSTALL_RUNTIME_DEPENDENCIES | ON            |                                | Automatically install runtime dependencies on supported platforms (e.g. DLLs)                                                           |
| RMLUI_LOTTIE_PLUGIN                | OFF           | ENABLE_LOTTIE_PLUGIN           |                                                                                                                                         |
| RMLUI_LUA_BINDINGS                 | OFF           | BUILD_LUA_BINDINGS             |                                                                                                                                         |
| RMLUI_LUA_BINDINGS_LIBRARY         | lua           | BUILD_LUA_BINDINGS_FOR_LUAJIT  | Now takes a string with one of the options: `lua`, `lua_as_cxx`, `luajit`                                                               |
| RMLUI_MATRIX_ROW_MAJOR             | OFF           | MATRIX_ROW_MAJOR               |                                                                                                                                         |
| RMLUI_PRECOMPILED_HEADERS          | ON            | ENABLE_PRECOMPILED_HEADERS     |                                                                                                                                         |
| RMLUI_SAMPLES                      | OFF           | BUILD_SAMPLES                  |                                                                                                                                         |
| RMLUI_SVG_PLUGIN                   | OFF           | ENABLE_SVG_PLUGIN              |                                                                                                                                         |
| RMLUI_THIRDPARTY_CONTAINERS        | ON            | NO_THIRDPARTY_CONTAINERS       |                                                                                                                                         |
| RMLUI_TRACY_CONFIGURATION          | ON            |                                | New option for multi-config generators to add Tracy as a separate configuration.                                                        |
| RMLUI_TRACY_MEMORY_PROFILING       | ON            |                                | New option to overload global operator new/delete for memory inspection with Tracy.                                                     |
| RMLUI_TRACY_PROFILING              | OFF           | ENABLE_TRACY_PROFILING         |                                                                                                                                         |
| -                                  |               | VISUAL_TESTS_CAPTURE_DIRECTORY | Replaced with environment variable `RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY`                                                               |
| -                                  |               | VISUAL_TESTS_COMPARE_DIRECTORY | Replaced with environment variable `RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY`                                                               |
| -                                  |               | VISUAL_TESTS_RML_DIRECTORIES   | Replaced with environment variable `RMLUI_VISUAL_TESTS_RML_DIRECTORIES`                                                                 |

For reference, the following options have not changed names, as these are standard options used by CMake.

| Unchanged options | Default value |
|-------------------|---------------|
| CMAKE_BUILD_TYPE  |               |
| BUILD_SHARED_LIBS | ON            |
| BUILD_TESTING     | OFF           |

#### New exported definitions

Certain CMake options, when changed from their default value, require clients to set definitions before including RmlUi. These are automatically set when using the exported CMake targets, otherwise users will need to define them manually.

Some exported definition names have changed, as follows.

| Definition                | Old definition         | Related CMake option  |
|---------------------------|------------------------|-----------------------|
| RMLUI_TRACY_PROFILING     | RMLUI_ENABLE_PROFILING | RMLUI_TRACY_PROFILING |
| RMLUI_CUSTOM_RTTI         | RMLUI_USE_CUSTOM_RTTI  | RMLUI_CUSTOM_RTTI     |

For reference, here follows all other possibly exported definitions.

| Definition                      | Related CMake option            |
|---------------------------------|---------------------------------|
| RMLUI_STATIC_LIB                | BUILD_SHARED_LIBS               |
| RMLUI_NO_THIRDPARTY_CONTAINERS  | RMLUI_THIRDPARTY_CONTAINERS     |
| RMLUI_MATRIX_ROW_MAJOR          | RMLUI_MATRIX_ROW_MAJOR          |
| RMLUI_CUSTOM_CONFIGURATION_FILE | RMLUI_CUSTOM_CONFIGURATION_FILE |

#### CMake presets

We now have CMake presets:

- `samples` Enable samples but only those without extra dependencies.
- `samples-all` Enable all samples, also those with extra dependencies.
- `standalone` Build the library completely without any dependencies, the only sample available is `bitmap_font`.
- `dev` Enable testing in addition to samples.
- `dev-all` Enable testing in addition to samples, including those that require extra dependencies.

The presets can be combined with other options, like `CMAKE_BUILD_TYPE` to select the desired build type when using single-configuration generators.

### Spatial navigation

Introduce [spatial navigation](https://mikke89.github.io/RmlUiDoc/pages/rcss/user_interface.html#nav) for keyboards and other input devices. This determines how the focus is moved when pressing one of the navigation direction buttons. #142 #519 #524 (thanks @gleblebedev)

- Add the new properties `nav-up`, `nav-right`, `nav-down`, `nav-left`, and shorthand `nav`.
- Add [`:focus-visible` pseudo class](https://mikke89.github.io/RmlUiDoc/pages/rcss/selectors.html#pseudo-selectors) as a way to style elements that should be highlighted during navigation, like its equivalent CSS selector.
- The `invaders` sample implements this feature for full keyboard navigation, and uses `:focus-visible` to highlight the focus.
- Elements in focus are now clicked when pressing space bar.

RCSS example usage:

```css
input { nav: auto; nav-right: #ok_button; }
.menu_item { nav: vertical; border: #000 3px; }
.menu_item:focus-visible { border-color: #ff3; }
```

### Text shaping and font engine

- Add `lang` and `dir` RML attributes, along with text shaping support in the font engine interface. #563 (thanks @LucidSigma)
- Create a sample for text shaping with Harfbuzz, including right-to-left text formatting. #568 #211 #588 (thanks @LucidSigma)
  - Implement fallback font support for the Harfbuzz sample. #635 (thanks @LucidSigma)
- Add support for the [`letter-spacing` property](https://mikke89.github.io/RmlUiDoc/pages/rcss/text.html#letter-spacing). #429 (thanks @igorsegallafa)
- Add initialize and shutdown procedures to font engine interface for improved lifetime management. #583

Screenshots of the HarfBuzz sample showing Arabic text properly rendered with the HarfBuzz font engine, and compared to the default font engine:

![HarfBuzz font engine vs default font engine comparison](https://github.com/user-attachments/assets/67b22875-e504-49c2-b449-3f3e4367d991)


### Elements

- Enable removal of properties using shorthand names. #463 (thanks @aimoonchen)
- Add [`Element::Matches`](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/elements.html#dom-interface), the last missing selector-related function. #573 (thanks @Paril)
- `Element::GetInnerRML` now includes the local style properties set on the element in the returned RML.
- Tab set: Allow `ElementTabSet::RemoveTab` to work on tab sets with no panels. #546 (thanks @exjam)
- Range input: Fix a bug where the bar position was initially placed wrong when using min and max attributes.

### Text input widget

The following improvements apply to both the textarea and text input elements.

- Add [input method editor (IME)](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/ime.html) support. #541 #630 (thanks @ShawnCZek)
  - The text input widget implements the new `TextInputContext` interface, backends can interact with this by implementing the [`TextInputHandler` interface](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/text_input_handler.html) for IME support. 
  - Add native IME support to Win32 backends (`Win32_GL2` and `Win32_VK`).
  - Add new sample `rmlui_sample_ime` to demonstrate IME support (only enabled on supported backends).
- Add support for the `text-align` property in text inputs. #454 #455 (thanks @Dakror)
- Fix clipboard being pasted when Ctrl + Alt + V key combination is used. #464 #465 (thanks @ShawnCZek)
- Fix selection index possibly returning an invalid index. #539 (thanks @ShawnCZek)
- Move the input cursor when the selection range changes. #542 (thanks @ShawnCZek)
- Ignore selection range update when the element is not focused. #544 (thanks @ShawnCZek)
- Text area elements now clip to their padding area instead of the content area, text input elements still clip to their content area (see [58581477](https://github.com/mikke89/RmlUi/commit/58581477a7c41cd2d163b306ae8c8fe0a04de9d2) for details).
- Improve text widget navigation and selection (see [be9a497b](https://github.com/mikke89/RmlUi/commit/be9a497b508bc7598ea22198577e7fb1f0ddf357) for details).
- The text cursor is no longer drawn when selecting text.
- Consume key events with modifiers (ctrl, shift) to prevent event propagation and subsequently performing navigation.
- Fix some cases where the scroll offset would alternate each time the text cursor was moved, causing rendering to flicker.
- Use rounded line height to make render output more stable when scrolling vertically.

IME sample screenshot:

![IME sample screenshots](https://github.com/mikke89/RmlUiDoc/blob/3ec50d400babb58bf4c79f26ac2454a2833bd95d/assets/images/ime_sample.png)

### Utilities

- Improved mesh utilities to construct background geometry for any given box area of the element, including for elements with border-radius, see [`MeshUtilities.h`](./Include/RmlUi/Core/MeshUtilities.h).
- New [`Rectangle`](./Include/RmlUi/Core/Rectangle.h) type to better represent many operations.
- Visual tests:
  - Several new visual tests for the new features.
  - Highlight differences when comparing to previous capture by holding shift key.
  - Replace CMake options with [environment variables](./Tests/readme.md).

### Data bindings

- Enable arbitrary expressions in data address lookups. #547 #550 (thanks @Dakror and @exjam)
- Add enum support to variants and data bindings. #445 (thanks @Dakror)
- Allow nested data models. #484 (thanks @Dakror)
- Fix XML parse error if single curly brace encountered at the end of a data binding string literal. #459 (thanks @Dakror)
- Fix usage of data variables in selected `option`s. #509 #510 (thanks @Dakror)

### Debugger plugin

- Display the axis-aligned bounding box of selected elements, including any transforms and box shadows (white box).
- List [box model sizes](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/debugger.html#element-info) for the selected element.
- Log an error message when externally closing documents owned by the debugger plugin.
- Debugger now works with documents that have modal focus. #642

### Lua plugin

- Add CMake option [`RMLUI_LUA_BINDINGS_LIBRARY`](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/building_with_cmake.html#plugins-and-dependencies) to link to the Lua library compiled as C++, or to LuaJIT. #604 (thanks @LiquidFenrir)
- Add `StopImmediatePropagation` to Rml::Event. #466 (thanks @ShawnCZek)
- Return inserted element from `AppendChild` and `InsertBefore`. #478 (thanks @ShawnCZek)

### System interface

- The system interface is now optional. All functions now have a default implementation. Thus, it is no longer necessary to derive from and set this interface unless you want to customize its functionality.
- The default log output is now used when there is no system interface installed. All print-like calls, including those in backends, are now submitted to the built-in logger.
- Fix the `JoinPath` method so that it is passed through when using the debugger plugin. #462 #603 (thanks @Dakror)

### General improvements

- Add repeat fit modes to the [image decorator](https://mikke89.github.io/RmlUiDoc/pages/rcss/decorators/image.html), e.g. `decorator: image(alien.png repeat)`. #259 #493 (thanks @viseztrance)
- Implement the ability to release specific textures from memory using `Rml::ReleaseTexture`. #543 (thanks @viseztrance)
- Add support for the `not` prefix in media queries. #564 (thanks @Paril)
- Use string parser to allow "quotes" in sprite sheet `src` property. #571 #574 (thanks @andreasschultes)
- Format color types using RCSS hexadecimal notation.
- `CreateString` and `FormatString` methods no longer take a `max_size` argument, as this is now handled automatically.
- Allow using `margin` to offset the `scrollbarcorner`.
- Log a warning when a texture could not be loaded.
- Improve text culling.
  - Previously, the text could be culled (that is, not rendered) even if it was visible due to transforms bringing it into view. Now, text culling properly considers the transform of the element. 
  - The text is now culled when the element is outside the viewport even if no scissor region is active.

### General fixes

- Fix wrong logic for assertion of released textures. #589 (thanks @pgruenbacher)
- Fix in-source documentation in Factory. #646 (thanks @ben-metzger-z)
- Fix some situations where the scroll offset of an element would reset or change after layout updates. #452
- Fix some situations where units were not shown in properties, now all invoked types are ensured to define a string converter.
- In `demo` sample, fix form submit animation not playing smoothly on power saving mode.
- Fix crash on shutdown in `bitmap_font` sample.
- Fix being able to drag through the scroll track of a scrollbar.
- Fix being able to scroll in a direction with hidden overflow.

### Build improvements

- Fix compilation issues on newer Clang and GCC releases due to mixed use of std namespace on standard integer types. #470 #545 #555
- Fix `UnitTests` compilation error on MSVC by updating doctest. #491 (thanks @mwl4)
- Fix `Benchmarks` compilation error when using custom string type. #490 (thanks @mwl4)
- Change `StringUtilities::DecodeRml` to improve compatibility with other string types, like `EASTL::string`. #472 #475 (thanks @gleblebedev)
- Various CMake fixes for MacOS. #525 (thanks @NaLiJa)
- Fix include paths. #533 #643 (thanks @gleblebedev and @Paril)
- Improve integration of the Tracy library in CMake. #516 #518
  - Added CMake options for enabling (1) a separate configuration in multi-config mode with `RMLUI_TRACY_CONFIGURATION`, and (2) the memory profiler by overriding global operators new/delete using `RMLUI_TRACY_MEMORY_PROFILING`.
- Make test executables work with Emscripten.

### Backends

- OpenGL 3: Restore all modified state after rendering a frame. #449 (thanks @reworks-org)
- OpenGL 3: Add depth test to OpenGL state backup. #629 (thanks @ben-metzger-z)
- OpenGL 3: Set forward compatibility flag to fix running on MacOS. #522 (thanks @NaLiJa)
- Vulkan: Several fixes for validation errors and flickering behavior. #593 #601 (thanks @wh1t3lord)
- Vulkan: Update deprecated debug utilities. #579 (thanks @skaarj1989)
- GLFW: Use new GLFW 3.4 cursors when available.
- GLFW: Fix mouse position conversion to pixel coordinates, particularly on MacOS retina displays. #521
- SDL: Use performance counters for increased time precision. 
- Win32: Center window in screen when opening it.

### Breaking changes

#### CMake and linking

- Most options, target names, library filenames, and certain exported definitions, have been changed. Please see the tables above.
- CMake minimum version raised to 3.10.

#### Layout

Expect some possible layout shifts in existing documents, mainly due to better CSS conformance. Detailed notes are available above. Here are some particular situations where layout output may change, and ways to address them:

- Some lines may become taller, with extra spacing added below its baseline.
  - Possible solutions: Adjust the `vertical-align` property of any inline block elements on the line. For example, use top/center/bottom or manual vertical alignment. The line height itself can be adjusted with the `line-height` property.
- Absolutely positioned elements may be placed relative to an element further up in the document tree.
  - Add `position: relative` on the desired ancestor to establish its containing block.
- The containing block size may change in some situations, which could affect percentages in (min-/max-) height properties.
  - To establish a containing block size, make their parent have a definite (non-auto) height.
- Some floated elements may have moved or now extend below parent boxes.
  - Set the parent to use overflow != visible to establish a new block formatting context, or use the new `display: flow-root`.
- Paint order on some elements may have changed.
  - Change the tree order, or the `z-index` or `clip` properties as appropriate.
- Size of shrink-to-fit boxes may have changed.
- Position of documents with margins may have changed.
- Documents that don't have their size set will now shrink to their contents, previously they would span the entire context.
  - The size can be set either directly using the `width` and `height` properties, or implicitly by a combination of the
    `top`/`right`/`bottom`/`left` properties.

#### Elements

- The text area element now clips to its padding area instead of the content area. You may want to adjust RCSS rules to account for this. If you use decorators to display borders for text areas, you can set the decorator paint area to `border-box` and add a transparent border, e.g. `decorator: image(my-textarea-background) border-box; border: 4px transparent;`.

#### Core types

- Changed `Box` enums and `Property` units as follows, now using strong types:
  - `Box::Area` -> `BoxArea` (e.g. `Box::BORDER` -> `BoxArea::Border`, values now in pascal case).
  - `Box::Edge` -> `BoxEdge` (e.g. `Box::TOP` -> `BoxEdge::Top`, values now in pascal case).
  - `Box::Direction` -> `BoxDirection` (e.g. `Box::VERTICAL` -> `BoxDirection::Vertical`, values now in pascal case).
  - `Property::Unit` -> `Unit` (e.g. `Property::PX` -> `Unit::PX`).

#### Core functions

- Changed the signature of `LoadFontFace` (from memory) to take a `Span` instead of a raw pointer. 
- Replaced `Element::ResolveNumericProperty` with `Element::ResolveLength` and `Element::ResolveNumericValue`. Can be used together with `Property::GetNumericValue`.
- Renamed and removed several `Math` functions.
- Removed the `max_size` argument from `CreateString` and `FormatString` methods, this is no longer needed.

#### RCSS

- The old `gradient` decorator has been deprecated, instead one can now use `horizontal-gradient` and `vertical-gradient`, thereby replacing the keyword to indicate direction. Please also see the new gradient decorators (linear, radial, and conic) above.

#### Render interface

Affects all users with a custom backend renderer.

- Signature changes to several functions, see notes above and the [updated documentation](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/render.html).
- See `RenderInterfaceCompatibility` notes above for an adapter from the old render interface.
- Texture is no longer part of the CompileGeometry command, instead it is submitted with the Render... command(s).
- The immediate rendering function has been removed in favor of the compiled geometry version.
  - However, data pointers to the compile function are now stable, which should ease the transition.
- Implementing the new clip mask API is required for handling clipping of transformed elements.
- RmlUi now provides vertex colors and generated texture data in premultiplied alpha.
  - Set the blend mode to handle them accordingly. It is recommended to convert your own textures to use premultiplied alpha. This ensures correct compositing, especially with layers and effects.
- Font effect color output now assumes premultiplied alpha. Color channels are initialized to black (previously white).

#### Render manager

Affects users with custom decorators and other more advanced usage with rendering commands.

- RenderManager should now be used for all rendering instead of directly calling into the render interface.
- Redefine Geometry, introduce Mesh.
  - Mesh is used to define vertices and indices, before it is submitted to construct a Geometry through the render manager.
  - Geometry is now a wrapper around the underlying geometry (to be) submitted to a render interface.
    - Move-only type which automatically releases its underlying resource when out of scope.
  - GeometryUtilities (class and header file) renamed to MeshUtilities.
    - Signatures changed to operate safely on a mesh instead of using raw pointers.
  - Geometry no longer stores a Texture, it must be submitted during rendering.
- Redefine Texture.
  - This class is now simply a non-owning view of either a file texture or a callback texture.
  - CallbackTexture is a uniquely owned wrapper around such a texture.
  - These are constructed through the render manager.
  - File textures are owned by and retained by the render manager throughout its lifetime, released during its destruction.
- Decorator interface: GenerateElementData has a new paint area parameter.
- Moved DecoratorInstancer into the Decorator files.

#### Font engine interface

The font engine interface has been reworked to encompass new features, to simplify, and to modernize. 

- Font metrics are now collected into a single function.
- Signature changes due to text shaping and letter-spacing support.
- The active render manager is now passed into the appropriate functions to generate textures against this one.
- Now using span and string view types where appropriate.

#### Removed deprecated functionality

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
