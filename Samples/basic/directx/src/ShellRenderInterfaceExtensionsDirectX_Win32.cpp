/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2014 David Wimsey
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

#include "RenderInterfaceDirectX.h"
#include <Rocket/Core.h>
#include <d3dx9.h>

void RenderInterfaceDirectX::SetContext(void *context)
{
	m_rocket_context = context;
}

void RenderInterfaceDirectX::SetViewport(int width, int height)
{
	Rocket::Core::Matrix4f rocket_projection;

	if(g_pd3dDevice != NULL)
	{
		D3DXMATRIX projection;
		D3DXMatrixOrthoOffCenterLH(&projection, 0, (float)width, (float)height, 0, -1, 1);
		g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &projection);
		rocket_projection = projection;
		rocket_projection = rocket_projection.Transpose();

		if(m_rocket_context != NULL)
		{
			((Rocket::Core::Context*)m_rocket_context)->SetDimensions(Rocket::Core::Vector2i(width, height));
			((Rocket::Core::Context*)m_rocket_context)->ProcessProjectionChange();
			//((Rocket::Core::Context*)m_rocket_context)->ProcessViewChange();
		}
	}
}

bool RenderInterfaceDirectX::AttachToNative(void *nativeWindow)
{
	RECT clientRect;
	if(!GetClientRect((HWND) nativeWindow, &clientRect))
	{
		// if we can't lookup the client rect, abort, something is seriously wrong
		return false;
	}

	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_pD3D == NULL)
		return false;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_pd3dDevice = NULL;

	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
									D3DDEVTYPE_HAL,
									(HWND) nativeWindow,
									D3DCREATE_SOFTWARE_VERTEXPROCESSING,
									&d3dpp,
									&g_pd3dDevice)))
	{

		this->DetachFromNative();
		return false;
	}

	// Set up an orthographic projection.
	D3DXMATRIX projection;
	D3DXMatrixOrthoOffCenterLH(&projection, 0, (FLOAT)clientRect.right, (FLOAT)clientRect.bottom, 0, -1, 1);

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

void RenderInterfaceDirectX::DetachFromNative()
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

void RenderInterfaceDirectX::PrepareRenderBuffer()
{
	if(g_pd3dDevice != NULL)
	{
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		g_pd3dDevice->BeginScene();
	}
}

void RenderInterfaceDirectX::PresentRenderBuffer()
{
	if(g_pd3dDevice != NULL)
	{
		g_pd3dDevice->EndScene();
		g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	}
}
