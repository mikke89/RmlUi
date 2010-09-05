/*
 * Copyright (c) 2006 - 2008
 * Wandering Monster Studios Limited
 *
 * Any use of this program is governed by the terms of Wandering Monster
 * Studios Limited's Licence Agreement included with this program, a copy
 * of which can be obtained by contacting Wandering Monster Studios
 * Limited at info@wanderingmonster.co.nz.
 *
 */

#include <Rocket/Core.h>
#include <Rocket/Debugger.h>
#include <Input.h>
#include <Shell.h>
#include "RenderInterfaceDirectX.h"
#include <d3d9.h>
#include <d3dx9.h>

static LPDIRECT3D9 g_pD3D = NULL;
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;

static Rocket::Core::Context* context = NULL;

bool InitialiseDirectX()
{
	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_pD3D == NULL)
		return false;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
									D3DDEVTYPE_HAL,
									(HWND) Shell::GetWindowHandle(),
									D3DCREATE_SOFTWARE_VERTEXPROCESSING,
									&d3dpp,
									&g_pd3dDevice)))
	{
		return false;
	}

	// Set up an orthographic projection.
	D3DXMATRIX projection;
	D3DXMatrixOrthoOffCenterLH(&projection, 0, 1024, 768, 0, -1, 1);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &projection);

	// Switch to clockwise culling instead of counter-clockwise culling; Rocket generates counter-clockwise geometry,
	// so you can either reverse the culling mode when Rocket is rendering, or reverse the indices in the render
	// interface.
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	// Enable alpha-blending for Rocket.
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	// Set up the texture stage states for the diffuse texture.
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

	g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	// Disable lighting for Rocket.
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	return true;
}

void ShutdownDirectX()
{
	if (g_pd3dDevice != NULL)
	{
		// Release the last resources we bound to the device.
		g_pd3dDevice->SetTexture(0, NULL);
		g_pd3dDevice->SetStreamSource(0, NULL, 0, 0);
		g_pd3dDevice->SetIndices(NULL);

		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}

	if (g_pD3D != NULL)
	{
		g_pD3D->Release();
		g_pD3D = NULL;
	}
}

void GameLoop()
{
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	g_pd3dDevice->BeginScene();

	context->Update();
	context->Render();

	g_pd3dDevice->EndScene();
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

#if defined EMP_PLATFORM_WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE EMP_UNUSED(instance_handle), HINSTANCE EMP_UNUSED(previous_instance_handle), char* EMP_UNUSED(command_line), int EMP_UNUSED(command_show))
#else
int main(int EMP_UNUSED(argc), char** EMP_UNUSED(argv))
#endif
{
	// Generic OS initialisation, creates a window and does not attach OpenGL.
	if (!Shell::Initialise("../../Samples/basic/directx/") ||
		!Shell::OpenWindow("DirectX Sample", false))
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
	RenderInterfaceDirectX directx_renderer(g_pD3D, g_pd3dDevice);
	Rocket::Core::SetRenderInterface(&directx_renderer);

	ShellSystemInterface system_interface;
	Rocket::Core::SetSystemInterface(&system_interface);

	Rocket::Core::Initialise();

	// Create the main Rocket context and set it on the shell's input layer.
	context = Rocket::Core::CreateContext("main", EMP::Core::Vector2i(1024, 768));
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
