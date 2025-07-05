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

#ifndef RMLUI_BACKENDS_BACKEND_H
#define RMLUI_BACKENDS_BACKEND_H

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>

using KeyDownCallback = bool (*)(Rml::Context* context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority);

/**
    This interface serves as a basic abstraction over the various backends included with RmlUi. It is mainly intended as an example to get something
    simple up and running, and provides just enough functionality for the included samples.

    This interface may be used directly for simple applications and testing. However, for anything more advanced we recommend to use the backend as a
    starting point and copy relevant parts into the main loop of your application. On the other hand, the underlying platform and renderer used by the
    backend are intended to be re-usable as is.
 */
namespace Backend {

enum class Type : unsigned char { Unknown, DirectX_12, DirectX_11, DirectX_10, DirectX_9, Vulkan, OpenGL_3, OpenGL_2, EGL, Metal };

enum class TypeSystemInterface : unsigned char {
	Native_Unknown,
	/// @brief RmlRenderInitInfo::p_native_window_handle requires as HWND
	Native_Win32,
	/// @brief todo mikke89: finish this please
	Native_Linux,
	/// @brief todo mikke89: finish this please
	Native_MacOS,
	/// @brief RmlRenderInitInfo::p_native_window_handle requires as SDL_Window*
	Library_SDL2,
	/// @brief RmlRenderInitInfo::p_native_window_handle requires as SDL_Window*
	Library_SDL3,
	/// @brief RmlRenderInitInfo::p_native_window_handle requires as GLFWWindow*
	Library_GLFW,
	/// @brief don't touch system
	Library_Unknown,
};

struct RmlRendererSettings {
	bool vsync{};
	unsigned char msaa_sample_count{};
};

struct RmlRenderInitInfo {
	bool is_full_initialization;
	bool is_execute_when_end_frame_issued;
	unsigned char backend_type;
	unsigned char system_interface_type;
	/// @brief uses for initializing Rml::Context
	int initial_width;
	/// @brief uses for initializing Rml::Context
	int initial_height;
	/// @brief it is user's handle of OS's handle, so if it is Windows you pass HWND* here
	void* p_native_window_handle;
	/// @brief Type that hides ID3D12Device*, VkDevice or something else depends on context and which renderer user initializes
	void* p_user_device;
	/// @brief
	void* p_user_adapter;
	/// @brief DX12 = ID3D12GraphicsCommandList*;
	void* p_command_list;
	/// @brief just name your context please
	char context_name[32];
	RmlRendererSettings settings;
};

// Initializes the backend, including the custom system and render interfaces, and opens a window for rendering the RmlUi context.
bool Initialize(const char* window_name, int width, int height, bool allow_resize, RmlRenderInitInfo* p_info = nullptr);

/// @brief uses for initializing render backend from rmlui and integrate to your solution by passing the init info about your backend
/// @param p_info init info where you have to specify some parameters for correct initialization of backend
/// @return true means backend was initialized otherwise it is failed
Rml::Context* Initialize(RmlRenderInitInfo* p_info);

void Resize(Rml::Context* p_context, int width, int height);

// Closes the window and release all resources owned by the backend, including the system and render interfaces.
void Shutdown();

// Returns a pointer to the custom system interface which should be provided to RmlUi.
Rml::SystemInterface* GetSystemInterface();
// Returns a pointer to the custom render interface which should be provided to RmlUi.
Rml::RenderInterface* GetRenderInterface();

// Polls and processes events from the current platform, and applies any relevant events to the provided RmlUi context and the key down callback.
// @return False to indicate that the application should be closed.
bool ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback = nullptr, bool power_save = false);
// Request application closure during the next event processing call.
void RequestExit();

// Prepares the render state to accept rendering commands from RmlUi, call before rendering the RmlUi context.
void BeginFrame();

/// @brief Each call begin frame you pass appropriate image where result will be renderer into that resource what you passed, this function for
/// integration situation and you must not use when you use shell initialization route (BeginFrame without arguments version, you must not call it)
/// @param p_input_image_resource DX12 = *; DX11 = *; VK =
void BeginFrame(void* p_input_rtv, void* p_input_dsv, unsigned char current_framebuffer_index);

// Presents the rendered frame to the screen, call after rendering the RmlUi context.
void PresentFrame();

/// @brief when you use integration you have to call this method instead of PresentFrame(void)
/// @param user_next_backbuffer_image
void EndFrame();

/// @brief not for user, internal usage
extern Type ___renderer_type;

/* NOT FOR USER, maybe add prefix ___ like with renderer_type for defining that it is directly NOT FOR USER ??? */
namespace DX12 {
	Rml::Context* Initialize(RmlRenderInitInfo* p_info, Rml::SystemInterface* p_system_interface);
	void Shutdown();
	void BeginFrame(void* p_input_rtv, void* p_input_dsv, unsigned char current_framebuffer_index);
	void Resize(Rml::Context* p_context, int width, int height);
	void EndFrame();
} // namespace DX12

namespace DX11 {
	bool Initialize(RmlRenderInitInfo* p_info);
	void Shutdown();
	void BeginFrame(void* p_input_image_resource);
	void EndFrame();
} // namespace DX11

namespace DX10 {
	bool Initialize(RmlRenderInitInfo* p_info);
	void Shutdown();
	void BeginFrame(void* p_input_image_resource);
	void EndFrame();
} // namespace DX10

namespace DX9 {
	bool Initialize(RmlRenderInitInfo* p_info);
	void Shutdown();
	void BeginFrame(void* p_input_image_resource);
	void EndFrame();
} // namespace DX9

namespace VK {
	bool Initialize(RmlRenderInitInfo* p_info);
	void Shutdown();
	void BeginFrame(void* p_input_image_resource);
	void EndFrame();
} // namespace VK

namespace GL3 {
	bool Initialize(RmlRenderInitInfo* p_info);
	void Shutdown();
	void BeginFrame(void* p_input_image_resource);
	void EndFrame();
} // namespace GL3

namespace GL2 {
	bool Initialize(RmlRenderInitInfo* p_info);
	void Shutdown();
	void BeginFrame(void* p_input_image_resource);
	void EndFrame();
} // namespace GL2

namespace Metal {
	bool Initialize(RmlRenderInitInfo* p_info);
	void Shutdown();
	void BeginFrame(void* p_input_image_resource);
	void EndFrame();
} // namespace Metal

} // namespace Backend

#endif
