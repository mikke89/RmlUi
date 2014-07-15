#include <Rocket/Core.h>
#include <Rocket/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include "RenderInterfaceDirectX10.h"

//Our device for this sample
static ID3D10Device * pD3D10Device=NULL;
//Swap Chain
static IDXGISwapChain * pSwapChain=NULL;
//Render Target
static ID3D10RenderTargetView * pRenderTargetView=NULL;

static Rocket::Core::Context* context = NULL;

bool InitialiseDirectX()
{
	//get the size of the window
	RECT windowRect;
	GetClientRect((HWND) Shell::GetWindowHandle(),&windowRect);
	UINT width=windowRect.right-windowRect.left;
	UINT height=windowRect.bottom-windowRect.top;

	//put the device into debug if we are in a debug build
	UINT createDeviceFlags=0;
#ifdef _DEBUG
	createDeviceFlags|=D3D10_CREATE_DEVICE_DEBUG;
#endif

	//Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount=1;
	sd.OutputWindow = (HWND) Shell::GetWindowHandle();
	sd.Windowed = TRUE;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	//Create device and swapchain
	if (FAILED(D3D10CreateDeviceAndSwapChain(NULL, 
		D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
		D3D10_SDK_VERSION, &sd, &pSwapChain,
		&pD3D10Device)))
		return false;
	
	//Create Render Target
	ID3D10Texture2D *pBackBuffer;
	if ( FAILED (pSwapChain->GetBuffer(0,
		__uuidof(ID3D10Texture2D),
		(void**)&pBackBuffer)))
		return false;
	if (FAILED(pD3D10Device->CreateRenderTargetView( pBackBuffer,
		NULL,
		&pRenderTargetView ))){
			pBackBuffer->Release();
			return false;
	}
	pBackBuffer->Release();
	
	pD3D10Device->OMSetRenderTargets(1,&pRenderTargetView,NULL);

	D3D10_VIEWPORT vp;
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pD3D10Device->RSSetViewports( 1, &vp );

	return true;
}

void ShutdownDirectX()
{	
	if (pD3D10Device)
		pD3D10Device->ClearState();
	if (pRenderTargetView)
		pRenderTargetView->Release();
	if (pSwapChain)
		pSwapChain->Release();
	if (pD3D10Device)
		pD3D10Device->Release();
}

void GameLoop()
{
	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	pD3D10Device->ClearRenderTargetView( pRenderTargetView,ClearColor );

	context->Update();
	context->Render();

	pSwapChain->Present( 0, 0 );
}

#include <windows.h>
int APIENTRY WinMain(HINSTANCE ROCKET_UNUSED_PARAMETER(instance_handle), HINSTANCE ROCKET_UNUSED_PARAMETER(previous_instance_handle), char* ROCKET_UNUSED_PARAMETER(command_line), int ROCKET_UNUSED_PARAMETER(command_show))
{
	ROCKET_UNUSED(instance_handle);
	ROCKET_UNUSED(previous_instance_handle);
	ROCKET_UNUSED(command_line);
	ROCKET_UNUSED(command_show);

	// Generic OS initialisation, creates a window and does not attach OpenGL.
	if (!Shell::Initialise("../Samples/basic/directx/") ||
		!Shell::OpenWindow("DirectX 10 Sample", false))
	{
		Shell::Shutdown();
		return -1;
	}

	// DirectX initialisation.
	if (!InitialiseDirectX())
	{
		Shell::CloseWindow();
		Shell::Shutdown();

		return -1;
	}

	// Install our DirectX render interface into Rocket.
	RenderInterfaceDirectX10 directx_renderer(pD3D10Device,1024,768);
	Rocket::Core::SetRenderInterface(&directx_renderer);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(1024, 768));
	if (context == NULL)
	{
		Rocket::Core::Shutdown();
		Shell::Shutdown();
		return -1;
	}

	Rocket::Debugger::Initialise(context);
	Input::SetContext(context);

	Shell::LoadFonts("../../assets/");

	// Load and show the tutorial document.
	Rocket::Core::ElementDocument* document = context->LoadDocument("../../assets/demo.rml");
	if (document != NULL)
	{
		document->Show();
		document->RemoveReference();
	}

	Shell::EventLoop(GameLoop);

	// Shutdown Rocket.
	context->RemoveReference();
	Rocket::Core::Shutdown();

	ShutdownDirectX();

	Shell::CloseWindow();
	Shell::Shutdown();

	return 0;
}
