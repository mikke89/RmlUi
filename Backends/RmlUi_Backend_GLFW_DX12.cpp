/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "RmlUi_Backend.h"
#include "RmlUi_Renderer_DX12.h"
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
	RenderInterface_DX12* render_interface;

	GLFWwindow* window = nullptr;
	Rml::Context* context = nullptr;
	KeyDownCallback key_down_callback = nullptr;
	Backend::RmlRenderInitInfo* p_default_init_info = nullptr;
	int glfw_active_modifiers = 0;
	bool running = true;
	bool context_dimensions_dirty = true;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize, RmlRenderInitInfo* p_info)
{
	RMLUI_ASSERT(!data);

	// todo: provide implementation for this thing please, for now temporary for fixing unused warning that treated as error
	if (p_info) { p_info = nullptr; }
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

	HWND window_handle = glfwGetWin32Window(window);
	
	if (!window_handle)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "GLFW failed to obtain win32 window handle! Failed to initailize renderer dx12!");
		return false;
	}

	if (!p_info)
	{
		p_info = new RmlRenderInitInfo(window_handle, true);
		p_info->Get_Settings().vsync = false;
		data->p_default_init_info = p_info;
	}

	data->render_interface = RmlDX12::Initialize(nullptr, p_info);

	if (!data->render_interface)
	{
		glfwDestroyWindow(data->window);
		glfwTerminate();
		data.reset();
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to initialize DirectX 12 render interface");
		return false;
	}

	data->system_interface.SetWindow(window);
	data->render_interface->SetViewport(width, height);

	// Receive num lock and caps lock modifiers for proper handling of numpad inputs in text fields.
	glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

	SetupCallbacks(window);

	return true;
}

void Backend::Shutdown()
{
	RMLUI_ASSERT(data);
	RMLUI_ASSERT(data->render_interface);

	if (data->p_default_init_info)
	{
		delete data->p_default_init_info;
		data->p_default_init_info = nullptr;
	}

	RmlDX12::Shutdown(data->render_interface);

	if (data->render_interface)
	{
		delete data->render_interface;
		data->render_interface = nullptr;
	}

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
	return data->render_interface;
}

static bool WaitForValidSwapchain()
{
	bool result = true;

	// In some situations the swapchain may become invalid, such as when the window is minimized. In this state the renderer cannot accept any render
	// calls. Since we don't have full control over the main loop here we may risk calls to Context::Render if we were to return. Instead, we keep the
	// application inside this loop until we are able to recreate the swapchain and render again.

	while (!data->render_interface->IsSwapchainValid())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (glfwWindowShouldClose(data->window))
		{
			glfwRestoreWindow(data->window);
			result = false;
		}

		glfwPollEvents();
		data->render_interface->RecreateSwapchain();
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
	data->render_interface->BeginFrame();
	data->render_interface->Clear();
}

void Backend::PresentFrame()
{
	RMLUI_ASSERT(data);
	data->render_interface->EndFrame();
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
		data->render_interface->SetViewport(width, height);
		RmlGLFW::ProcessFramebufferSizeCallback(data->context, width, height);
	});

	glfwSetWindowContentScaleCallback(window,
		[](GLFWwindow* /*window*/, float xscale, float /*yscale*/) { RmlGLFW::ProcessContentScaleCallback(data->context, xscale); });
}
