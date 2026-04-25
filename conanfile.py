from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
import os

class RmlUiConan(ConanFile):
    name = "rmlui" # The package name should be the library's name
    version = "0.1.2"
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
        copy(self, "*", dst=os.path.join(self.package_folder, "Backends"), 
             src=os.path.join(self.source_folder, "Backends"))
        cmake.install()

    def package_info(self):
        self.cpp_info.components["core"].libs = ["rmlui"]
        self.cpp_info.components["core"].requires = ["freetype::freetype","glfw::glfw"]
        if not self.options.shared:
            self.cpp_info.components["core"].defines.append("RMLUI_STATIC_LIB")
        self.cpp_info.components["debugger"].libs = ["rmlui_debugger"]
        self.cpp_info.components["debugger"].requires = ["core"]

        self.cpp_info.components["backend"].includedirs = ["Backends"]
        
        self.cpp_info.components["backend"].requires = ["core"]
            