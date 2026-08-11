## RmlUi Sample Applications

This directory contains a collection of sample applications that demonstrate the use of RmlUi in small, easy to understand applications.

### Directory Overview

#### `assets`

This directory contains the assets shared by all the sample applications.

#### `basic`

This directory contains basic applications that demonstrate initialisation, usage, shutdown and installation of custom interfaces.

- `animation` Animations and transitions.
- `benchmark` A benchmark to measure performance.
- `bitmap_font` Using a custom font engine. Available even without FreeType, i.e. `RMLUI_FONT_ENGINE="none"`.
- `custom_log` Setting up custom logging.
- `data_binding` Setting up and using data bindings.
- `demo` Demonstrates a variety of features in RmlUi.
- `drag` Dragging elements between containers.
- `effects` Advanced rendering effects, including filters, gradients and box shadows. Only enabled with supported backends.
- `harfbuzz` Advanced text shaping. Only enabled when [HarfBuzz](https://harfbuzz.github.io/) is enabled.
- `ime` A showcase of Input Method Editor (IME) with fallback fonts to support different writing systems. Available only when using a Windows backend.
- `load_document` Loading your first document.
- `lottie` Playing Lottie animations, only enabled with the [Lottie plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/lottie.html).
- `svg` Render SVG images, only enabled with the [SVG plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/svg.html).
- `transform` Demonstration of transforms.
- `tree_view` Using data bindings to create a file browser.
- `variables` RCSS variables and custom properties.

#### `invaders`

A full implementation of the 1970s classic Space Invaders using the RmlUi interface.

#### `lua_invaders`

Lua version of the invaders sample. Only installed with the Lua plugin.

#### `sandbox`

An interactive sandbox for playing with RML and RCSS. The left pane holds the RML source, which is rendered live into the area below. The source is treated as the contents of a document body, unless it declares an `rml` root tag, in which case it is loaded as a complete document. The right pane holds RCSS that is applied on top of the document's own style sheet.

Any document can be loaded from file, including documents belonging to other projects. Set the working directory to the location that root-relative paths within the document should be resolved against, such as the asset root of the application it belongs to. Messages logged by RmlUi are listed in the log at the bottom of the menu.

Documents from a running application can also be brought into the sandbox: press `Ctrl+Shift+C` in any of the samples to copy the focused document to the clipboard as a standalone RML document, then paste it into the RML pane. All style sheets are inlined into the copy, see `ElementDocument::SerializeDocument`.

#### `shell`

The shell provides some common functionality that are specific to the included samples and tests, such as loading fonts and handling global keyboard shortcuts.

Note that, the code for rendering, opening and closing windows, and providing inputs, is instead located in the [backends](../Backends/), however some extensions to these backends are found in the shell.

#### `tutorial`

Tutorial code that should be used in conjunction with the tutorials in the [RmlUi documentation](https://mikke89.github.io/RmlUiDoc/).
