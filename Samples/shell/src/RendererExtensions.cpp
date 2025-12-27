#include "../include/RendererExtensions.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Traits.h>

#if defined RMLUI_RENDERER_GL2

	#if defined RMLUI_PLATFORM_WIN32
		#include <RmlUi_Include_Windows.h>
		#include <gl/Gl.h>
	#elif defined RMLUI_PLATFORM_MACOSX
		#include <AGL/agl.h>
		#include <OpenGL/gl.h>
		#include <OpenGL/glext.h>
	#elif defined RMLUI_PLATFORM_UNIX
		#include <GL/gl.h>
		#include <GL/glext.h>
		#include <GL/glx.h>
		#include <RmlUi_Include_Xlib.h>
	#endif

#elif defined RMLUI_RENDERER_GL3 && !defined RMLUI_PLATFORM_EMSCRIPTEN

	#include <RmlUi_Include_GL3.h>

#elif defined RMLUI_RENDERER_GL3 && defined RMLUI_PLATFORM_EMSCRIPTEN

	#include <GLES3/gl3.h>

#elif defined RMLUI_RENDERER_DX12

	#include <RmlUi_Renderer_DX12.h>

#endif

#if defined RMLUI_RENDERER_GL2 || defined RMLUI_RENDERER_GL3

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
	int viewport[4] = {}; // x, y, width, height
	glGetIntegerv(GL_VIEWPORT, viewport);

	Image image;
	image.num_components = 3;
	image.width = viewport[2];
	image.height = viewport[3];

	if (image.width < 1 || image.height < 1)
		return Image();

	const int byte_size = image.width * image.height * image.num_components;
	image.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[byte_size]);

	glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.data.get());

	bool result = true;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		result = false;
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screenshot, got GL error: 0x%x", err);
	}

	if (!result)
		return Image();

	return image;
}

#elif defined RMLUI_RENDERER_DX12

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
	auto* render_interface = rmlui_static_cast<RenderInterface_DX12*>(Rml::GetRenderInterface());
	if (!render_interface)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screen. RmlUi render interface not set, or unexpected type.");
		return {};
	}

	RendererExtensions::Image image;
	if (!render_interface->CaptureScreen(image.width, image.height, image.num_components, image.data))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "The DirectX 12 renderer was unable to capture screen.");
		return {};
	}

	return image;
}

#else

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
	return Image();
}

#endif
