#include "RmlUi_Backend.h"

// todo: mikke89 think about how to resolve headers because they must be in one place like this because user request for which backend platform we
// want to create Rml::SystemInterface and then provide to initialize of backend it is important

#ifdef RMLUI_PLATFORM_WIN32
	#include "RmlUi_Platform_Win32.h"
#endif
// #include "RmlUi_Platform_SDL.h"
// #include "RmlUi_Platform_GLFW.h"
// #include "RmlUi_Platform_X11.h"

#define FIX_UNUSED(word) (void)(word);

namespace Backend {
Backend::Type ___renderer_type = Backend::Type::Unknown;
KeyDownCallback ___renderer_key_down_callback = nullptr;
bool ___renderer_context_dpi_enable = false;
void* ___renderer_context_hwnd = nullptr;
TypeSystemInterface ___renderer_copy_info_tsi = TypeSystemInterface::Native_Unknown;
int ___renderer_initial_width = 0;
int ___renderer_initial_height = 0;

Rml::Context* Initialize(RmlRenderInitInfo* p_info)
{
	if (p_info == nullptr)
		return nullptr;

	if (p_info->backend_type == 0)
		return nullptr;

	___renderer_key_down_callback = p_info->p_key_callback;
	___renderer_copy_info_tsi = static_cast<TypeSystemInterface>(p_info->system_interface_type);
	___renderer_context_hwnd = p_info->p_native_window_handle;
	___renderer_initial_width = p_info->initial_width;
	___renderer_initial_height = p_info->initial_height;

	switch (static_cast<Backend::Type>(p_info->backend_type))
	{
	case Backend::Type::DirectX_12:
	{
		Backend::TypeSystemInterface type_sys = static_cast<Backend::TypeSystemInterface>(p_info->system_interface_type);

		Rml::SystemInterface* p_system_interface = nullptr;

		FIX_UNUSED(p_system_interface);

		switch (type_sys)
		{
		case Backend::TypeSystemInterface::Native_Unknown:
		{
			RMLUI_ASSERTMSG(false, "unknown typesysteminterface!");
			break;
		}
		case Backend::TypeSystemInterface::Native_Win32:
		{
#ifdef RMLUI_PLATFORM_WIN32
			SystemInterface_Win32* p_instance = new SystemInterface_Win32();

			if (p_instance)
				p_instance->SetWindow(static_cast<HWND>(p_info->p_native_window_handle));

			p_system_interface = p_instance;
#endif
			break;
		}
		case Backend::TypeSystemInterface::Native_Linux:
		{
			RMLUI_ASSERTMSG(false,
				"not compatiable system interface platform on requested backend since DirectX-12 support following platforms: Native_Win32, "
				"Library_SDL2, Library_SDL3, Library_GLFW");
			break;
		}
		case Backend::TypeSystemInterface::Native_MacOS:
		{
			RMLUI_ASSERTMSG(false,
				"not compatiable system interface platform on requested backend since DirectX-12 support following platforms: Native_Win32, "
				"Library_SDL2, Library_SDl3, Library_GLFW");
			break;
		}
		case Backend::TypeSystemInterface::Library_SDL2:
		{
			RMLUI_ASSERTMSG(false, "mikke89 should resolve and provide implementation");
			break;
		}
		case Backend::TypeSystemInterface::Library_SDL3:
		{
			RMLUI_ASSERTMSG(false, "mikke89 should resolve and provide implementation");
			break;
		}
		case Backend::TypeSystemInterface::Library_GLFW:
		{
			RMLUI_ASSERTMSG(false, "mikke89 should resolve and provide implementation");
			break;
		}
		case Backend::TypeSystemInterface::Library_Unknown:
		{
			RMLUI_ASSERTMSG(false, "unknown library typesysteminterface type!");
			break;
		}
		}

		RMLUI_ASSERTMSG(p_system_interface, "failed to create Rml::SystemInterface");

#ifdef RMLUI_PLATFORM_WIN32
		return Backend::DX12::Initialize(p_info, p_system_interface);
#else
		return nullptr;
#endif
	}
	case Backend::Type::DirectX_11:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-11 integration initialize route");
		return nullptr;
	}
	case Backend::Type::DirectX_10:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-10 integration initialize route");
		return nullptr;
	}
	case Backend::Type::DirectX_9:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-9 integration initialize route");
		return nullptr;
	}
	case Backend::Type::Vulkan:
	{
		RMLUI_ASSERTMSG(false, "todo: add vulkan integration initialize route");
		return nullptr;
	}
	case Backend::Type::OpenGL_3:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 3 integration initialize route");
		return nullptr;
	}
	case Backend::Type::OpenGL_2:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 2 integration initialize route");
		return nullptr;
	}
	case Backend::Type::EGL:
	{
		RMLUI_ASSERTMSG(false, "todo: add embedded opengl integration initialize route");
		return nullptr;
	}
	case Backend::Type::Metal:
	{
		RMLUI_ASSERTMSG(false, "todo: add metal integration initialize route");
		return nullptr;
	}
	default:
	{
		RMLUI_ASSERTMSG(false, "unknown and undefined backend type that rmlui doesn't support, report to developers please!");
		return nullptr;
	}
	}
}

void Resize(Rml::Context* p_context, int width, int height)
{
	RMLUI_ASSERT(___renderer_type != Backend::Type::Unknown && "early calling?");

	FIX_UNUSED(p_context);
	FIX_UNUSED(width);
	FIX_UNUSED(height);

	switch (___renderer_type)
	{
	case Backend::Type::DirectX_12:
	{
#ifdef RMLUI_PLATFORM_WIN32
		Backend::DX12::Resize(p_context, width, height);
#endif

		break;
	}
	case Backend::Type::DirectX_11:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-11 integration resize route");
		break;
	}
	case Backend::Type::DirectX_10:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-10 integration resize route");
		break;
	}
	case Backend::Type::DirectX_9:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-9 integration resize route");
		break;
	}
	case Backend::Type::Vulkan:
	{
		RMLUI_ASSERTMSG(false, "todo: add vulkan integration resize route");
		break;
	}
	case Backend::Type::OpenGL_3:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 3 integration resize route");
		break;
	}
	case Backend::Type::OpenGL_2:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 2 integration resize route");
		break;
	}
	case Backend::Type::EGL:
	{
		RMLUI_ASSERTMSG(false, "todo: add egl integration resize route");
		break;
	}
	case Backend::Type::Metal:
	{
		RMLUI_ASSERTMSG(false, "todo: add metal integration resize route");
		break;
	}
	default:
	{
		RMLUI_ASSERTMSG(false, "unknown and undefined backend type that rmlui doesn't support, report to developers please!");
		break;
	}
	}
}

void Shutdown()
{
	RMLUI_ASSERT(___renderer_type != Backend::Type::Unknown && "early calling?");

	switch (___renderer_type)
	{
	case Backend::Type::DirectX_12:
	{
#ifdef RMLUI_PLATFORM_WIN32
		Backend::DX12::Shutdown();
#endif

		break;
	}
	case Backend::Type::DirectX_11:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-11 integration shutdown route");
		break;
	}
	case Backend::Type::DirectX_10:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-10 integration shutdown route");
		break;
	}
	case Backend::Type::DirectX_9:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-9 integration shutdown route");
		break;
	}
	case Backend::Type::Vulkan:
	{
		RMLUI_ASSERTMSG(false, "todo: add vulkan integration shutdown route");
		break;
	}
	case Backend::Type::OpenGL_3:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 3 integration shutdown route");
		break;
	}
	case Backend::Type::OpenGL_2:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 2 integration shutdown route");
		break;
	}
	case Backend::Type::EGL:
	{
		RMLUI_ASSERTMSG(false, "todo: add egl integration shutdown route");
		break;
	}
	case Backend::Type::Metal:
	{
		RMLUI_ASSERTMSG(false, "todo: add metal integration shutdown route");
		break;
	}
	default:
	{
		RMLUI_ASSERTMSG(false, "unknown and undefined backend type that rmlui doesn't support, report to developers please!");
		break;
	}
	}
}

void BeginFrame(void* p_input_rtv, void* p_input_dsv, unsigned char current_framebuffer_index)
{
	RMLUI_ASSERT(___renderer_type != Backend::Type::Unknown && "early calling?");

	FIX_UNUSED(p_input_rtv);
	FIX_UNUSED(p_input_dsv);
	FIX_UNUSED(current_framebuffer_index);

	switch (___renderer_type)
	{
	case Backend::Type::DirectX_12:
	{
#ifdef RMLUI_PLATFORM_WIN32
		Backend::DX12::BeginFrame(p_input_rtv, p_input_dsv, current_framebuffer_index);
#endif

		break;
	}
	case Backend::Type::DirectX_11:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-11 integration beginframe route");
		break;
	}
	case Backend::Type::DirectX_10:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-10 integration beginframe route");
		break;
	}
	case Backend::Type::DirectX_9:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-9 integration beginframe route");
		break;
	}
	case Backend::Type::Vulkan:
	{
		RMLUI_ASSERTMSG(false, "todo: add vulkan integration beginframe route");
		break;
	}
	case Backend::Type::OpenGL_3:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 3 integration beginframe route");
		break;
	}
	case Backend::Type::OpenGL_2:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 2 integration beginframe route");
		break;
	}
	case Backend::Type::EGL:
	{
		RMLUI_ASSERTMSG(false, "todo: add egl integration beginframe route");
		break;
	}
	case Backend::Type::Metal:
	{
		RMLUI_ASSERTMSG(false, "todo: add metal integration beginframe route");
		break;
	}
	default:
	{
		RMLUI_ASSERTMSG(false, "unknown and undefined backend type that rmlui doesn't support, report to developers please!");
		break;
	}
	}
}

void EndFrame()
{
	RMLUI_ASSERT(___renderer_type != Backend::Type::Unknown && "early calling?");

	switch (___renderer_type)
	{
	case Backend::Type::DirectX_12:
	{
#ifdef RMLUI_PLATFORM_WIN32
		Backend::DX12::EndFrame();
#endif

		break;
	}
	case Backend::Type::DirectX_11:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-11 integration endframe route");
		break;
	}
	case Backend::Type::DirectX_10:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-10 integration endframe route");
		break;
	}
	case Backend::Type::DirectX_9:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-9 integration endframe route");
		break;
	}
	case Backend::Type::Vulkan:
	{
		RMLUI_ASSERTMSG(false, "todo: add vulkan integration endframe route");
		break;
	}
	case Backend::Type::OpenGL_3:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 3 integration endframe route");
		break;
	}
	case Backend::Type::OpenGL_2:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 2 integration endframe route");
		break;
	}
	case Backend::Type::EGL:
	{
		RMLUI_ASSERTMSG(false, "todo: add egl integration endframe route");
		break;
	}
	case Backend::Type::Metal:
	{
		RMLUI_ASSERTMSG(false, "todo: add metal integration endframe route");
		break;
	}
	default:
	{
		RMLUI_ASSERTMSG(false, "unknown and undefined backend type that rmlui doesn't support, report to developers please!");
		break;
	}
	}
}

void ProcessEvents(Rml::Context* context, const RmlProcessEventInfo& info, bool power_save)
{
	FIX_UNUSED(context);
	FIX_UNUSED(info);
	FIX_UNUSED(power_save);

	switch (___renderer_type)
	{
	case Backend::Type::DirectX_12:
	{
#ifdef RMLUI_PLATFORM_WIN32
		Backend::DX12::ProcessEvents(context, ___renderer_key_down_callback, info, power_save);
#endif

		break;
	}
	case Backend::Type::DirectX_11:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-11 integration processevents route");
		break;
	}
	case Backend::Type::DirectX_10:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-10 integration processevents route");
		break;
	}
	case Backend::Type::DirectX_9:
	{
		RMLUI_ASSERTMSG(false, "todo: add directx-9 integration processevents route");
		break;
	}
	case Backend::Type::Vulkan:
	{
		RMLUI_ASSERTMSG(false, "todo: add vulkan integration processevents route");
		break;
	}
	case Backend::Type::OpenGL_3:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 3 integration processevents route");
		break;
	}
	case Backend::Type::OpenGL_2:
	{
		RMLUI_ASSERTMSG(false, "todo: add opengl 2 integration processevents route");
		break;
	}
	case Backend::Type::EGL:
	{
		RMLUI_ASSERTMSG(false, "todo: add egl integration processevents route");
		break;
	}
	case Backend::Type::Metal:
	{
		RMLUI_ASSERTMSG(false, "todo: add metal integration processevents route");
		break;
	}
	default:
	{
		break;
	}
	}
}

} // namespace Backend