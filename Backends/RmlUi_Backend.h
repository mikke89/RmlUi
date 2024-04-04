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

/// @brief This class defines information for not initializating renderers fully. Fully initialization means that renderer on RmlUi side will
/// initialize device, queues and other things by its own (in case of OpenGL it will load functions & will init OpenGL) so in case where you already
/// have own renderer and that renderer is initialized you want to prevent a such initialization and thus RmlUi needs to know user data.
class RmlRenderInitInfo {
public:
	/// @brief This is constructor for initializing class
	/// @param p_user_device ID3D12Device raw pointer (e.g. ID3D12Device* don't pass ComPtr instance!) or you pass VkDevice instance.
	RmlRenderInitInfo(void* p_window_handle, void* p_user_device, bool use_vsync) :
		m_is_full_initialization{true}, m_is_use_vsync{use_vsync}, m_p_native_window_handle{p_window_handle}, m_p_user_device{p_user_device}
	{
		RMLUI_ASSERT(p_user_device &&
			"if you want to initialize renderer by system you don't need to pass this parameter as nullptr just use second constructor and set "
			"is_full_initialization to true!");
	}

	/// @brief This is constructor for older graphics API where don't require Devices, Queues etc...
	/// @param p_window_handle HWND or similar types to OS's window handle
	RmlRenderInitInfo(void* p_window_handle, bool is_full_initialization, bool use_vsync) :
		m_is_full_initialization{is_full_initialization}, m_is_use_vsync{use_vsync}, m_p_native_window_handle{p_window_handle}, m_p_user_device{}
	{}

	~RmlRenderInitInfo() {}

	/// @brief Returns hidden type of ID3D12Device* or VkDevice; It is user's responsibility to pass a valid type for it otherwise system will be
	/// broken and renderer couldn't be initialized!
	/// @return you need to use reinterpret_cast to ID3D12Device* type or VkDevice. Depends on what user wants to initialize.
	void* Get_UserDevice() { return this->m_p_user_device; }

	/// @brief Returns hidden type of native window handle (on Windows e.g. HWND)
	/// @return window native type of handle  (HWND etc depends on OS)
	void* Get_WindowHandle() { return this->m_p_native_window_handle; }

	/// @brief returns flag of full initialization. It means that if user wants full initialization that system will create and initialize GAPI and
	/// renderer by its own. In case of OpenGL system loads functions and initialize renderer, io case of DirectX 12 and Vulkan system initializes
	/// Device, Queues and etc. Otherwise it is not full initialization and user passed own Device, Queues or set is_full_initialize to false in
	/// the second constructor for older GAPIs like OpenGL
	/// @param nothing, because it is getter
	/// @return field of class m_is_full_initialization if it is true it means that user didn't passed Device, Queues from user's renderer or set
	/// is_full_initialization to true
	bool Is_FullInitialization(void) const { return this->m_is_full_initialization; }

	bool Is_UseVSync() const { return this->m_is_use_vsync; }

private:
	bool m_is_full_initialization;
	/// @brief use vsync feature of render api (if supports at all); true = means enable feature; false = disable feature.
	bool m_is_use_vsync;
	/// @brief it is user's handle of OS's handle, so if it is Windows you pass HWND* here
	void* m_p_native_window_handle;
	/// @brief Type that hides ID3D12Device*, VkDevice or something else depends on context and which renderer user initializes
	void* m_p_user_device;
};

// Initializes the backend, including the custom system and render interfaces, and opens a window for rendering the RmlUi context.
bool Initialize(const char* window_name, int width, int height, bool allow_resize, RmlRenderInitInfo* p_info = nullptr);
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
// Presents the rendered frame to the screen, call after rendering the RmlUi context.
void PresentFrame();

} // namespace Backend

#endif
