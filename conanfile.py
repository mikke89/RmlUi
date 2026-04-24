from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
    
class RmlUiConan(ConanFile):
    name = "rmlui" # The package name should be the library's name
    version = "0.1.1"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_app": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_app": False
    }
    # Make sure to export ALL necessary source code.
    exports_sources = "CMakeLists.txt", "Source/*","Samples/*","CMake/*","Backends/*","Include/*"
    
    def requirements(self):
        #self.requires("Libname/version")
        self.requires("freetype/2.14.1")
        self.requires("glfw/3.4")
        if self.options.build_app:  # Only for the app
            pass
        else: # Only for the libs
            pass
    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        #NOTE: This is if you want to publish apps with libs too
        tc.variables["BUILD_APPLICATION"] = self.options.build_app
        tc.variables["RMLUI_BACKEND"]="GLFW_GL3"
        # tc.variables["RMLUI_IS_ROOT_PROJECT"]=True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
            # Define the main RmlUi component
            self.cpp_info.components["rmlui_backend_common_headers"].includedirs=["Backends"]
            self.cpp_info.components["rmlui_backend_common_headers"].type = "header-library"
            self.cpp_info.components["rml_core"].libs = ["rmlui"]
            self.cpp_info.components["rml_core"].requires = ["freetype::freetype", "glfw::glfw"]
            
            # Define the Debugger component (since it was built)
            self.cpp_info.components["rml_debugger"].libs = ["rmlui_debugger"]
            self.cpp_info.components["rml_debugger"].requires = ["rml_core"]