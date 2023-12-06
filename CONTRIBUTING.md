# RmlUi contribution guidelines

## CMake

The RmlUi project aims to be both compiled standalone and as a subfolder inside a bigger CMake project, allowing other CMake projects to integrate the library in their build pipeline without too much hassle. For this reason, the following conventions must be followed when editing CMake code for the project:

- **Follow the [Modern CMake](https://cliutils.gitlab.io/modern-cmake/) conventions**

- **Go simple:** CMake already allows to do many things without reinventing the wheel. Most code often written in a CMake build script is
not necessarily related to the project itself but to covering specific compilation scenarios and to set certain flags that aren't really
necessary as a means to pre-configure the project without having to input the options at configure time via the CMake CLI. This is a bad
practice that quickly increases the complexity of the build script. Instead:
    - **For flags and options related to cross-compilation scenarios**, [CMake toolchain files](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html) should be used to set such flags and options.
    - **If a compiler or compiler version is being problematic** about something, use a [CMake initial cache script](https://cmake.org/cmake/help/latest/manual/cmake.1.html#options) or CMake preset and advise the user to use it. This way, the consumer will always know when the compilation settings are being diverted from the compiler's default settings.

        Although not recommended, [CMake toolchain files](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html) can be used as well as long as the [`CMAKE_SYSTEM_NAME`](https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html) variable doesn't get set in the toolchain file.

    - **For platform-specific and case-specific flags** like building shared libraries or building framework packages for iOS and macOS, this
    should be specified by the consumer, not by the project itself. For this, CMake toolchain files, CMake presets and initial cache scripts can be used.

    - **To share CMake flag configurations to save consumers time** use [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html), [CMake initial cache scripts](https://cmake.org/cmake/help/latest/manual/cmake.1.html#options) and ready-to-copy CLI commands in the documentation.

- **Assume the person building the library is a consumer:** To save trouble to consumers, the default behavior of the CMake project should be oriented towards the bare minimum needed for the library to work (tests disabled, compilation of examples disabled, additional plugins disabled...). If the consumer needs anything more, they should be the ones tweaking the behavior of the project to suit their needs via CMake options.

- **Treat CMake code as production code and comment extensively:** Properly commenting code improves readability and therefore speeds up code revisions and further modifications and improvements of existing code. When deemed necessary, context clues and additional information about certain decisions might also be given in the form of block comments delimited using [bracket comments](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#bracket-comment).

- **Prefix all created variables and targets:** Because the project aims to support consuming RmlUi as a CMake subdirectory, care should be taken when using and accessing variables and setting up and linking against targets. CMake subdirectories inherit read and write access to all variables accessible from their parent scope/directory, meaning naming overlap of variables and targets may occur and lead to unexpected behavior. For this reason, the name of all created variables must begin with either `RMLUI_` (meaningful variables) or `rmlui_` (temporary variables) and the name of all created targets related to the project must begin with either `rmlui_` (internal CMake targets and aliases) or `RmlUi::` (targets and aliases intended to be referenced externally when consumed as a CMake subdirectory or installed and exported when built standalone). Consumers looking to consume RmlUi as a subdirectory in their CMake project should be aware of the fact that the project reserves the right to use these prefixes to prevent name collisions.

- **Try keeping the scope clean:** For temporary variables that are necessary for a very limited amount of lines of code, remove them afterwards using `unset()`.

- **Use quotes to declare string literals:**
    If not quoted and depending on the scenario, CMake and some of its functions might read what the developer intended to be an unquoted string literal as a variable name. Strings containing [references to variables](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#variable-references) whose values might contain whitespaces, such as variables that contain variable paths, also benefit from this as it protects the CMake project from unexpected behavior due to these whitespaces. It also helps with readability. However, when calling `add_library()` with paths that are known not to have whitespaces, quoting the source file paths isn't necessary because it doesn't read CMake variables and reads every literal string without a whitespace as a new source file.

- **Avoid setting global variables at all costs:** These should be set, if necessary, by the consumer via the CLI, a CMake configure preset, a CMake initial cache script or CMake variables coming from a parent CMake project, not by the project itself. This includes, among others, the widely used `CMAKE_<LANG>_FLAGS`.

- **Avoid setting compiler and/or linker flags at all costs:** 
    Many projects have the habit of setting
    compiler flags for things like preventing certain irremediable compiler warnings from appearing
    in the compiler logs and to mitigate other compiler-specific issues, often creating an unnecessary
    dependency on certain compilers, increasing the complexity of build scripts and potentially
    creating issues with the project consuming the library.

    Both the code and the CMake project should aim to be as toolchain-agnostic as possible and therefore avoid
    any kind of compiler-specific code as much as possible. **In the event that compiler flags need to be set and used in every possible use case of the library, please do so on a target basis** using either [CMake compile features](https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html)
    and [`target_compile_features()`](https://cmake.org/cmake/help/latest/command/target_compile_features.html) when possible or [`target_compile_options()`](https://cmake.org/cmake/help/latest/command/target_compile_options.html). The use of such flags by the project by default needs to be noted in the documentation in order to help consumers predict and mitigate any issues that may arise when consuming the library.

    If the goal is to save time for consumers to set certain options, these should be instead reproduced in a [CMake configure preset](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html#configure-preset), a [CMake initial cache script](https://cmake.org/cmake/help/latest/manual/cmake.1.html#options) (for CMake versions without preset support) and specified in the documentation so that the consumer is always aware of which options are being used to compile the library.

* **Do not reference [`CMAKE_SOURCE_DIR`](https://cmake.org/cmake/help/latest/variable/CMAKE_SOURCE_DIR.html):** RmlUi aims to be consumable by other CMake projects when being included as a CMake sub-project using [`add_subdirectory()`](https://cmake.org/cmake/help/latest/command/add_subdirectory.html). In this scenario, `CMAKE_SOURCE_DIR` won't point to the top source directory of the RmlUi source, but to the top source directory of the parent CMake project consuming RmlUi. For this reason, **reference either [`CMAKE_CURRENT_SOURCE_DIR`](https://cmake.org/cmake/help/latest/variable/CMAKE_CURRENT_SOURCE_DIR.html) or [`PROJECT_SOURCE_DIR`](https://cmake.org/cmake/help/latest/variable/PROJECT_SOURCE_DIR.html) when relevant instead**.

- **Use [generator expressions](https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html) when relevant instead of their CMake variable counterparts:** To find the folder where the executable has been built or to simply find the current build type, CMake variables like `CMAKE_BUILD_TYPE` and `CMAKE_BINARY_DIR` have been used, but this behavior is not recommended as every build system has its own conventions when it comes to folder paths and not all details are known at configure time, specially when multi-config build systems like MSBuild (Visual Studio) are used. For this reason, it is strongly advised to use CMake generator expressions whenever possible to ensure the project can be compiled regardless of the build system used. If a CMake feature you need to use doesn't work with generator expressions, try making a CMake script and [calling it at build time](https://cmake.org/cmake/help/latest/manual/cmake.1.html#run-a-script) using [`add_custom_command()`](https://cmake.org/cmake/help/latest/command/add_custom_command.html) or create an issue or discussion in the RmlUi repository.

- **Every time the minimum CMake version is going to be raised, check for the `RMLUI_CMAKE_MINIMUM_VERSION_RAISE_NOTICE` keyword in all CMake files (`*.cmake`, `CMakeLists.txt`):** This keyword appears in comments indicating improvements, changes or warnings in the CMake project that could be relevant once the minimum CMake version reaches a certain point. Contributors are invited to add more of these comments as they implement more code for the CMake project so that they can be later revised when the moment to increase the minimum CMake version comes.
