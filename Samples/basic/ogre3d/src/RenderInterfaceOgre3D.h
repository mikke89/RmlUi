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

#ifndef RENDERINTERFACEOGRE3D_H
#define RENDERINTERFACEOGRE3D_H

#include <Rocket/Core/RenderInterface.h>
#include <Ogre.h>

/**
	A sample render interface for Rocket into Ogre3D.

	@author Peter Curry
 */

class RenderInterfaceOgre3D : public Rocket::Core::RenderInterface
{
	public:
		RenderInterfaceOgre3D(unsigned int window_width, unsigned int window_height);
		virtual ~RenderInterfaceOgre3D();

		/// Called by Rocket when it wants to render geometry that it does not wish to optimise.
		virtual void RenderGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture, const EMP::Core::Vector2f& translation);

		/// Called by Rocket when it wants to compile geometry it believes will be static for the forseeable future.
		virtual Rocket::Core::CompiledGeometryHandle CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture);

		/// Called by Rocket when it wants to render application-compiled geometry.
		virtual void RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const EMP::Core::Vector2f& translation);
		/// Called by Rocket when it wants to release application-compiled geometry.
		virtual void ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry);

		/// Called by Rocket when it wants to enable or disable scissoring to clip content.
		virtual void EnableScissorRegion(bool enable);
		/// Called by Rocket when it wants to change the scissor region.
		virtual void SetScissorRegion(int x, int y, int width, int height);

		/// Called by Rocket when a texture is required by the library.
		virtual bool LoadTexture(Rocket::Core::TextureHandle& texture_handle, EMP::Core::Vector2i& texture_dimensions, const EMP::Core::String& source);
		/// Called by Rocket when a texture is required to be built from an internally-generated sequence of pixels.
		virtual bool GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const EMP::Core::byte* source, const EMP::Core::Vector2i& source_dimensions);
		/// Called by Rocket when a loaded texture is no longer required.
		virtual void ReleaseTexture(Rocket::Core::TextureHandle texture);

		/// Returns the native horizontal texel offset for the renderer.
		float GetHorizontalTexelOffset();
		/// Returns the native vertical texel offset for the renderer.
		float GetVerticalTexelOffset();

	private:
		Ogre::RenderSystem* render_system;

		Ogre::LayerBlendModeEx colour_blend_mode;
		Ogre::LayerBlendModeEx alpha_blend_mode;

		bool scissor_enable;
		int scissor_left;
		int scissor_top;
		int scissor_right;
		int scissor_bottom;
};

#endif
