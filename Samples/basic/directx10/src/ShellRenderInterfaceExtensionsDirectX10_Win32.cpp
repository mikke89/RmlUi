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

#include "RenderInterfaceDirectX10.h"
#include <Rocket/Core.h>
#include <d3d10.h>
#include <d3dx10.h>

// For _T unicode/mbcs macro
#include <tchar.h>

void RenderInterfaceDirectX10::SetContext(void *context)
{
	m_rocket_context = context;
}

void RenderInterfaceDirectX10::SetViewport(int width, int height)
{
	if(this->m_pD3D10Device != NULL)
	{
		if(width == 0 || height == 0)
		{
			// Windows with no client area cause crashes
			return;
		}

		if(this->m_pRenderTargetView)
		{
			// Release the existing render target
			this->m_pRenderTargetView->Release();
			this->m_pRenderTargetView = NULL;
		}

		// Resize the swap chain's buffer to the given dimensions
		m_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

		// Recreate Render Target
		ID3D10Texture2D *pBackBuffer;
		if(FAILED(this->m_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*) &pBackBuffer)))
		{
			MessageBox(NULL, _T("SwapChain->GetBuffer failed."), _T("Could not resize DirectX 10 surface"), MB_OK|MB_ICONERROR);
			return;
		}
		if(FAILED(this->m_pD3D10Device->CreateRenderTargetView(pBackBuffer, NULL, &this->m_pRenderTargetView)))
		{
				pBackBuffer->Release();
				MessageBox(NULL, _T("D3D10Device->CreateRenderTargetView failed."), _T("Could not resize DirectX 10 surface"), MB_OK|MB_ICONERROR);
				return;
		}
		pBackBuffer->Release();
	
		this->m_pD3D10Device->OMSetRenderTargets(1, &this->m_pRenderTargetView, NULL);

		D3D10_VIEWPORT vp;
		vp.Width = width;
		vp.Height = height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		this->m_pD3D10Device->RSSetViewports(1, &vp);

		// Recreate our view and projection matrix
		D3DXMatrixOrthoOffCenterLH(&this->m_matProjection, 0, width, height, 0, -1, 1);
		m_pProjectionMatrixVariable->SetMatrix((float*)this->m_matProjection);

		if(m_rocket_context != NULL)
		{
			((Rocket::Core::Context*)m_rocket_context)->SetDimensions(Rocket::Core::Vector2i(width, height));
			Rocket::Core::Matrix4f mat;
			mat = m_matProjection;
			mat = mat.Transpose();
			((Rocket::Core::Context*)m_rocket_context)->ProcessProjectionChange(mat);
			mat = m_matWorld;
			mat = mat.Transpose();
			((Rocket::Core::Context*)m_rocket_context)->ProcessViewChange(mat);
		}
	}
}

bool RenderInterfaceDirectX10::AttachToNative(void *nativeWindow)
{
	RECT clientRect;
	if(!GetClientRect((HWND) nativeWindow, &clientRect))
	{
		// if we can't lookup the client rect, abort, something is seriously wrong
		return false;
	}
	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;

	//put the device into debug if we are in a debug build
	UINT createDeviceFlags=0;
#ifdef _DEBUG
	createDeviceFlags|=D3D10_CREATE_DEVICE_DEBUG;
#endif

	//Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount=1;
	sd.OutputWindow = (HWND) nativeWindow;
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
	if(FAILED(D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,	D3D10_SDK_VERSION, &sd, &this->m_pSwapChain, &this->m_pD3D10Device)))
	{
		if(MessageBox(NULL, _T("D3D10CreateDeviceAndSwapChain failed for D3D10_DRIVER_TYPE_HARDWARE.\r\n\r\nWould you like to try the reference renderer, this will be very slow!"), _T("Could not intialized DirectX 10"), MB_OKCANCEL|MB_ICONERROR) == IDOK)
		{
			if(FAILED(D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, createDeviceFlags,	D3D10_SDK_VERSION, &sd, &this->m_pSwapChain, &this->m_pD3D10Device)))
			{
				MessageBox(NULL, _T("D3D10CreateDeviceAndSwapChain failed for D3D10_DRIVER_TYPE_REFERENCE, giving up."), _T("Could not intialized DirectX 10"), MB_OK|MB_ICONERROR);
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	
	//Create Render Target
	ID3D10Texture2D *pBackBuffer;
	if(FAILED (this->m_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D),(void**)&pBackBuffer)))
	{
		MessageBox(NULL, _T("SwapChain->GetBuffer failed."), _T("Could not intialized DirectX 10"), MB_OK|MB_ICONERROR);
		return false;
	}
	if(FAILED(this->m_pD3D10Device->CreateRenderTargetView(pBackBuffer, NULL, &this->m_pRenderTargetView)))
	{
			pBackBuffer->Release();
			MessageBox(NULL, _T("D3D10Device->CreateRenderTargetView failed."), _T("Could not intialized DirectX 10"), MB_OK|MB_ICONERROR);
			return false;
	}
	pBackBuffer->Release();
	
	this->m_pD3D10Device->OMSetRenderTargets(1, &this->m_pRenderTargetView, NULL);

	D3D10_VIEWPORT vp;
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	this->m_pD3D10Device->RSSetViewports(1, &vp);

	setupEffect();

	//Create our view and projection matrix
	D3DXMatrixOrthoOffCenterLH(&this->m_matProjection, 0, width, height, 0, -1, 1);
	m_pProjectionMatrixVariable->SetMatrix((float*)this->m_matProjection);

	//Create scissor raster states
	D3D10_RASTERIZER_DESC rasterDesc;
	rasterDesc.FillMode=D3D10_FILL_SOLID;
	rasterDesc.CullMode=D3D10_CULL_NONE;
	rasterDesc.ScissorEnable=TRUE;
	rasterDesc.FrontCounterClockwise=TRUE;
	if(FAILED(this->m_pD3D10Device->CreateRasterizerState(&rasterDesc, &this->m_pScissorTestEnable)))
	{
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Can't create Raster State - ScissorEnable");
	}

	rasterDesc.ScissorEnable=FALSE;
	if(FAILED(this->m_pD3D10Device->CreateRasterizerState(&rasterDesc, &this->m_pScissorTestDisable)))
	{
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Can't create Raster State - ScissorDisable");
	}

	return true;
}

void RenderInterfaceDirectX10::DetachFromNative()
{
	if(this->m_pD3D10Device != NULL)
	{
		this->m_pD3D10Device->ClearState();
		this->m_pD3D10Device = NULL;
	}
	if(this->m_pRenderTargetView != NULL)
	{
		this->m_pRenderTargetView->Release();
		this->m_pRenderTargetView = NULL;
	}
	if(this->m_pSwapChain != NULL)
	{
		this->m_pSwapChain->Release();
		this->m_pSwapChain = NULL;
	}
	if(this->m_pD3D10Device != NULL)
	{
		this->m_pD3D10Device->Release();
		this->m_pD3D10Device = NULL;
	}

}

void RenderInterfaceDirectX10::PrepareRenderBuffer()
{
	if(this->m_pD3D10Device == NULL)
	{
		return;
	}

	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	this->m_pD3D10Device->ClearRenderTargetView(this->m_pRenderTargetView, ClearColor);
}

void RenderInterfaceDirectX10::PresentRenderBuffer()
{
	if(this->m_pSwapChain == NULL)
	{
		return;
	}

	this->m_pSwapChain->Present(0, 0);
}
