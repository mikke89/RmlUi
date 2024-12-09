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
#include "RmlUi_Include_Windows.h"
#include "RmlUi_Platform_Win32.h"
#include "RmlUi_Renderer_DX11.h"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Profiling.h>

#ifdef RMLUI_USE_STB_IMAGE_LOADER
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
    #include <RmlUi/Core/FileInterface.h>
#endif

/**
    Custom render interface example for the DX11/Win32 backend.

    Overloads the DX11 render interface to load textures through stb_image.
 */
class RenderInterface_DX11_Win32 : public RenderInterface_DX11 {
public:
    RenderInterface_DX11_Win32() {}

#ifdef RMLUI_USE_STB_IMAGE_LOADER
    Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override
    {
        int texture_width = 0, texture_height = 0, num_channels = 0;
        size_t image_size_bytes = 0;
        uint8_t* texture_data = nullptr;

        // Find where on disk the file is
        Rml::FileInterface* file_interface = Rml::GetFileInterface();
        Rml::FileHandle file_handle = file_interface->Open(source);
        if (!file_handle)
        {
            // Tell RmlUI that the image is invalid
            texture_data = nullptr;
            return false;
        }

        // Load the file through stb_image
        texture_data = stbi_load_from_file((FILE*)file_handle, &texture_width, &texture_height, &num_channels, 0);

        // If the file data is correct
        if (texture_data != nullptr)
        {
            // Compute number of elements in texture
            image_size_bytes = texture_width * texture_height * num_channels;

            // Pre-multiply the data
            for (int i = 0; i < image_size_bytes; i += 4)
            {
                texture_data[i + 0] = (uint8_t)((texture_data[i + 0] * texture_data[i + 3]) / 255);
                texture_data[i + 1] = (uint8_t)((texture_data[i + 1] * texture_data[i + 3]) / 255);
                texture_data[i + 2] = (uint8_t)((texture_data[i + 2] * texture_data[i + 3]) / 255);
            }

            texture_dimensions.x = texture_width;
            texture_dimensions.y = texture_height;

            Rml::TextureHandle handle = GenerateTexture({texture_data, image_size_bytes}, texture_dimensions);

            stbi_image_free(texture_data);
            return handle;
        }
        return false;
    }
#endif
};

/**
High DPI support using Windows Per Monitor V2 DPI awareness.

Requires Windows 10, version 1703.
*/
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

// Declare pointers to the DPI aware Windows API functions.
using ProcSetProcessDpiAwarenessContext = BOOL(WINAPI*)(HANDLE value);
using ProcGetDpiForWindow = UINT(WINAPI*)(HWND hwnd);
using ProcAdjustWindowRectExForDpi = BOOL(WINAPI*)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);

static bool has_dpi_support = false;
static ProcSetProcessDpiAwarenessContext procSetProcessDpiAwarenessContext = NULL;
static ProcGetDpiForWindow procGetDpiForWindow = NULL;
static ProcAdjustWindowRectExForDpi procAdjustWindowRectExForDpi = NULL;

// Make ourselves DPI aware on supported Windows versions.
static void InitializeDpiSupport()
{
    // Cast function pointers to void* first for MinGW not to emit errors.
    procSetProcessDpiAwarenessContext =
    (ProcSetProcessDpiAwarenessContext)(void*)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "SetProcessDpiAwarenessContext");
    procGetDpiForWindow = (ProcGetDpiForWindow)(void*)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "GetDpiForWindow");
    procAdjustWindowRectExForDpi =
    (ProcAdjustWindowRectExForDpi)(void*)GetProcAddress(GetModuleHandle(TEXT("User32.dll")), "AdjustWindowRectExForDpi");

    if (!has_dpi_support && procSetProcessDpiAwarenessContext != NULL && procGetDpiForWindow != NULL && procAdjustWindowRectExForDpi != NULL)
    {
        // Activate Per Monitor V2.
        if (procSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        has_dpi_support = true;
    }
}

static UINT GetWindowDpi(HWND window_handle)
{
    if (has_dpi_support)
    {
        UINT dpi = procGetDpiForWindow(window_handle);
        if (dpi != 0)
        return dpi;
    }
    return USER_DEFAULT_SCREEN_DPI;
}

static float GetDensityIndependentPixelRatio(HWND window_handle)
{
    return float(GetWindowDpi(window_handle)) / float(USER_DEFAULT_SCREEN_DPI);
}

// Create the window but don't show it yet. Returns the pixel size of the window, which may be different than the passed size due to DPI settings.
static HWND InitializeWindow(HINSTANCE instance_handle, const std::wstring& name, int& inout_width, int& inout_height, bool allow_resize);

// D3D Creation / Cleanup functions
static bool CreateDeviceD3D(HWND hwnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();

/**
Global data used by this backend.

Lifetime governed by the calls to Backend::Initialize() and Backend::Shutdown().
*/
struct BackendData {
    SystemInterface_Win32 system_interface;
    RenderInterface_DX11_Win32 render_interface;
    TextInputMethodEditor_Win32 text_input_method_editor;

    HINSTANCE instance_handle = nullptr;
    std::wstring instance_name;
    HWND window_handle = nullptr;

    // D3D11 resources
    struct {
        ID3D11Device* pd3dDevice = nullptr;
        ID3D11DeviceContext* pd3dDeviceContext = nullptr;
        IDXGISwapChain* pSwapChain = nullptr;
        bool swapchain_occluded = false;
        ID3D11RenderTargetView* pMainRenderTargetView = nullptr;
    } d3d_resources;

    bool context_dimensions_dirty = true;
    Rml::Vector2i window_dimensions;
    Rml::Vector2i resize_dimensions;
    bool running = true;

    // Arguments set during event processing and nulled otherwise.
    Rml::Context* context = nullptr;
    KeyDownCallback key_down_callback = nullptr;
};
static Rml::UniquePtr<BackendData> data;

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize)
{
    RMLUI_ASSERT(!data);

    const std::wstring name = RmlWin32::ConvertToUTF16(Rml::String(window_name));

    data = Rml::MakeUnique<BackendData>();
    data->instance_handle = GetModuleHandle(nullptr);
    data->instance_name = name.data();

    InitializeDpiSupport();

    // Initialize the window but don't show it yet.
    HWND window_handle = InitializeWindow(data->instance_handle, name, width, height, allow_resize);
    if (!window_handle)
        return false;

    if (!CreateDeviceD3D(window_handle)) {
        ::CloseWindow(window_handle);
        return false;
    }

#ifdef RMLUI_USE_STB_IMAGE_LOADER
    // Disable pre-multiplication because loading images through stb_image will inconsistently return pre-multiplied images (only iPhone PNGs are pre-multipled)
    // We handle pre-multiplication manually on decode
    stbi_set_unpremultiply_on_load(true);
    // iPhone PNGs are encoded as BGRA, this tells stb_image to unpack as RGBA
    stbi_convert_iphone_png_to_rgb(true);
#endif

    data->render_interface.Init(data->d3d_resources.pd3dDevice);

    data->window_handle = window_handle;
    data->system_interface.SetWindow(window_handle);

    // Now we are ready to show the window.
    ::ShowWindow(window_handle, SW_SHOW);
    ::SetForegroundWindow(window_handle);
    ::SetFocus(window_handle);

    // Provide a backend-specific text input handler to manage the IME.
    Rml::SetTextInputHandler(&data->text_input_method_editor);

    return true;
}

void Backend::Shutdown()
{
    RMLUI_ASSERT(data);

    // As we forcefully override the global text input handler, we must reset it before the data is destroyed to avoid any potential use-after-free.
    if (Rml::GetTextInputHandler() == &data->text_input_method_editor)
        Rml::SetTextInputHandler(nullptr);

    // Cleanup renderer resources
    data->render_interface.Cleanup();

    // Shutdown DirectX11
    CleanupDeviceD3D();

    ::DestroyWindow(data->window_handle);
    ::UnregisterClassW((LPCWSTR)data->instance_name.data(), data->instance_handle);

    data.reset();
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

static bool NextEvent(MSG& message, UINT timeout)
{
    if (timeout != 0)
    {
        UINT_PTR timer_id = SetTimer(NULL, 0, timeout, NULL);
        BOOL res = GetMessage(&message, NULL, 0, 0);
        KillTimer(NULL, timer_id);
        if (message.message != WM_TIMER || message.hwnd != nullptr || message.wParam != timer_id)
        return res;
    }
    return PeekMessage(&message, nullptr, 0, 0, PM_REMOVE);
}

bool Backend::ProcessEvents(Rml::Context* context, KeyDownCallback key_down_callback, bool power_save)
{
    RMLUI_ASSERT(data && context);

    // The initial window size may have been affected by system DPI settings, apply the actual pixel size and dp-ratio to the context.
    if (data->context_dimensions_dirty)
    {
        data->context_dimensions_dirty = false;
        const float dp_ratio = GetDensityIndependentPixelRatio(data->window_handle);
        context->SetDimensions(data->window_dimensions);
        context->SetDensityIndependentPixelRatio(dp_ratio);
    }

    data->context = context;
    data->key_down_callback = key_down_callback;

    MSG message;
    bool has_message = NextEvent(message, power_save ? static_cast<int>(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0) * 1000.0) : 0);
    while (has_message)
    {
        // Dispatch the message to our local event handler below.
        TranslateMessage(&message);
        DispatchMessage(&message);

        has_message = NextEvent(message, 0);
    }

    data->context = nullptr;
    data->key_down_callback = nullptr;

    const bool result = data->running;
    data->running = true;
    return result;
}

void Backend::RequestExit()
{
    RMLUI_ASSERT(data);
    data->running = false;
}

void Backend::BeginFrame()
{
    RMLUI_ASSERT(data);

    // Handle window resize (we don't resize directly in the WM_SIZE handler)
    if (data->resize_dimensions.x != 0 && data->resize_dimensions.y != 0)
    {
        CleanupRenderTarget();
        data->d3d_resources.pSwapChain->ResizeBuffers(0, data->resize_dimensions.x, data->resize_dimensions.y, DXGI_FORMAT_UNKNOWN, 0);
        data->resize_dimensions.x = data->resize_dimensions.y = 0;
        CreateRenderTarget();
    }

    data->render_interface.BeginFrame(data->d3d_resources.pMainRenderTargetView);
}

void Backend::PresentFrame()
{
    RMLUI_ASSERT(data);
    data->render_interface.EndFrame();

    // Present
    HRESULT hr = data->d3d_resources.pSwapChain->Present(1, 0); // Present with vsync
    //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
    data->d3d_resources.swapchain_occluded = (hr == DXGI_STATUS_OCCLUDED);

    // Optional, used to mark frames during performance profiling.
    RMLUI_FrameMark;
}

// Local event handler for window and input events.
static LRESULT CALLBACK WindowProcedureHandler(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    RMLUI_ASSERT(data);

    switch (message)
    {
        case WM_NCACTIVATE:
        case WM_NCPAINT:
            // Handle Windows Aero Snap
            return DefWindowProcW(window_handle, message, w_param, l_param);
        case WM_CLOSE:
        {
            data->running = false;
            return 0;
        }
        break;
        case WM_SIZE:
        {
            const int width = LOWORD(l_param);
            const int height = HIWORD(l_param);
            data->window_dimensions.x = width;
            data->window_dimensions.y = height;
            data->resize_dimensions.x = width;
            data->resize_dimensions.y = height;
            data->context_dimensions_dirty = true;
            data->render_interface.SetViewport(width, height);
            if (data->context)
                data->context->SetDimensions(data->window_dimensions);
            return 0;
        }
        break;
        case WM_DPICHANGED:
        {
            RECT* new_pos = (RECT*)l_param;
            SetWindowPos(window_handle, NULL, new_pos->left, new_pos->top, new_pos->right - new_pos->left, new_pos->bottom - new_pos->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            if (data->context && has_dpi_support)
                data->context->SetDensityIndependentPixelRatio(GetDensityIndependentPixelRatio(window_handle));
            return 0;
        }
        break;
        case WM_KEYDOWN:
        {
            // Override the default key event callback to add global shortcuts for the samples.
            Rml::Context* context = data->context;
            KeyDownCallback key_down_callback = data->key_down_callback;

            const Rml::Input::KeyIdentifier rml_key = RmlWin32::ConvertKey((int)w_param);
            const int rml_modifier = RmlWin32::GetKeyModifierState();
            const float native_dp_ratio = GetDensityIndependentPixelRatio(window_handle);

            // See if we have any global shortcuts that take priority over the context.
            if (key_down_callback && !key_down_callback(context, rml_key, rml_modifier, native_dp_ratio, true))
            return 0;
            // Otherwise, hand the event over to the context by calling the input handler as normal.
            if (!RmlWin32::WindowProcedure(context, data->text_input_method_editor, window_handle, message, w_param, l_param))
            return 0;
            // The key was not consumed by the context either, try keyboard shortcuts of lower priority.
            if (key_down_callback && !key_down_callback(context, rml_key, rml_modifier, native_dp_ratio, false))
            return 0;
            return 0;
        }
        break;
        default:
        {
            // Submit it to the platform handler for default input handling.
            if (!RmlWin32::WindowProcedure(data->context, data->text_input_method_editor, window_handle, message, w_param, l_param))
            return 0;
        }
        break;
    }

    // All unhandled messages go to DefWindowProc.
    return DefWindowProc(window_handle, message, w_param, l_param);
}

static HWND InitializeWindow(HINSTANCE instance_handle, const std::wstring& name, int& inout_width, int& inout_height, bool allow_resize)
{
    // Fill out the window class struct.
    WNDCLASSW window_class;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = &WindowProcedureHandler; // Attach our local event handler.
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = instance_handle;
    window_class.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = nullptr;
    window_class.lpszMenuName = nullptr;
    window_class.lpszClassName = name.data();

    if (!RegisterClassW(&window_class))
    {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to register window class");
        return nullptr;
    }

    HWND window_handle = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
                                         name.data(),                                                                // Window class name.
                                         name.data(), WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, 0, 0, // Window position.
                                         0, 0,                                                                       // Window size.
                                         nullptr, nullptr, instance_handle, nullptr);

    if (!window_handle)
    {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create window");
        return nullptr;
    }

    UINT window_dpi = GetWindowDpi(window_handle);
    inout_width = (inout_width * (int)window_dpi) / USER_DEFAULT_SCREEN_DPI;
    inout_height = (inout_height * (int)window_dpi) / USER_DEFAULT_SCREEN_DPI;

    DWORD style = (allow_resize ? WS_OVERLAPPEDWINDOW : (WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX));
    DWORD extended_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

    // Adjust the window size to take the edges into account.
    RECT window_rect;
    window_rect.top = 0;
    window_rect.left = 0;
    window_rect.right = inout_width;
    window_rect.bottom = inout_height;
    if (has_dpi_support)
    procAdjustWindowRectExForDpi(&window_rect, style, FALSE, extended_style, window_dpi);
    else
    AdjustWindowRectEx(&window_rect, style, FALSE, extended_style);

    SetWindowLong(window_handle, GWL_EXSTYLE, extended_style);
    SetWindowLong(window_handle, GWL_STYLE, style);

    // Resize the window and center it on the screen.
    Rml::Vector2i screen_size = {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    Rml::Vector2i window_size = {int(window_rect.right - window_rect.left), int(window_rect.bottom - window_rect.top)};
    Rml::Vector2i window_pos = Rml::Math::Max((screen_size - window_size) / 2, Rml::Vector2i(0));

    SetWindowPos(window_handle, HWND_TOP, window_pos.x, window_pos.y, window_size.x, window_size.y, SWP_NOACTIVATE);

    return window_handle;
}

static bool CreateDeviceD3D(HWND hwnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef RMLUI_DEBUG
    // Enable debug layer
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &data->d3d_resources.pSwapChain, &data->d3d_resources.pd3dDevice, &featureLevel, &data->d3d_resources.pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &data->d3d_resources.pSwapChain, &data->d3d_resources.pd3dDevice, &featureLevel, &data->d3d_resources.pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {

    CleanupRenderTarget();
    if (data->d3d_resources.pSwapChain)
    {
        data->d3d_resources.pSwapChain->Release();
        data->d3d_resources.pSwapChain = nullptr;
    }
    if (data->d3d_resources.pd3dDeviceContext)
    {
        data->d3d_resources.pd3dDeviceContext->Release();
        data->d3d_resources.pd3dDeviceContext = nullptr;
    }
    if (data->d3d_resources.pd3dDevice)
    {
        data->d3d_resources.pd3dDevice->Release();
        data->d3d_resources.pd3dDevice = nullptr;
    }
}

static void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    data->d3d_resources.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    data->d3d_resources.pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &data->d3d_resources.pMainRenderTargetView);
    pBackBuffer->Release();
}

static void CleanupRenderTarget()
{
    if (data->d3d_resources.pMainRenderTargetView)
    {
        data->d3d_resources.pMainRenderTargetView->Release();
        data->d3d_resources.pMainRenderTargetView = nullptr;
    }
}