#ifndef RENDERINTERFACEDIRECTX_H
#define RENDERINTERFACEDIRECTX_H

#include <RmlUi/Core/RenderInterface.h>
#include "../../../shell/include/ShellRenderInterfaceExtensions.h"
#include <d3d10.h>
#include <d3dx10.h>

/**
	A sample render interface for RmlUi into DirectX 10.

	TODO: 
	1) Constant Buffers for variables

	@author Brian McDonald
 */

class RenderInterfaceDirectX10 : public Rml::Core::RenderInterface, public ShellRenderInterfaceExtensions
{
public:
	RenderInterfaceDirectX10();
	virtual ~RenderInterfaceDirectX10(void);

	/// Called by RmlUi when it wants to render geometry that it does not wish to optimise.
	virtual void RenderGeometry(Rml::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::Core::TextureHandle texture, const Rml::Core::Vector2f& translation);

	/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
	virtual Rml::Core::CompiledGeometryHandle CompileGeometry(Rml::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::Core::TextureHandle texture);

	/// Called by RmlUi when it wants to render application-compiled geometry.
	virtual void RenderCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry, const Rml::Core::Vector2f& translation);
	/// Called by RmlUi when it wants to release application-compiled geometry.
	virtual void ReleaseCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry);

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	virtual void EnableScissorRegion(bool enable);
	/// Called by RmlUi when it wants to change the scissor region.
	virtual void SetScissorRegion(int x, int y, int width, int height);

	/// Called by RmlUi when a texture is required by the library.
	virtual bool LoadTexture(Rml::Core::TextureHandle& texture_handle, Rml::Core::Vector2i& texture_dimensions, const Rml::Core::String& source);
	/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
	virtual bool GenerateTexture(Rml::Core::TextureHandle& texture_handle, const byte* source, const Rml::Core::Vector2i& source_dimensions);
	/// Called by RmlUi when a loaded texture is no longer required.
	virtual void ReleaseTexture(Rml::Core::TextureHandle texture_handle);

	/// Returns the native horizontal texel offset for the renderer.
	float GetHorizontalTexelOffset();
	/// Returns the native vertical texel offset for the renderer.
	float GetVerticalTexelOffset();

	//loads the effect from memory
	void setupEffect();

	// ShellRenderInterfaceExtensions
	virtual void SetViewport(int width, int height);
	virtual void SetContext(void *context);
	virtual bool AttachToNative(void *nativeWindow);
	virtual void DetachFromNative(void);
	virtual void PrepareRenderBuffer(void);
	virtual void PresentRenderBuffer(void);

private:
	// RmlUi Context, needed for when the shell window is resized so we can update the Rml::Core::Context
	// dimensions
	void *m_rmlui_context;

	//The D3D 10 Device
	ID3D10Device * m_pD3D10Device;
	//The Effect we are using to render GUI
	ID3D10Effect * m_pEffect;
	//The Current technique
	ID3D10EffectTechnique*  m_pTechnique;
	//The Vertex Layout
	ID3D10InputLayout*      m_pVertexLayout;

	//Swap Chain
	IDXGISwapChain* m_pSwapChain;
	//Render Target
	ID3D10RenderTargetView* m_pRenderTargetView;

	//Effect variables, used to send variables to the effect
	ID3D10EffectMatrixVariable * m_pProjectionMatrixVariable;
	ID3D10EffectMatrixVariable * m_pWorldMatrixVariable;
	ID3D10EffectShaderResourceVariable *m_pDiffuseTextureVariable;

	//Matrices
	D3DXMATRIX m_matProjection;
	D3DXMATRIX m_matWorld;

	//Renderstate Blocks
	ID3D10RasterizerState *m_pScissorTestEnable;
	ID3D10RasterizerState *m_pScissorTestDisable;

};

#endif