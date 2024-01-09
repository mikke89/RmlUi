-- premake5.lua
workspace "rml0"
   configurations { "Debug", "Release" }
   gccprefix ""
   
   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Speed"


-------------------------------
project "shell"
   	kind "WindowedApp"
  	language "C++"
	  
  	includedirs { 
		"C:/tools/msys64/mingw64/include", 
		"Include", 
		"Backends",
		"Samples/shell/include",
		"3rdparty/GLFW/include",
		"3rdparty/freetype2/include", 
		"3rdparty/lunasvg/include"
	}

	targetdir "bin/%{cfg.buildcfg}"
	objdir "bin/obj/%{cfg.buildcfg}"
	  
	files { "Samples/shell/src/*.cpp", 
			-- "Samples/basic/demo/src/main.cpp", 
			-- "Samples/basic/lottie/src/main.cpp", 
			-- "Samples/basic/drag/src/*.cpp", 
			"Samples/basic/svg/src/*.cpp", 
			"Backends/RmlUi_Renderer_GL2.cpp", 
			"Backends/RmlUi_Platform_GLFW.cpp",
			"Backends/RmlUi_Backend_GLFW_GL2.cpp"
	}
			
	-- libdirs { "c:/Qt/Qt6.3.1/6.3.1/mingw_64/lib" }
	links { "rml", "ft2", "luna", "glfw3", "rlottie", "quickjs",
			"user32", "kernel32", "shlwapi", "opengl32" }

	defines { "RMLUI_STATIC_LIB=1" }
	


-------------------------------
project "rml"
   	kind "StaticLib"
  	language "C++"
	  
  	includedirs { "C:/tools/msys64/mingw64/include", 
					"3rdparty/freetype2/include", 
					"3rdparty/lunasvg/include",
					"3rdparty/rlottie/inc",
					"3rdparty",
					"include"
	}

	targetdir "bin/lib/rml/%{cfg.buildcfg}"
	objdir "bin/lib/rml/obj/%{cfg.buildcfg}"
	  
	files { "Source/Core/*.cpp", 
				"Source/Core/Elements/*.cpp", 
				"Source/Core/FontEngineDefault/*.cpp", 
				"Source/Core/Layout/*.cpp",
				"Source/Debugger/*.cpp", 
				"Source/SVG/*.cpp",
				"Source/Lottie/*.cpp",
				"Source/QuickJS/*.cpp"
		 }

	-- libdirs { "c:/Qt/Qt6.3.1/6.3.1/mingw_64/lib" }
	-- links { "ft2", "rlottie", "qjs" }
	defines { "RMLUI_VERSION_SHORT=6.0-dev", 
				"RMLUI_STATIC_LIB=1", 
				"RMLUI_ENABLE_LOTTIE_PLUGIN", 
				"RMLUI_ENABLE_SVG_PLUGIN",
				"RMLUI_ENABLE_QJS_PLUGIN"
	}
	

-------------------------------	
project "ft2"
	kind "StaticLib"
	language "C++"
	
	includedirs {"3rdparty/freetype2/include"}
	targetdir "bin/lib/ft2/%{cfg.buildcfg}"
  	objdir "bin/lib/ft2/%{cfg.buildcfg}/obj"
	
    files {
	  "3rdparty/freetype2/src/autofit/autofit.c",
	  "3rdparty/freetype2/src/bdf/bdf.c",
	  "3rdparty/freetype2/src/cff/cff.c",
	  "3rdparty/freetype2/src/base/ftbase.c",
	  "3rdparty/freetype2/src/base/ftbitmap.c",
	  "3rdparty/freetype2/src/cache/ftcache.c",
	  "3rdparty/freetype2/src/base/ftdebug.c",
	  "3rdparty/freetype2/src/base/ftfstype.c",
	  "3rdparty/freetype2/src/base/ftglyph.c",
	  "3rdparty/freetype2/src/gzip/ftgzip.c",
	  "3rdparty/freetype2/src/base/ftinit.c",
	  "3rdparty/freetype2/src/lzw/ftlzw.c",
	  "3rdparty/freetype2/src/base/ftstroke.c",
	  "3rdparty/freetype2/src/base/ftsystem.c",
	  "3rdparty/freetype2/src/smooth/smooth.c",

	  "3rdparty/freetype2/src/base/ftbbox.c",
	  "3rdparty/freetype2/src/base/ftgxval.c",
	  "3rdparty/freetype2/src/base/ftlcdfil.c",
	  "3rdparty/freetype2/src/base/ftmm.c",
	  "3rdparty/freetype2/src/base/ftotval.c",
	  "3rdparty/freetype2/src/base/ftpatent.c",
	  "3rdparty/freetype2/src/base/ftpfr.c",
	  "3rdparty/freetype2/src/base/ftsynth.c",
	  --"3rdparty/freetype2/src/base/ftxf86.c",
	  "3rdparty/freetype2/src/base/ftfstype.c",
	  "3rdparty/freetype2/src/pcf/pcf.c",
	  "3rdparty/freetype2/src/pfr/pfr.c",
	  "3rdparty/freetype2/src/psaux/psaux.c",
	  "3rdparty/freetype2/src/pshinter/pshinter.c",
	  "3rdparty/freetype2/src/psnames/psmodule.c",
	  "3rdparty/freetype2/src/raster/raster.c",
	  "3rdparty/freetype2/src/sfnt/sfnt.c",
	  "3rdparty/freetype2/src/truetype/truetype.c",
	  "3rdparty/freetype2/src/type1/type1.c",
	  "3rdparty/freetype2/src/cid/type1cid.c",
	  "3rdparty/freetype2/src/type42/type42.c",
	  "3rdparty/freetype2/src/winfonts/winfnt.c",      
    }
  
    defines
    {
	  "WIN32",
	  "WIN32_LEAN_AND_MEAN",
	  "VC_EXTRALEAN",
	  "_CRT_SECURE_NO_WARNINGS",
	  "FT2_BUILD_LIBRARY",      
    }    



-------------------------------	
project "luna"
	kind "StaticLib"
	language "C++"
	
	includedirs {"3rdparty/lunasvg/include", "3rdparty/lunasvg/3rdparty/plutovg"}
	targetdir "bin/lib/luna/%{cfg.buildcfg}"
  	objdir "bin/lib/luna/%{cfg.buildcfg}/obj"
	
    files {
		"3rdparty/lunasvg/source/lunasvg.cpp",
		"3rdparty/lunasvg/source/element.cpp",
		"3rdparty/lunasvg/source/property.cpp",
		"3rdparty/lunasvg/source/parser.cpp",
		"3rdparty/lunasvg/source/layoutcontext.cpp",
		"3rdparty/lunasvg/source/canvas.cpp",
		"3rdparty/lunasvg/source/clippathelement.cpp",
		"3rdparty/lunasvg/source/defselement.cpp",
		"3rdparty/lunasvg/source/gelement.cpp",
		"3rdparty/lunasvg/source/geometryelement.cpp",
		"3rdparty/lunasvg/source/graphicselement.cpp",
		"3rdparty/lunasvg/source/maskelement.cpp",
		"3rdparty/lunasvg/source/markerelement.cpp",
		"3rdparty/lunasvg/source/paintelement.cpp",
		"3rdparty/lunasvg/source/stopelement.cpp",
		"3rdparty/lunasvg/source/styledelement.cpp",
		"3rdparty/lunasvg/source/styleelement.cpp",
		"3rdparty/lunasvg/source/svgelement.cpp",
		"3rdparty/lunasvg/source/symbolelement.cpp",
		"3rdparty/lunasvg/source/useelement.cpp",

		"3rdparty/lunasvg/3rdparty/plutovg/*.c"
	}


-- GLFW -----------------------------
project "glfw3"
	kind "StaticLib"
	language "C++"
	includedirs {"3rdparty/GLFW"}
	defines { "_GLFW_WIN32", "UNICODE" }

	files { 
		"3rdparty/GLFW/src/win32_init.c",
		"3rdparty/GLFW/src/win32_joystick.c",
		"3rdparty/GLFW/src/win32_monitor.c",
		"3rdparty/GLFW/src/win32_time.c",
		"3rdparty/GLFW/src/win32_thread.c",
		"3rdparty/GLFW/src/win32_window.c",
		"3rdparty/GLFW/src/wgl_context.c",
		"3rdparty/GLFW/src/egl_context.c",
		"3rdparty/GLFW/src/osmesa_context.c",

		"3rdparty/GLFW/src/context.c",
		"3rdparty/GLFW/src/init.c",
		"3rdparty/GLFW/src/input.c",
		"3rdparty/GLFW/src/monitor.c",
		"3rdparty/GLFW/src/vulkan.c",
		"3rdparty/GLFW/src/window.c"
	}

	targetdir "bin/lib/glfw3/%{cfg.buildcfg}"
	objdir "bin/lib/glfw3/%{cfg.buildcfg}/obj"

-- QUICKJS -----------------------------
project "quickjs"
	kind "StaticLib"
	language "C++"
	includedirs {"3rdparty/quickjs"}
	files { "3rdparty/quickjs/*.c", 
			--"3rdparty/quickjs/debugger/*.cpp" 
	}
	--defines { "DUMP_BYTECODE=251" }

	targetdir "bin/lib/qjs/%{cfg.buildcfg}"
   	objdir "bin/lib/qjs/%{cfg.buildcfg}/obj"
	defines { "_CRT_SECURE_NO_WARNINGS" }

-- RLOTTIE -----------------------------
project "rlottie"
	kind "StaticLib"
	language "C++"
	includedirs {
		"3rdparty/rlottie/inc","3rdparty/rlottie",
		"3rdparty/rlottie/src/vector",
		"3rdparty/rlottie/src/vector/freetype"
	}

	files { "3rdparty/rlottie/src/lottie/*.cpp",
			"3rdparty/rlottie/src/lottie/zip/*.cpp",
			"3rdparty/rlottie/src/vector/*.cpp",
			"3rdparty/rlottie/src/vector/freetype/*.cpp",
		 }
	--defines { "RLOTTIE_BUILD" }

	targetdir "bin/lib/rlottie/%{cfg.buildcfg}"
	objdir "bin/lib/rlottie/%{cfg.buildcfg}/obj"
