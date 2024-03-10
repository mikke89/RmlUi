#[[
    Lists of options available to configure some aspects of the project
]]

set(RMLUI_AVAILABLE_SAMPLES_BACKENDS
	"auto"
    "Win32_GL2"
    "Win32_VK"
    "X11_GL2"
    "SDL_GL2"
    "SDL_GL3"
    "SDL_VK"
    "SDL_SDLrenderer"
    "SFML_GL2"
    "GLFW_GL2"
    "GLFW_GL3"
    "GLFW_VK"
)

set(RMLUI_AVAILABLE_FONT_ENGINES
    "none"
    "freetype"
)

set(RMLUI_AVAILABLE_LUA_BINDINGS_LIBRARIES
	"lua"
	"luajit"
)
