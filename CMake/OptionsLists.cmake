#[[
	Lists of options available to configure certain aspects of the project.
]]

set(RMLUI_BACKEND_OPTIONS
	"auto"
	"native"
	"Win32_GL2"
	"Win32_VK"
	"X11_GL2"
	"SDL_GL2"
	"SDL_GL3"
	"SDL_VK"
	"SDL_SDLrenderer"
	"SDL_GPU"
	"SFML_GL2"
	"GLFW_GL2"
	"GLFW_GL3"
	"GLFW_VK"
	"BackwardCompatible_GLFW_GL2"
	"BackwardCompatible_GLFW_GL3"
)

set(RMLUI_FONT_ENGINE_OPTIONS
	"none"
	"freetype"
)

set(RMLUI_LUA_BINDINGS_LIBRARY_OPTIONS
	"lua"
	"lua_as_cxx"
	"luajit"
)
