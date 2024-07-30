## RmlUi Test Suite

This directory contains tests and benchmarks for RmlUi. They have been separated into three projects located under the `Source` directory. These projects can be enabled using the CMake options `BUILD_TESTING`, and built using the CMake targets below.

#### Visual tests: `rmlui_visual_tests`

For visually testing the layout engine in particular, with small test documents that can easily be added. Includes features for capturing and comparing tests for easily spotting differences during development. A conversion script for CSS tests is available, as described below.

The following environment variables can be used to configure the directories used for the visual tests:

| Environment variable                   | Description                                                                                                                                                 |
|----------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `RMLUI_VISUAL_TESTS_RML_DIRECTORIES`   | Additional directories containing `*.rml` test documents. Separate multiple directories by comma. Change test suite directory using `Up`/`Down` arrow keys. |
| `RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY` | Input directory for screenshot comparisons.                                                                                                                 |
| `RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY` | Output directory for generated screenshots.                                                                                                                 |


#### Unit tests: `rmlui_unit_tests`

Ensures smaller units of the library are working properly.


#### Benchmarks: `rmlui_benchmarks`

Benchmarking various components of the library to keep track of performance improvements or regressions for future development, and to find any performance hotspots that could need extra attention.


### Directory Overview

#### `Data`

This directory contains the shared style sheets and documents, as well as the source documents for the included visual tests. All documents located under `Data/VisualTests/` will automatically be loaded by the visual tests project. Additional source folders can be specified with the environment variable `RMLUI_VISUAL_TESTS_RML_DIRECTORIES` as described above. 

#### `Dependencies`

This directory contains additional libraries used by the test suite.

#### `Output`

By default, the visual tests will store screenshots and other outputs into this directory, and read previous screenshots from this directory. To specify other directories, use the environment variables options `RMLUI_VISUAL_TESTS_CAPTURE_DIRECTORY` and `RMLUI_VISUAL_TESTS_COMPARE_DIRECTORY` as described above.

#### `Source`

Source code for the test suite.
               
#### `Tools`

Includes a best-effort conversion script for the [CSS 2.1 tests](https://www.w3.org/Style/CSS/Test/CSS2.1/) in particular, which includes thousands of tests, to RML/RCSS for testing conformance with the CSS specifications. Also works with some CSS 3 tests, like for [flexbox](https://test.csswg.org/suites/css-flexbox-1_dev/) support.
