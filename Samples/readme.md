## RmlUi Sample Applications

This directory contains a collection of sample applications that demonstrate the use of RmlUi in small, easy to understand applications.

### Directory Overview

#### `assets`

This directory contains the assets shared by all the sample applications.

#### `basic`

This directory contains basic applications that demonstrate initialisation, usage, shutdown and installation of custom interfaces.

-  `animation` animations and transitions
-  `benchmark` a benchmark to measure performance
-  `bitmapfont` using a custom font engine
-  `customlog` setting up custom logging
-  `databinding` setting up and using data bindings
-  `demo` demonstrates a variety of features in RmlUi and includes a sandbox for playing with RML/RCSS
-  `drag` dragging elements between containers
-  `harfbuzzshaping` advanced text shaping, only enabled when [HarfBuzz](https://harfbuzz.github.io/) is enabled 
-  `loaddocument` loading your first document
-  `lottie` playing Lottie animations, only enabled with the [Lottie plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/lottie.html)
-  `svg` render SVG images, only enabled with the [SVG plugin](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/svg.html)
-  `transform` demonstration of transforms
-  `treeview` using data bindings to create a file browser

#### `invaders`

A full implementation of the 1970s classic Space Invaders using the RmlUi interface.

#### `luainvaders`

Lua version of the invaders sample. Only installed with the Lua plugin.

#### `shell`

The shell provides some common functionality that are specific to the included samples and tests, such as loading fonts and handling global keyboard shortcuts.

Note that, the code for rendering, opening and closing windows, and providing inputs, is instead located in the [backends](../Backends/), however some extensions to these backends are found in the shell.

#### `tutorial`

Tutorial code that should be used in conjunction with the tutorials in the [RmlUi documentation](https://mikke89.github.io/RmlUiDoc/).
