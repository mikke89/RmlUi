#include "RmlUi_Backend.h"
#include "RmlUi_Renderer_VK.h"
// This space is intentional to prevent autoformat from reordering RmlUi_Renderer_VK behind RmlUi_Platform_GLFW
#include "RmlUi_Platform_GLFW.h"
#include <RmlUi/Config/Config.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <thread>

static void SetupCallbacks(GLFWwindow* window);

static void LogErrorFromGLFW(int error, const char* description)
{
	Rml::Log::Message(Rml::Log::LT_ERROR, "GLFW error (0x%x): %s", error, description);
}

/**
    Global data used by this backend.

    Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
 */
struct BackendData {
	SystemInterface_GLFW system_interface;
	RenderInterface_VK render_interface;

	GLFWwindow* window = nullptr;
	Rml::Context* context = nullptr;
	KeyDownCallback key_down_callback = nullptr;

	int glfw_active_modifiers = 0;
	bool running = true;
	bool context_dimensions_dirty = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
	RMLUI_ASSERT(!data);

	glfwSetErrorCallback(LogErrorFromGLFW);

	if (!glfwInit())
	{
		glfwTerminate();
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, allow_resize ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(width, height, window_name, nullptr, nullptr);

	if (!window)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "GLFW failed to create window");
		return false;
	}

	data = Rml::MakeUnique<BackendData>();
	data->window = window;

	uint32_t count;
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);
	RMLUI_VK_ASSERTMSG(extensions != nullptr, "Failed to query GLFW Vulkan extensions");
	if (!data->render_interface.Initialize(Rml::Vector<const char*>(extensions, extensions + count),
			[](VkInstance instance, VkSurfaceKHR* out_surface) {
				return glfwCreateWindowSurface(instance, data->window, nullptr, out_surface) == VkResult::VK_SUCCESS;
				;
			}))
	{
		data.reset();
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to initialize Vulkan render interface");
		return false;
	}

	data->system_interface.SetWindow(window);
	data->render_interface.SetViewport(width, height);

	// Receive num lock and caps lock modifiers for proper handling of numpad inputs in text fields.
	glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

	SetupCallbacks(window);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);

	data->render_interface.Shutdown();

	glfwDestroyWindow(data->window);

	data.reset();

	glfwTerminate();
}

Rml::SystemInterface* Backend::GetSystemInterface()
{
	RMLUI_ASSERT(data);
	return &data->system_interface;
}

Rml::RenderInterface* Backend::GetRenderInterface()
{
	RMLUI_ASSERT(data);
	return &data->render_interface;
}

static bool WaitForValidSwapchain()
{
	bool result = true;

	// In some situations the swapchain may become invalid, such as when the window is minimized. In this state the renderer cannot accept any render
	// calls. Since we don't have full control over the main loop here we may risk calls to Context::Render if we were to return. Instead, we keep the
	// application inside this loop until we are able to recreate the swapchain and render again.

	while (!data->render_interface.IsSwapchainValid())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (glfwWindowShouldClose(data->window))
		{
			glfwRestoreWindow(data->window);
			result = false;
		}

		glfwPollEvents();
		data->render_interface.RecreateSwapchain();
	}

	return result;
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
	RMLUI_ASSERT(data && context);

	bool result = data->running;

	if (data->context_dimensions_dirty)
	{
		data->context_dimensions_dirty = false;

		Rml::Vector2i window_size;
		float dp_ratio = 1.f;
		glfwGetFramebufferSize(data->window, &window_size.x, &window_size.y);
		glfwGetWindowContentScale(data->window, &dp_ratio, nullptr);

		context->SetDimensions(window_size);
		context->SetDensityIndependentPixelRatio(dp_ratio);
	}

	data->context = context;
	data->key_down_callback = key_down_callback;

	if (power_save)
		glfwWaitEventsTimeout(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0));
	else
		glfwPollEvents();

	if (!WaitForValidSwapchain())
		result = false;

	data->context = nullptr;
	data->key_down_callback = nullptr;

	result = !glfwWindowShouldClose(data->window);
	glfwSetWindowShouldClose(data->window, GLFW_FALSE);

	return result;
}

void Backend::RequestExit()
{
	RMLUI_ASSERT(data);
	data->running = false;
	glfwSetWindowShouldClose(data->window, GLFW_TRUE);
}

void Backend::BeginFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.BeginFrame();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface.EndFrame();
}

static void SetupCallbacks(GLFWwindow* window)
{
	RMLUI_ASSERT(data);

	// Key input
	glfwSetKeyCallback(window, [](GLFWwindow* /*window*/, int glfw_key, int /*scancode*/, int glfw_action, int glfw_mods) {
		if (!data->context)
			return;

		// Store the active modifiers for later because GLFW doesn't provide them in the callbacks to the mouse input events.
		data->glfw_active_modifiers = glfw_mods;

		// Override the default key event callback to add global shortcuts for the samples.
		Rml::Context* context = data->context;
		KeyDownCallback key_down_callback = data->key_down_callback;

		switch (glfw_action)
		{
		case GLFW_PRESS:
		case GLFW_REPEAT:
		{
			const Rml::Input::KeyIdentifier key = RmlGLFW::ConvertKey(glfw_key);
			const int key_modifier = RmlGLFW::ConvertKeyModifiers(glfw_mods);
			float dp_ratio = 1.f;
			glfwGetWindowContentScale(data->window, &dp_ratio, nullptr);

			// See if we have any global shortcuts that take priority over the context.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, dp_ratio, true))
				break;
			// Otherwise, hand the event over to the context by calling the input handler as normal.
			if (!RmlGLFW::ProcessKeyCallback(context, glfw_key, glfw_action, glfw_mods))
				break;
			// The key was not consumed by the context either, try keyboard shortcuts of lower priority.
			if (key_down_callback && !key_down_callback(context, key, key_modifier, dp_ratio, false))
				break;
		}
		break;
		case GLFW_RELEASE: RmlGLFW::ProcessKeyCallback(context, glfw_key, glfw_action, glfw_mods); break;
		}
	});

	glfwSetCharCallback(window, [](GLFWwindow* /*window*/, unsigned int codepoint) { RmlGLFW::ProcessCharCallback(data->context, codepoint); });

	glfwSetCursorEnterCallback(window, [](GLFWwindow* /*window*/, int entered) { RmlGLFW::ProcessCursorEnterCallback(data->context, entered); });

	// Mouse input
	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
		RmlGLFW::ProcessCursorPosCallback(data->context, window, xpos, ypos, data->glfw_active_modifiers);
	});

	glfwSetMouseButtonCallback(window, [](GLFWwindow* /*window*/, int button, int action, int mods) {
		data->glfw_active_modifiers = mods;
		RmlGLFW::ProcessMouseButtonCallback(data->context, button, action, mods);
	});

	glfwSetScrollCallback(window, [](GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
		RmlGLFW::ProcessScrollCallback(data->context, yoffset, data->glfw_active_modifiers);
	});

	// Window events
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* /*window*/, int width, int height) {
		data->render_interface.SetViewport(width, height);
		RmlGLFW::ProcessFramebufferSizeCallback(data->context, width, height);
	});

	glfwSetWindowContentScaleCallback(window,
		[](GLFWwindow* /*window*/, float xscale, float /*yscale*/) { RmlGLFW::ProcessContentScaleCallback(data->context, xscale); });
}
