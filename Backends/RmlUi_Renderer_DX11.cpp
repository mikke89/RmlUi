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

#include <RmlUi/Core/Platform.h>

#if defined RMLUI_PLATFORM_WIN32

	#include "RmlUi_Renderer_DX11.h"
	#include <RmlUi/Core/Core.h>
	#include <RmlUi/Core/FileInterface.h>
	#include <RmlUi/Core/Log.h>

	#include "RmlUi_Include_Windows.h"

RenderInterface_DX11::RenderInterface_DX11() {

}

void RenderInterface_DX11::Init(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext, IDXGISwapChain* pSwapChain,
	ID3D11RenderTargetView* mainRenderTargetView)
{
	RMLUI_ASSERTMSG(pd3dDevice, "pd3dDevice cannot be nullptr!");
	RMLUI_ASSERTMSG(pSwapChain, "pSwapChain cannot be nullptr!");
	RMLUI_ASSERTMSG(pd3dDeviceContext, "pd3dDeviceContext cannot be nullptr!");
	RMLUI_ASSERTMSG(mainRenderTargetView, "mainRenderTargetView cannot be nullptr!");

	// Assign D3D resources
	m_d3dDevice = pd3dDevice;
	m_d3dContext = pd3dDeviceContext;
	m_swapChain = pSwapChain;
	m_mainRenderTargetView = mainRenderTargetView;

	// RmlUi serves vertex colors and textures with premultiplied alpha, set the blend mode accordingly.
	// Equivalent to glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
	if (!m_blendState)
	{
		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		HRESULT result = m_d3dDevice->CreateBlendState(&blendDesc, &m_blendState);
		if (FAILED(result))
		{
			Rml::Log::Message(Rml::Log::LT_ERROR, "ID3D11Device1::CreateBlendState (%d)", result);
		}
	}
}

void RenderInterface_DX11::Cleanup() {
	m_blendState->Release();
}

void RenderInterface_DX11::BeginFrame() {
	RMLUI_ASSERTMSG(m_d3dContext, "d3dContext cannot be nullptr!");
	RMLUI_ASSERTMSG(m_d3dDevice, "d3dDevice cannot be nullptr!");

	D3D11_VIEWPORT d3dviewport;
	d3dviewport.TopLeftX = 0;
	d3dviewport.TopLeftY = 0;
	d3dviewport.Width = m_width;
	d3dviewport.Height = m_height;
	d3dviewport.MinDepth = 0.0f;
	d3dviewport.MaxDepth = 1.0f;
	m_d3dContext->RSSetViewports(1, &d3dviewport);
	Clear();
	SetBlendState(m_blendState);
}

void RenderInterface_DX11::EndFrame() {

}


Rml::CompiledGeometryHandle RenderInterface_DX11::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
	return Rml::CompiledGeometryHandle(0);
}

void RenderInterface_DX11::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {

}

void RenderInterface_DX11::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) {

}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1)
struct TGAHeader {
	char idLength;
	char colourMapType;
	char dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char bitsPerPixel;
	char imageDescriptor;
};
// Restore packing
#pragma pack()


Rml::TextureHandle RenderInterface_DX11::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
	{
		return false;
	}

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	if (buffer_size <= sizeof(TGAHeader))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Texture file size is smaller than TGAHeader, file is not a valid TGA image.");
		file_interface->Close(file_handle);
		return false;
	}

	using Rml::byte;
	Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer.get(), sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	const size_t image_size = header.width * header.height * 4; // We always make 32bit textures

	if (header.dataType != 2)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return false;
	}

	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported.");
		return false;
	}

	const byte* image_src = buffer.get() + sizeof(TGAHeader);
	Rml::UniquePtr<byte[]> image_dest_buffer(new byte[image_size]);
	byte* image_dest = image_dest_buffer.get();

	// Targa is BGR, swap to RGB, flip Y axis, and convert to premultiplied alpha.
	for (long y = 0; y < header.height; y++)
	{
		long read_index = y * header.width * color_mode;
		long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : (header.height - y - 1) * header.width * 4;
		for (long x = 0; x < header.width; x++)
		{
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4)
			{
				const byte alpha = image_src[read_index + 3];
				for (size_t j = 0; j < 3; j++)
					image_dest[write_index + j] = byte((image_dest[write_index + j] * alpha) / 255);
				image_dest[write_index + 3] = alpha;
			}
			else
				image_dest[write_index + 3] = 255;

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;

	return GenerateTexture({image_dest, image_size}, texture_dimensions);
}

Rml::TextureHandle RenderInterface_DX11::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
	return Rml::TextureHandle(0);
}

void RenderInterface_DX11::ReleaseTexture(Rml::TextureHandle texture_handle) {

}

void RenderInterface_DX11::EnableScissorRegion(bool enable) {

}

void RenderInterface_DX11::SetScissorRegion(Rml::Rectanglei region) {
}

void RenderInterface_DX11::SetViewport(const int width, const int height) {
	m_width = width;
	m_height = height;
}

void RenderInterface_DX11::Clear() {
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_d3dContext->ClearRenderTargetView(m_mainRenderTargetView, clearColor);
}

void RenderInterface_DX11::SetBlendState(ID3D11BlendState* blendState) {
	if (blendState != m_currentBlendState)
	{
		m_d3dContext->OMSetBlendState(blendState, 0, 0xFFFFFFFF);
		m_currentBlendState = blendState;
	}
}
#endif // RMLUI_PLATFORM_WIN32