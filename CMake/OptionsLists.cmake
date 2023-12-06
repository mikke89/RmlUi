#[[
    Lists of options available to configure some aspects of the project
]]

# List of available backends
list(APPEND RMLUI_SAMPLES_AVAILABLE_BACKENDS
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

# List of available font interfaces
list(APPEND RMLUI_AVAILABLE_FONT_INTERFACES 
    "none"
    "freetype"
)
