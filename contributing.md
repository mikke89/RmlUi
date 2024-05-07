# RmlUi contribution guidelines

## CMake

RmlUi aims to support all of the following consumption modes:

- Adding the library as a subdirectory directly from CMake.
- Building the library "in-source" without installing.
- Building and installing the library.
- Using pre-built binaries on applicable platforms.

This flexibility allows projects to easily integrate the library within their build setups, and also provides a good foundation for integration into package managers. To support these modes, together with the aim of ensuring clean and maintainable code, the following conventions should be followed when editing CMake code for the project:

- **Follow modern CMake conventions**. See [Modern CMake](https://cliutils.gitlab.io/modern-cmake/) by Henry Schreiner and contributors, and [Professional CMake](https://crascit.com/professional-cmake/) by Craig Scott.

- **Go simple:** CMake already provides a wide set of features that have been adopted to work on a large variety of target platforms and compilers. Make use of these generic facilities rather than writing scenario-specific code or setting compiler-specific flags.

  - **Compiler-specific options and flags should be provided by consumers:** If necessary, consumers should set variables on the command-line, set variables in the parent project, add a [CMake user presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) file, or provide a [CMake toolchain file](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html), as appropriate. This applies in particular to setting options for their compiler toolchain, platform, building static instead of dynamic libraries, building framework packages for iOS and macOS, or other specific scenarios.

  - **Avoid setting global variables:** If necessary, these should be set by the consumer, not by the project itself. This includes in particular the variable `CMAKE_<LANG>_FLAGS`.

  - **Avoid setting compiler flags:** This often creates an unnecessary dependency on certain compilers, increasing the complexity of build scripts and potentially creating issues with the project consuming the library.

  - **In the event that compiler flags need to be set, please do so on a target basis** using either [CMake compile features](https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html) and [`target_compile_features()`](https://cmake.org/cmake/help/latest/command/target_compile_features.html) when possible or [`target_compile_options()`](https://cmake.org/cmake/help/latest/command/target_compile_options.html). The use of such flags by the project by default needs to be noted in the documentation in order to help consumers predict and mitigate any issues that may arise when consuming the library.

  - **Keep CMake presets small and independent of specific compilers**: The library-provided CMake presets should only provide a small number of typical scenarios for consumers, contributors, and maintainers. Flags and settings that are specific to certain compiler toolchains should not be in the presets. By following this convention, we avoid a combinatorial explosion due to base scenarios combined with different toolchains, as well as variations of each scenario. Instead, users should provide variables and flags specific to their setup.

- **Assume building the library as a consumer:** The default behavior of the CMake project should be oriented towards the bare minimum needed for the library to work (tests, samples, and additional plugins disabled). If the consumer needs anything more, they should be the ones tweaking the behavior of the project to suit their needs via CMake options.

- **Treat CMake code as production code:** When deemed necessary, context clues and additional information about certain decisions might also be given in the form of block comments delimited using [bracket comments](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#bracket-comment).

- **Prefix all variables and targets visible from outer scopes:** Because the project aims to support consuming RmlUi as a CMake subdirectory, care should be taken when using variables and targets. CMake subdirectories inherit read and write access to all variables accessible from their parent scope/directory, meaning naming overlap of variables and targets may occur and lead to unexpected behavior. For this reason, the names of meaningful variables should begin with `RMLUI_`, and the name of all targets related to the project must begin with `rmlui_` (internal CMake targets and aliases) or `RmlUi::` (imported targets and aliases intended to be referenced externally when consumed as a CMake subdirectory or installed and exported when built standalone).

- **Keep the scope clean:** For temporary variables that are necessary for a very limited amount of lines of code, remove them afterwards using `unset()`.

- **Prefer quoted string literals:** Quoted string literals helps with clarity and protects some CMake functions from interpreting the string as a variable name. It further protects against unintended behavior in strings containing [references to variables](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#variable-references) in particular when whitespace is present.

- **Do not reference [`CMAKE_SOURCE_DIR`](https://cmake.org/cmake/help/latest/variable/CMAKE_SOURCE_DIR.html):** RmlUi aims to be consumable by other CMake projects when being included as a CMake sub-project using [`add_subdirectory()`](https://cmake.org/cmake/help/latest/command/add_subdirectory.html). In this scenario, `CMAKE_SOURCE_DIR` won't point to the top source directory of the RmlUi source, but to the top source directory of the parent CMake project consuming RmlUi. For this reason, **reference either [`CMAKE_CURRENT_SOURCE_DIR`](https://cmake.org/cmake/help/latest/variable/CMAKE_CURRENT_SOURCE_DIR.html) or [`PROJECT_SOURCE_DIR`](https://cmake.org/cmake/help/latest/variable/PROJECT_SOURCE_DIR.html) when relevant instead**.

- **Use [generator expressions](https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html) when relevant instead of their CMake variable counterparts:** To find the folder where the executable has been built or to simply find the current build type, CMake variables like `CMAKE_BUILD_TYPE` and `CMAKE_BINARY_DIR` are commonly used. However, this behavior is not recommended as every build system has its own conventions when it comes to folder paths and are not always known at configure time, especially for multi-config generators like Visual Studio. For this reason, it is strongly advised to use CMake generator expressions whenever possible to ensure the project can be compiled regardless of the build system used. If a specific CMake feature doesn't work with generator expressions, try making a CMake script and [calling it at build time](https://cmake.org/cmake/help/latest/manual/cmake.1.html#run-a-script) using [`add_custom_command()`](https://cmake.org/cmake/help/latest/command/add_custom_command.html) or create an issue or discussion in the RmlUi repository.

- **Every time the minimum CMake version is going to be raised, check for the `RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE` keyword in all CMake files (`*.cmake`, `CMakeLists.txt`):** This keyword appears in comments indicating improvements, changes or warnings in the CMake project that could be relevant once the minimum CMake version reaches a certain point. Contributors are invited to add more of these comments as they implement more code for the CMake project so that they can be later revised when the moment to increase the minimum CMake version comes.
