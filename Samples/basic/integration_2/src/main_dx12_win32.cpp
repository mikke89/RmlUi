/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2025 The RmlUi Team, and contributors
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

// author: @wh1t3lord (https://github.com/wh1t3lord)

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <DirectXMath.h>
#include <RmlUi_Backend.h>
#include <RmlUi_Include_Windows.h>
#include <Shell.h>
#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

constexpr const char pShaderSourceText_Main[] = R"(struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
};

cbuffer Constants : register(b0) {
    float4x4 WorldViewProj;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), WorldViewProj);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return float4(0.8f, 0.6f, 0.1f, 1.0f); // Orange color
})";

constexpr const char pShaderSourceText_Offscreen[] = R"(
	
)";

/// @brief We make a really simple assumption of your game engine's backend (or just engine for 3d visualization CAD, any other 3d software), so we
/// make a such simple backend only for demonstration purposes and simplier explanation however if you have some really complex backends and you have
/// some multithreading then you can proceed and make a pull request for us in order to make advanced integration sample. For now this sample provides
/// 100% general understanding how to insert backend from RmlUi to your existed solution without copy & pasting files (legacy approach) you can use
/// just RmlRenderInitInfo struct
class D3D12Renderer {
public:
	D3D12Renderer(HWND hwnd);
	~D3D12Renderer();

	void Initialize();
	void Destroy();
	void Update(float deltaTime, Rml::Context* p_context);
	void Render(Rml::Context* p_context);
	void Resize(UINT width, UINT height);

	ID3D12Device* GetDevice(void) const { return m_device.Get(); }
	IDXGISwapChain* GetSwapchain(void) const { return m_swapChain.Get(); }
	ID3D12GraphicsCommandList* GetCommandList(void) const { return m_commandList.Get(); }
	unsigned char GetSwapchainFrameCount() const { return static_cast<unsigned char>(FrameCount); }
	IDXGIAdapter* GetAdapter(void) const { return m_adapter.Get(); }
	void SetContext(Rml::Context* p_context) { m_p_context = p_context; }
	Rml::Context* GetContext(void) const { return m_p_context; }

	int GetWidth(void) const { return static_cast<int>(m_width); }
	int GetHeight(void) const { return static_cast<int>(m_height); }

private:
	void InitializeDevice();
	void InitializeCommandObjects();
	void InitializeSwapChain();
	void InitializeRTV();
	void InitializeDepthBuffer();
	void InitializeRootSignature();
	void InitializePipelineState();
	void InitializeBoxGeometry();
	void InitializeConstantBuffer();
	void InitializeFence();
	void ReleaseResources();
	void FlushCommandQueue();
	void WaitForGPU();

	struct Vertex {
		XMFLOAT3 Position;
	};

	struct ConstantBuffer {
		XMFLOAT4X4 WorldViewProj;
	};

	static const UINT FrameCount = 2;
	static const UINT ConstantBufferSize = sizeof(ConstantBuffer);

	HWND m_hwnd;
	UINT m_width;
	UINT m_height;
	float m_aspectRatio;
	float m_cameraAngle = 0.0f;
	bool m_resizePending = false;

	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12Resource> m_depthStencil;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_constantBuffer[FrameCount];
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<IDXGIAdapter1> m_adapter;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValue[FrameCount] = {};

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	Rml::Context* m_p_context;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_frameIndex = 0;
};

D3D12Renderer::D3D12Renderer(HWND hwnd) : m_hwnd(hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
	m_aspectRatio = static_cast<float>(m_width) / m_height;
	m_viewport = {0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
	m_scissorRect = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
	m_fenceEvent = 0;
	m_p_context = 0;
}

D3D12Renderer::~D3D12Renderer() {}

void D3D12Renderer::Initialize()
{
	InitializeDevice();
	InitializeCommandObjects();
	InitializeSwapChain();
	InitializeRTV();
	InitializeDepthBuffer();
	InitializeRootSignature();
	InitializePipelineState();
	InitializeBoxGeometry();
	InitializeConstantBuffer();
	InitializeFence();
}

void D3D12Renderer::Destroy()
{
	WaitForGPU();
	FlushCommandQueue();
	// 5) Don't forget to destroy resources from RmlUi and RmlUi will do its job for you
	Backend::Shutdown();
	CloseHandle(m_fenceEvent);
}

void D3D12Renderer::Resize(UINT width, UINT height)
{
	if (width == 0 || height == 0)
		return; // Skip when minimized

	if (width != m_width || height != m_height)
	{
		m_width = width;
		m_height = height;
		m_aspectRatio = static_cast<float>(width) / height;
		m_viewport = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
		m_scissorRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
		m_resizePending = true;
	}
}

void D3D12Renderer::InitializeDevice()
{
	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory1(IID_PPV_ARGS(&factory));

	ComPtr<IDXGIAdapter1> adapter;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
		{
			m_adapter = adapter;
			break;
		}
	}
}

void D3D12Renderer::InitializeCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

	for (UINT i = 0; i < FrameCount; i++)
	{
		m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i]));
	}

	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList));
	m_commandList->Close();
}

void D3D12Renderer::InitializeSwapChain()
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.BufferDesc.Width = m_width;
	swapChainDesc.BufferDesc.Height = m_height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = m_hwnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain> tempSwapChain;
	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	factory->CreateSwapChain(m_commandQueue.Get(), &swapChainDesc, &tempSwapChain);
	tempSwapChain.As(&m_swapChain);
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3D12Renderer::InitializeRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < FrameCount; i++)
	{
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
	}
}

void D3D12Renderer::InitializeDepthBuffer()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));

	D3D12_RESOURCE_DESC depthTexDesc = {};
	depthTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthTexDesc.Width = m_width;
	depthTexDesc.Height = m_height;
	depthTexDesc.DepthOrArraySize = 1;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.SampleDesc.Count = 1;
	depthTexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthTexDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
		IID_PPV_ARGS(&m_depthStencil));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_device->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void D3D12Renderer::InitializeRootSignature()
{
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam.Descriptor.ShaderRegister = 0;
	rootParam.Descriptor.RegisterSpace = 0;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
	rootSigDesc.pParameters = &rootParam;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
}

void D3D12Renderer::InitializePipelineState()
{
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	HRESULT compile_status = D3DCompile(pShaderSourceText_Main, sizeof(pShaderSourceText_Main), nullptr, nullptr, nullptr, "VSMain", "vs_5_0",
		compileFlags, 0, &vertexShader, nullptr);
	assert(SUCCEEDED(compile_status) && "failed to compile vertex shader");

	compile_status = D3DCompile(pShaderSourceText_Main, sizeof(pShaderSourceText_Main), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags,
		0, &pixelShader, nullptr);
	assert(SUCCEEDED(compile_status) && "failed to compile pixel shader");

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
	psoDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};

	// Rasterizer state
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Blend state
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		blendDesc.RenderTarget[i].BlendEnable = FALSE;
		blendDesc.RenderTarget[i].LogicOpEnable = FALSE;
		blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	psoDesc.BlendState = blendDesc;

	// Depth stencil state
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	psoDesc.DepthStencilState = depthStencilDesc;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
}

void D3D12Renderer::InitializeBoxGeometry()
{
	// Box vertices
	Vertex vertices[] = {
		// Front face
		{XMFLOAT3(-0.5f, -0.5f, -0.5f)},
		{XMFLOAT3(-0.5f, 0.5f, -0.5f)},
		{XMFLOAT3(0.5f, 0.5f, -0.5f)},
		{XMFLOAT3(0.5f, -0.5f, -0.5f)},

		// Back face
		{XMFLOAT3(-0.5f, -0.5f, 0.5f)},
		{XMFLOAT3(0.5f, -0.5f, 0.5f)},
		{XMFLOAT3(0.5f, 0.5f, 0.5f)},
		{XMFLOAT3(-0.5f, 0.5f, 0.5f)},
	};

	// Box indices
	WORD indices[] = {// Front face
		0, 1, 2, 0, 2, 3,
		// Back face
		4, 5, 6, 4, 6, 7,
		// Top face
		1, 7, 6, 1, 6, 2,
		// Bottom face
		0, 3, 5, 0, 5, 4,
		// Left face
		0, 4, 7, 0, 7, 1,
		// Right face
		3, 2, 6, 3, 6, 5};

	// Create vertex buffer
	const UINT vertexBufferSize = sizeof(vertices);
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = vertexBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_vertexBuffer));

	void* vertexData;
	D3D12_RANGE readRange = {0, 0};
	m_vertexBuffer->Map(0, &readRange, &vertexData);
	memcpy(vertexData, vertices, vertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	// Create index buffer
	const UINT indexBufferSize = sizeof(indices);
	resourceDesc.Width = indexBufferSize;
	m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&m_indexBuffer));

	void* indexData;
	m_indexBuffer->Map(0, &readRange, &indexData);
	memcpy(indexData, indices, indexBufferSize);
	m_indexBuffer->Unmap(0, nullptr);
}

void D3D12Renderer::InitializeConstantBuffer()
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = ConstantBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	for (UINT i = 0; i < FrameCount; i++)
	{
		m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_constantBuffer[i]));
	}
}

void D3D12Renderer::InitializeFence()
{
	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	for (UINT i = 0; i < FrameCount; i++)
	{
		m_fenceValue[i] = 0;
	}
}

void D3D12Renderer::ReleaseResources()
{
	FlushCommandQueue();

	for (UINT i = 0; i < FrameCount; i++)
	{
		m_renderTargets[i].Reset();
	}
	m_depthStencil.Reset();
}

void D3D12Renderer::FlushCommandQueue()
{
	m_commandQueue->Signal(m_fence.Get(), ++m_fenceValue[m_frameIndex]);
	if (m_fence->GetCompletedValue() < m_fenceValue[m_frameIndex])
	{
		m_fence->SetEventOnCompletion(m_fenceValue[m_frameIndex], m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void D3D12Renderer::WaitForGPU()
{
	const UINT64 fence = m_fenceValue[m_frameIndex];
	m_commandQueue->Signal(m_fence.Get(), fence);

	if (m_fence->GetCompletedValue() < fence)
	{
		m_fence->SetEventOnCompletion(fence, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// Reset frame fence values
	for (UINT i = 0; i < FrameCount; i++)
	{
		m_fenceValue[i] = 0;
	}
}

void D3D12Renderer::Update(float deltaTime, Rml::Context* p_context)
{
	if (m_resizePending)
	{
		FlushCommandQueue();
		ReleaseResources();

		DXGI_SWAP_CHAIN_DESC desc;
		m_swapChain->GetDesc(&desc);
		m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, desc.BufferDesc.Format, desc.Flags);

		InitializeRTV();
		InitializeDepthBuffer();
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		m_resizePending = false;

		Backend::Resize(p_context, m_width, m_height);
	}

	m_cameraAngle += 1.0f * deltaTime;

	// Calculate matrices
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(2.0f * (float)sin(m_cameraAngle), 1.0f, 2.0f * (float)cos(m_cameraAngle), 0.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, m_aspectRatio, 0.1f, 100.0f);
	XMMATRIX worldViewProj = world * view * proj;

	// Update constant buffer
	ConstantBuffer cb;
	XMStoreFloat4x4(&cb.WorldViewProj, XMMatrixTranspose(worldViewProj));

	UINT8* data;
	D3D12_RANGE readRange = {0, 0};
	m_constantBuffer[m_frameIndex]->Map(0, &readRange, reinterpret_cast<void**>(&data));
	memcpy(data, &cb, sizeof(cb));
	m_constantBuffer[m_frameIndex]->Unmap(0, nullptr);

	if (p_context)
	{
		p_context->Update();
	}
}

void D3D12Renderer::Render(Rml::Context* p_context)
{
	if (m_width == 0 || m_height == 0)
		return; // Skip rendering when minimized

	m_commandAllocator[m_frameIndex]->Reset();
	m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_pipelineState.Get());

	// Set viewport and scissor
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Transition render target
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_commandList->ResourceBarrier(1, &barrier);

	// Set render targets
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_frameIndex * m_rtvDescriptorSize};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear render targets
	const float clearColor[] = {0.1f, 0.2f, 0.3f, 1.0f};
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Set pipeline state
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer[m_frameIndex]->GetGPUVirtualAddress());

	// Set vertex and index buffers
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = sizeof(Vertex) * 8;
	m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = sizeof(WORD) * 36;
	m_commandList->IASetIndexBuffer(&indexBufferView);

	// Draw cube like we simulate game rendering
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	Backend::RmlRenderInput rtv_arg;
	Backend::RmlRenderInput dsv_arg;

/* 
	Note: That's supposed for a situation where you would like to use your own swapchain render targets and render them directly using frame buffer index


	rtv_arg.p_input_present_resource = m_renderTargets[m_frameIndex].Get();
	rtv_arg.p_input_present_resource_binding = &rtvHandle;

	dsv_arg.p_input_present_resource = m_depthStencil.Get();
	dsv_arg.p_input_present_resource_binding = &dsvHandle;
*/

	


	// Draw RmlUi in your engine
	// but keep in mind that we don't make barrier thing for your passed input arguments since it supposed that you have their state as render target
	// and they are ready for clear operations too
	Backend::BeginFrame(&rtv_arg, &dsv_arg, static_cast<unsigned char>(m_frameIndex));

	if (p_context)
	{
		p_context->Render();
	}

	Backend::EndFrame();

	// Transition back to present
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

	m_commandList->Close();

	// Execute command list
	ID3D12CommandList* commandLists[] = {m_commandList.Get()};
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// Present and swap buffers
	m_swapChain->Present(1, 0);

	// Signal fence
	const UINT64 currentFenceValue = ++m_fenceValue[m_frameIndex];
	m_commandQueue->Signal(m_fence.Get(), currentFenceValue);

	// Update frame index
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Wait for previous frame
	if (m_fence->GetCompletedValue() < m_fenceValue[m_frameIndex])
	{
		m_fence->SetEventOnCompletion(m_fenceValue[m_frameIndex], m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	m_fenceValue[m_frameIndex] = currentFenceValue + 1;
}

// Win32 Window Setup and Message Loop
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	D3D12Renderer* renderer = reinterpret_cast<D3D12Renderer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (hwnd)
	{
		if (renderer)
		{
			// filling info about our window callback data for RmlUi processing its internal state
			Backend::RmlProcessEventInfo info;

			info.hwnd = hwnd;
			info.lParam = lParam;
			info.wParam = wParam;
			info.msg = msg;

			Backend::ProcessEvents(renderer->GetContext(), info, true);
		}
	}

	switch (msg)
	{
	case WM_CREATE:
	{
		break;
	}
	case WM_SIZE:
	{
		D3D12Renderer* renderer = reinterpret_cast<D3D12Renderer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (renderer)
		{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);

			// 4) contains Backend::Resize
			renderer->Resize(width, height);
		}
		break;
	}
	case WM_DESTROY: PostQuitMessage(0); return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/// @brief Making a simulation of your engine and your complex architecture to this trivial sample, here we create window then create and initialize
/// your renderer (user) and then initialize rmlui and its backend for integrating to your renderer
/// @param hInstance
/// @param
/// @param
/// @param nCmdShow
/// @return
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	const UINT initialWidth = 800;
	const UINT initialHeight = 600;

	// Register window class
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = TEXT("D3D12WindowClass");
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);

	// Create window
	HWND hwnd = CreateWindow(wc.lpszClassName, TEXT("DX12 Resizable Rotating Camera"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		initialWidth, initialHeight, nullptr, nullptr, hInstance, nullptr);

	// Create renderer instance
	D3D12Renderer renderer(hwnd);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&renderer));

	ShowWindow(hwnd, nCmdShow);

	// Initialize renderer
	renderer.Initialize();

	// clang-format off
	/*
	 * How to integrate RmlUi backend under your user backend just in 5 steps?
	 *
	 * 1) Step one = initialize RmlUi backend
	 * 2) Step two = Load your fonts for correct working of Rml
	 * 3) Step three = Provide a place for rendering UI where you should put BeginFrame/EndFrame in your renderer engine & for updaing context using Backend::ProcessEvents (DONT FORGET about ProcessEvents)
	 * 4) Step four = Don't forget to use Backend::Resize where your window handles it and you should be able to see rendered image in your swapchain render target images
	 * targets) 
	 * 5) Step five = After using don't forget to call Backend::Shutdown() where you wish to call depends on your needs
	 */
	// clang-format on

	// 1) Step one, initialize RmlUi backend using appropriate version of Backend::Initialize function that contain single argument of
	// Backend::RmlRenderInitInfo*

	// init instance of RmlRenderInitInfo to 0 each field
	// if you forget to initialize render info correctly it will contain memory uninitialized trash and thus some resources/fields will be treated
	// like they were initialized by system and thus make initialization step failed by some reason of those trash memory fields since they might
	// trick system to believe that all is fine while it is def not fine
	Backend::RmlRenderInitInfo info{};

	// we specify that we initialize renderer not fully it means
	// RmlUi's renderer won't create own device, initialize swapchain and etc
	info.is_full_initialization = false;

	// you must specify for which backend you want to use/initialize otherwise system won't know to which types cast when initialize will come to
	// backend and which backend to use
	info.backend_type = static_cast<unsigned char>(Backend::Type::DirectX_12);

	// you must specify for which platform you want to use/initialize Rml since it requires SystemInterface for proper working
	info.system_interface_type = static_cast<unsigned char>(Backend::TypeSystemInterface::Native_Win32);

	// you must have initialize ID3D12Device* on your side
	info.p_user_device = renderer.GetDevice();

	// rmlui's required adapter for modern GPU for succeeded allocator creation
	info.p_user_adapter = renderer.GetAdapter();

	// in this example we use existed command list but if you don't want to pass field to rmlui then pass nullptr and rmlui will create own command
	// list but you have to set nullptr if you didn't initialize the whole instance of RmlRenderInitInfo to 0 all fields
	info.p_command_list = renderer.GetCommandList();

	// in this example we suppose that execution of command list comes on YOUR side it means we just record command into the exist list that we set
	// earlier it might be used when you would like to render to texture and use it as postprocess effect like rendering on quad or something (there's
	// sample integration_as_postprocess if you're curious)
	info.is_execute_when_end_frame_issued = false;

	// registering key callback since we have to provide it for ProcessEvents calling, we use default implementation only for demonstration purposes
	info.p_key_callback = &Shell::ProcessKeyDownShortcuts;

	info.initial_height = renderer.GetHeight();
	info.initial_width = renderer.GetWidth();
	std::memcpy(info.context_name, "main_dx12_win32_postprocess", sizeof("main_dx12_win32_postprocess"));

	// 1) probably we could put it under ::Initialize of renderer but it is for simplicity and for comfortable reading, so we initialize the rmlui's
	// backend where you had to specify to which one to use in current case it is DirectX-12
	Rml::Context* p_context = Backend::Initialize(&info);

	// registering context for accessing in windowproc function for processing Backend::ProcessEvents
	renderer.SetContext(p_context);

	// failed to initialize context so yeah, critical error by different things, it is useful to first iterations of development run under Debug build
	if (!p_context)
	{
		MessageBoxA(NULL, "failed to initialize context or Backend!", "ERROR", 0);
		std::exit(-1);
	}

	// 2) For simplicity and better understanding of this sample, we used default way of loading fonts from Shell BUT you have to use font_interface
	// and using your file system and your implementation/design of file system and load manually (AND AS YOU THINK IS RIGHT IN YOUR ENVIRONMENT)
	Shell::LoadFonts();

	Rml::ElementDocument* p_doc = p_context->LoadDocument("assets/demo.rml");

	if (!p_doc)
	{
		MessageBoxA(NULL, "failed to load document!", "ERROR", 0);
		std::exit(-1);
	}

	if (p_doc)
		p_doc->Show();

	// Main loop
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			static float lastTime = 0.0f;
			static LARGE_INTEGER frequency;
			static bool first = true;
			if (first)
			{
				QueryPerformanceFrequency(&frequency);
				first = false;
			}

			LARGE_INTEGER currentTime;
			QueryPerformanceCounter(&currentTime);
			float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime) / frequency.QuadPart;
			lastTime = (FLOAT)currentTime.QuadPart;

			// 3) It contains Backend::ProcessEvents AND
			// 4) It contains Resize too, but honestly some of you can call it in window's proc or where your resize handling exists
			// don't forget to call p_context->Update it contains in D3D12Renderer::Update
			renderer.Update(deltaTime, p_context);

			// 3) Backend::BeginFrame and Backend::EndFrame are inside this method of D3D12Renderer::Render
			// don't forget to call p_context->Render it contains in D3D12Renderer::Render
			renderer.Render(p_context);

			// 4) now you should see rendered UI and we congratulate you with successful integration!
		}
	}

	// 5) Backend::Shutdown inside of D3D12Renderer::Destroy but keep in mind that you have to call Backned::Shutdown after when you issued sync for
	// GPU and safely deleting resources from Backend due to fact that you sync operations, RmlUi doesn't call Flush on its side when intergration
	// works
	renderer.Destroy();
	return 0;
}